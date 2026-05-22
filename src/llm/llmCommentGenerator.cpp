// llmCommentGenerator.cpp
// ----------------------------------------------------------------------------
// Implementation of LLMCommentGenerator.
// ----------------------------------------------------------------------------

#include "llmCommentGenerator.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <functional>
#include <sstream>
#include <thread>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace jdc {
namespace llm {

// ---------------------------------------------------------------------------
// Construction / config
// ---------------------------------------------------------------------------
LLMCommentGenerator::LLMCommentGenerator(const Settings& config)
    : m_config(config)
    , m_ollama(std::make_unique<OllamaInterface>(config.ollama.endpoint))
{
}

LLMCommentGenerator::~LLMCommentGenerator() = default;

void LLMCommentGenerator::updateConfig(const Settings& config)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    const bool endpointChanged = (config.ollama.endpoint != m_config.ollama.endpoint);
    m_config = config;
    if (endpointChanged)
        m_ollama->setEndpoint(config.ollama.endpoint);
}

Settings LLMCommentGenerator::getConfig() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_config;
}

void LLMCommentGenerator::cancel() { m_ollama->cancel(); }
void LLMCommentGenerator::reset()  { m_ollama->reset();  setError(""); }

std::string LLMCommentGenerator::getLastError() const
{
    std::lock_guard<std::mutex> lk(m_errMutex);
    if (!m_lastError.empty()) return m_lastError;
    return m_ollama ? m_ollama->getLastError() : std::string();
}

void LLMCommentGenerator::setError(const std::string& e)
{
    std::lock_guard<std::mutex> lk(m_errMutex);
    m_lastError = e;
}

size_t LLMCommentGenerator::clearCache()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    size_t n = m_cache.size();
    m_cache.clear();
    return n;
}

// ---------------------------------------------------------------------------
// Service / model management
// ---------------------------------------------------------------------------
bool LLMCommentGenerator::validateOllamaConnection()
{
    if (!m_ollama->isServiceRunning())
    {
        setError("Ollama service is not reachable at " + m_config.ollama.endpoint +
                 ". Is it running? Try: `ollama serve`");
        return false;
    }
    return true;
}

std::vector<std::string> LLMCommentGenerator::getAvailableModels()
{
    std::vector<std::string> names;
    for (const auto& m : m_ollama->listModels()) names.push_back(m.name);
    return names;
}

bool LLMCommentGenerator::pullModel(const std::string& modelName)
{
    return m_ollama->pullModel(modelName);
}

bool LLMCommentGenerator::ensureConfiguredModel()
{
    if (!validateOllamaConnection()) return false;

    Settings cfg = getConfig();
    auto models = getAvailableModels();

    auto match = [&](const std::string& want)
    {
        for (const auto& m : models)
        {
            // Models often appear as "mistral:latest" - accept exact or prefix
            if (m == want) return true;
            if (m.size() > want.size() && m.compare(0, want.size(), want) == 0
                && m[want.size()] == ':')
                return true;
        }
        return false;
    };

    if (match(cfg.ollama.modelName)) return true;

    if (!cfg.commenting.autoDetectModels)
    {
        setError("Model '" + cfg.ollama.modelName + "' not found locally and "
                 "autoDetectModels is disabled. Pull it with "
                 "`ollama pull " + cfg.ollama.modelName + "`.");
        return false;
    }

    // Try to pull it
    if (!pullModel(cfg.ollama.modelName))
    {
        // fallback: if "mistral" is locally available, switch to it transparently
        if (cfg.ollama.modelName != "mistral" && match("mistral"))
        {
            cfg.ollama.modelName = "mistral";
            updateConfig(cfg);
            return true;
        }
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Prompt construction
// ---------------------------------------------------------------------------
namespace {

const char* styleHint(CommentStyle s)
{
    switch (s)
    {
        case CommentStyle::Doxygen:
            return "Use Doxygen format: /** ... */ with @brief, @param, "
                   "@return tags.";
        case CommentStyle::Verbose:
            return "Use C-style // comments. Include reasoning across multiple "
                   "lines where helpful.";
        case CommentStyle::Simple:
        default:
            return "Use short C-style // comments. Be concise: one or two "
                   "lines total.";
    }
}

} // anonymous

std::string LLMCommentGenerator::buildFunctionCommentPrompt(
    const std::string& code, const std::string& funcName) const
{
    std::ostringstream ss;
    ss << "You are an experienced reverse engineer reviewing decompiled C "
          "code. Analyse this function and write a documentation comment "
          "describing what it does, its parameters (if any), and its return "
          "value.\n\n"
          "Important: this code came from a decompiler, so variable names are "
          "synthetic. Focus on behaviour, not on the literal names.\n\n";
    ss << styleHint(m_config.commenting.commentStyle) << "\n\n";
    ss << "Output ONLY the comment block. Do not repeat the function code, do "
          "not include any preface like \"Here is the comment:\".\n\n";
    if (!funcName.empty())
        ss << "Function name: " << funcName << "\n";
    ss << "Function source:\n```c\n" << code << "\n```\n";
    return ss.str();
}

std::string LLMCommentGenerator::buildLineCommentsPrompt(const std::string& code) const
{
    std::ostringstream ss;
    ss << "You are reviewing decompiled C code. For each numbered line below, "
          "produce a single short inline comment (no more than 10 words) if "
          "and only if the line does something non-obvious. For trivial lines "
          "(declarations, braces, blank lines, simple returns), output an "
          "empty string.\n\n"
          "Output strictly as JSON: an array of strings, exactly one element "
          "per input line, in order. Do not include the code itself in the "
          "output. Do not wrap the array in any other object.\n\n"
          "Lines:\n";

    // Number each line for clarity
    std::istringstream in(code);
    std::string line;
    int n = 1;
    while (std::getline(in, line))
    {
        ss << n << ": " << line << "\n";
        ++n;
    }
    return ss.str();
}

std::string LLMCommentGenerator::buildVariableRenamePrompt(const std::string& context) const
{
    std::ostringstream ss;
    ss << "You are reviewing decompiled C code. Suggest more meaningful names "
          "for the synthetic variables (typically named var1, local_4, arg_0, "
          "etc.) based on how they are used.\n\n"
          "Output strictly as JSON: an array of objects, each with fields "
          "\"original\", \"suggested\", and optionally \"reason\". Suggest "
          "snake_case names. Output the JSON ONLY, no surrounding prose.\n\n"
          "Code:\n```c\n" << context << "\n```\n";
    return ss.str();
}

std::string LLMCommentGenerator::buildComplexityPrompt(const std::string& code) const
{
    std::ostringstream ss;
    ss << "Analyse the complexity of this decompiled C function. Provide:\n"
          "  1. An estimated cyclomatic complexity (an integer).\n"
          "  2. A one-sentence qualitative rating "
          "(trivial / simple / moderate / complex / very complex).\n"
          "  3. Two to three bullet points highlighting what contributes to "
          "the complexity (loops, branches, indirect calls, etc.).\n\n"
          "Be concise. Output as plain text, not JSON. Do not repeat the "
          "code.\n\nCode:\n```c\n" << code << "\n```\n";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Post-processing
// ---------------------------------------------------------------------------
namespace {

std::string trim(std::string s)
{
    auto notSpace = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string stripCodeFences(std::string s)
{
    // Remove leading ```... and trailing ```
    auto start = s.find("```");
    if (start != std::string::npos)
    {
        // Find end of opening fence line
        auto eol = s.find('\n', start);
        if (eol != std::string::npos)
        {
            // Only strip if we can also find a closing fence
            auto end = s.rfind("```");
            if (end != std::string::npos && end > eol)
            {
                s = s.substr(eol + 1, end - eol - 1);
            }
        }
    }
    return s;
}

// Strip common LLM prefaces like "Sure! Here is..." up to the first comment
// marker or up to the first blank line.
std::string stripPreface(std::string s)
{
    // If the response already begins with a comment marker, leave it.
    auto firstNonSpace = s.find_first_not_of(" \t\r\n");
    if (firstNonSpace == std::string::npos) return s;
    char c = s[firstNonSpace];
    if (c == '/' || c == '*') return s.substr(firstNonSpace);

    // Otherwise strip the first line if it's prose.
    auto nl = s.find('\n', firstNonSpace);
    if (nl == std::string::npos) return s;
    std::string firstLine = s.substr(firstNonSpace, nl - firstNonSpace);
    static const char* prefaces[] = {
        "sure", "here", "certainly", "of course", "okay", "ok,", "this function",
        "the function", "below is", "i'll", "let me"
    };
    std::string lower = firstLine;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch){ return std::tolower(ch); });
    for (const char* p : prefaces)
    {
        if (lower.find(p) == 0) return s.substr(nl + 1);
    }
    return s;
}

std::string toLineComments(const std::string& body)
{
    std::ostringstream out;
    std::istringstream in(body);
    std::string line;
    bool first = true;
    while (std::getline(in, line))
    {
        // Strip any pre-existing comment markers so we re-apply uniformly
        std::string t = trim(line);
        if (t.empty() && first) continue;
        first = false;
        if (t.rfind("//", 0) == 0) out << t;
        else                       out << "// " << t;
        out << "\n";
    }
    return out.str();
}

std::string toDoxygenBlock(const std::string& body)
{
    std::ostringstream out;
    out << "/**\n";
    std::istringstream in(body);
    std::string line;
    while (std::getline(in, line))
    {
        std::string t = trim(line);
        if (t.empty()) { out << " *\n"; continue; }
        // strip leading "* " or "//" from model output to normalise
        if (t.rfind("//", 0) == 0) t = trim(t.substr(2));
        else if (t.rfind("* ", 0) == 0) t = trim(t.substr(2));
        else if (t.rfind("*",  0) == 0) t = trim(t.substr(1));
        out << " * " << t << "\n";
    }
    out << " */\n";
    return out.str();
}

} // anonymous

std::string LLMCommentGenerator::formatAsComment(const std::string& raw,
                                                 CommentStyle style) const
{
    std::string s = trim(stripPreface(stripCodeFences(raw)));
    if (s.empty()) return s;

    // If the model already produced a comment block, normalise instead of
    // double-wrapping.
    bool alreadyCommented = (s.rfind("/*", 0) == 0) ||
                            (s.rfind("//", 0) == 0);

    switch (style)
    {
        case CommentStyle::Doxygen:
            if (s.rfind("/**", 0) == 0) return s; // already doxygen
            // Convert plain-text or // body to doxygen
            return toDoxygenBlock(alreadyCommented ? s : s);
        case CommentStyle::Simple:
        case CommentStyle::Verbose:
        default:
            if (alreadyCommented) return s;
            return toLineComments(s);
    }
}

bool LLMCommentGenerator::extractJson(const std::string& raw, std::string& outJson)
{
    // Find the first '{' or '[' and scan for the matching balanced close,
    // ignoring brackets inside strings.
    const size_t n = raw.size();
    for (size_t i = 0; i < n; ++i)
    {
        char open = raw[i];
        if (open != '{' && open != '[') continue;
        char close = (open == '{') ? '}' : ']';

        int depth = 0;
        bool inStr = false;
        bool esc   = false;
        for (size_t j = i; j < n; ++j)
        {
            char c = raw[j];
            if (esc) { esc = false; continue; }
            if (c == '\\' && inStr) { esc = true; continue; }
            if (c == '"') { inStr = !inStr; continue; }
            if (inStr) continue;
            if (c == open)  ++depth;
            else if (c == close)
            {
                if (--depth == 0)
                {
                    outJson = raw.substr(i, j - i + 1);
                    return true;
                }
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Retry helper
// ---------------------------------------------------------------------------
std::string LLMCommentGenerator::generateWithRetry(const std::string& prompt,
                                                   const GenerateOptions& opts)
{
    const int kMaxAttempts = 3;
    std::string model;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        model = m_config.ollama.modelName;
    }

    for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
    {
        if (m_ollama->isCancelled()) return {};

        std::string r = m_ollama->generate(model, prompt, opts);
        if (!r.empty()) return r;

        if (m_ollama->isCancelled()) return {};

        // Exponential backoff: 0s, 1s, 2s. Sleep in small slices so we can
        // honour cancellation promptly.
        int waitMs = (attempt == 0) ? 0 : (attempt == 1 ? 1000 : 2000);
        for (int slept = 0; slept < waitMs; slept += 50)
        {
            if (m_ollama->isCancelled()) return {};
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    if (getLastError().empty())
        setError("Ollama generation failed after retries");
    return {};
}

// ---------------------------------------------------------------------------
// Cache
// ---------------------------------------------------------------------------
std::string LLMCommentGenerator::makeCacheKey(const char* task,
                                              const std::string& model,
                                              const std::string& code)
{
    // std::hash isn't cryptographic but it's fine for an in-memory cache.
    size_t h = std::hash<std::string>{}(code);
    std::ostringstream ss;
    ss << task << '|' << model << '|' << std::hex << h;
    return ss.str();
}

bool LLMCommentGenerator::cacheLookup(const std::string& key, std::string& out) const
{
    if (!m_config.commenting.cacheResults) return false;
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) return false;
    out = it->second;
    return true;
}

void LLMCommentGenerator::cacheStore(const std::string& key, const std::string& value)
{
    if (!m_config.commenting.cacheResults) return;
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_cache.size() >= kMaxCacheEntries)
    {
        // Cheap eviction: drop one arbitrary entry. Good enough for a
        // user-driven workflow where the cache mostly avoids regenerating
        // the same function within a session.
        m_cache.erase(m_cache.begin());
    }
    m_cache[key] = value;
}

// ---------------------------------------------------------------------------
// Public task methods
// ---------------------------------------------------------------------------
std::string LLMCommentGenerator::generateFunctionComment(const std::string& code,
                                                         const std::string& funcName)
{
    if (code.empty()) { setError("generateFunctionComment: empty code"); return {}; }

    Settings cfg = getConfig();
    std::string key = makeCacheKey("fnComment", cfg.ollama.modelName, code + "|" + funcName);
    std::string cached;
    if (cacheLookup(key, cached)) return cached;

    GenerateOptions opts;
    opts.maxTokens   = cfg.ollama.maxTokens;
    opts.temperature = cfg.ollama.temperature;
    opts.topP        = cfg.ollama.topP;
    opts.timeoutSec  = cfg.ollama.timeoutSec;
    opts.system      = "You are a careful, terse software engineer. You return "
                       "only what you are asked for, with no preface or commentary.";

    std::string raw = generateWithRetry(buildFunctionCommentPrompt(code, funcName), opts);
    if (raw.empty()) return {};

    std::string formatted = formatAsComment(raw, cfg.commenting.commentStyle);
    cacheStore(key, formatted);
    return formatted;
}

std::vector<std::string> LLMCommentGenerator::generateLineComments(const std::string& code)
{
    std::vector<std::string> result;
    if (code.empty()) { setError("generateLineComments: empty code"); return result; }

    // Count expected lines so we can pad/trim
    size_t expected = 1;
    for (char c : code) if (c == '\n') ++expected;
    if (!code.empty() && code.back() == '\n') --expected;

    Settings cfg = getConfig();
    GenerateOptions opts;
    opts.maxTokens   = std::max(cfg.ollama.maxTokens, 800);
    opts.temperature = std::min(cfg.ollama.temperature, 0.3);
    opts.topP        = cfg.ollama.topP;
    opts.timeoutSec  = cfg.ollama.timeoutSec;
    opts.system      = "You output JSON arrays exactly as specified, with no "
                       "preface, no markdown, no trailing commentary.";

    std::string raw = generateWithRetry(buildLineCommentsPrompt(code), opts);
    if (raw.empty()) { result.assign(expected, ""); return result; }

    std::string jsonStr;
    if (!extractJson(raw, jsonStr))
    {
        setError("Could not find JSON array in model response");
        result.assign(expected, "");
        return result;
    }

    try
    {
        json j = json::parse(jsonStr);
        if (!j.is_array())
        {
            setError("Line-comments response was not a JSON array");
            result.assign(expected, "");
            return result;
        }
        for (const auto& el : j)
            result.push_back(el.is_string() ? el.get<std::string>() : std::string{});
    }
    catch (const std::exception& e)
    {
        setError(std::string("Failed to parse line-comments JSON: ") + e.what());
        result.assign(expected, "");
        return result;
    }

    // Normalise length: pad with empties or truncate
    if (result.size() < expected) result.resize(expected, "");
    if (result.size() > expected) result.resize(expected);
    return result;
}

std::vector<VariableRename> LLMCommentGenerator::suggestVariableNames(const std::string& context)
{
    std::vector<VariableRename> out;
    if (context.empty()) { setError("suggestVariableNames: empty context"); return out; }

    Settings cfg = getConfig();
    GenerateOptions opts;
    opts.maxTokens   = std::max(cfg.ollama.maxTokens, 600);
    opts.temperature = std::min(cfg.ollama.temperature, 0.4);
    opts.topP        = cfg.ollama.topP;
    opts.timeoutSec  = cfg.ollama.timeoutSec;
    opts.system      = "You output JSON exactly as specified, with no preface.";

    std::string raw = generateWithRetry(buildVariableRenamePrompt(context), opts);
    if (raw.empty()) return out;

    std::string jsonStr;
    if (!extractJson(raw, jsonStr))
    {
        setError("Could not find JSON in variable-rename response");
        return out;
    }

    try
    {
        json j = json::parse(jsonStr);
        if (!j.is_array()) { setError("Rename response not a JSON array"); return out; }
        for (const auto& el : j)
        {
            if (!el.is_object()) continue;
            VariableRename r;
            if (el.contains("original")  && el["original"].is_string())  r.original  = el["original"].get<std::string>();
            if (el.contains("suggested") && el["suggested"].is_string()) r.suggested = el["suggested"].get<std::string>();
            if (el.contains("reason")    && el["reason"].is_string())    r.reason    = el["reason"].get<std::string>();
            if (!r.original.empty() && !r.suggested.empty()) out.push_back(std::move(r));
        }
    }
    catch (const std::exception& e)
    {
        setError(std::string("Failed to parse rename JSON: ") + e.what());
    }
    return out;
}

std::string LLMCommentGenerator::analyzeComplexity(const std::string& code)
{
    if (code.empty()) { setError("analyzeComplexity: empty code"); return {}; }

    Settings cfg = getConfig();
    std::string key = makeCacheKey("complexity", cfg.ollama.modelName, code);
    std::string cached;
    if (cacheLookup(key, cached)) return cached;

    GenerateOptions opts;
    opts.maxTokens   = std::max(cfg.ollama.maxTokens, 400);
    opts.temperature = std::min(cfg.ollama.temperature, 0.3);
    opts.topP        = cfg.ollama.topP;
    opts.timeoutSec  = cfg.ollama.timeoutSec;
    opts.system      = "You are a careful, terse software engineer.";

    std::string raw = generateWithRetry(buildComplexityPrompt(code), opts);
    if (raw.empty()) return {};

    // The complexity report is plain text - just trim + strip preface.
    std::string s = raw;
    // Reuse the helpers via formatAsComment(Verbose), but here we don't
    // actually want comment markers. Just trim.
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))  s.pop_back();

    cacheStore(key, s);
    return s;
}

} // namespace llm
} // namespace jdc
