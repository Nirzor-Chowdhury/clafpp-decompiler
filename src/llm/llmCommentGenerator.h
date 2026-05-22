#pragma once

// llmCommentGenerator.h
// ----------------------------------------------------------------------------
// High-level interface for generating documentation, inline comments, variable
// name suggestions, and complexity analyses from decompiled C code using a
// local Ollama instance.
//
// This class wraps OllamaInterface with:
//   * Prompt templates appropriate to each task
//   * Response post-processing (stripping prefaces, normalising whitespace)
//   * An LRU-ish in-memory cache keyed on (task, model, code hash)
//   * Thread-safe cancellation
//
// Threading model:
//   The public methods are blocking. Callers wanting non-blocking behaviour
//   (the wxWidgets GUI) should run them on a std::thread and marshal the
//   result back to the UI thread (e.g. via wxEvtHandler::CallAfter). The
//   `cancel()` method can be called from any thread to abort whatever
//   request is currently in flight.
// ----------------------------------------------------------------------------

#include "llmConfig.h"
#include "ollama_interface.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace jdc {
namespace llm {

// Variable rename suggestion produced by suggestVariableNames().
struct VariableRename
{
    std::string original;
    std::string suggested;
    std::string reason; // optional, may be empty
};

class LLMCommentGenerator
{
public:
    explicit LLMCommentGenerator(const Settings& config);
    ~LLMCommentGenerator();

    LLMCommentGenerator(const LLMCommentGenerator&)            = delete;
    LLMCommentGenerator& operator=(const LLMCommentGenerator&) = delete;

    // ---- Configuration -----------------------------------------------------
    void updateConfig(const Settings& config);
    Settings getConfig() const;

    // ---- High-level tasks --------------------------------------------------
    // Returns a block of C comments (the formatting follows the configured
    // CommentStyle) describing the function. Empty string on failure - call
    // getLastError() for details.
    std::string generateFunctionComment(const std::string& code,
                                        const std::string& funcName);

    // Returns one short comment per non-trivial line of the function. The
    // returned vector is parallel to the input lines, with empty strings for
    // lines that don't warrant a comment.
    std::vector<std::string> generateLineComments(const std::string& code);

    // Returns rename suggestions for variables found in the snippet.
    std::vector<VariableRename> suggestVariableNames(const std::string& context);

    // Returns a short human-readable complexity analysis (cyclomatic
    // complexity estimate + qualitative notes).
    std::string analyzeComplexity(const std::string& code);

    // ---- Service / model management ----------------------------------------
    bool validateOllamaConnection();
    bool pullModel(const std::string& modelName);
    std::vector<std::string> getAvailableModels();
    // Ensures the configured model exists locally; pulls it if missing and
    // autoDetectModels is enabled. Returns true if the model is usable.
    bool ensureConfiguredModel();

    // ---- Cancellation & errors --------------------------------------------
    void cancel();
    void reset();
    std::string getLastError() const;

    // Clear cached results. Returns the number of entries removed.
    size_t clearCache();

private:
    // ---- Prompt construction ----------------------------------------------
    std::string buildFunctionCommentPrompt(const std::string& code,
                                           const std::string& funcName) const;
    std::string buildLineCommentsPrompt(const std::string& code) const;
    std::string buildVariableRenamePrompt(const std::string& context) const;
    std::string buildComplexityPrompt(const std::string& code) const;

    // ---- Response post-processing -----------------------------------------
    // Strip common LLM prefaces ("Sure, here is..."), trim, and apply the
    // requested CommentStyle to a free-form text block.
    std::string formatAsComment(const std::string& raw,
                                CommentStyle style) const;

    // Heuristic JSON extraction: many small models wrap JSON in prose. This
    // finds the first balanced { ... } or [ ... ] in the string.
    static bool extractJson(const std::string& raw, std::string& outJson);

    // ---- Generation with retry --------------------------------------------
    // Wraps OllamaInterface::generate with exponential backoff (3 attempts:
    // 0s, 1s, 2s). Cancellation short-circuits the wait.
    std::string generateWithRetry(const std::string& prompt,
                                  const GenerateOptions& opts);

    // ---- Caching ----------------------------------------------------------
    static std::string makeCacheKey(const char* task,
                                    const std::string& model,
                                    const std::string& code);
    bool        cacheLookup(const std::string& key, std::string& out) const;
    void        cacheStore (const std::string& key, const std::string& value);

    // ---- State ------------------------------------------------------------
    Settings                          m_config;
    std::unique_ptr<OllamaInterface>  m_ollama;
    mutable std::mutex                m_mutex;
    std::unordered_map<std::string, std::string> m_cache;
    static constexpr size_t           kMaxCacheEntries = 256;

    mutable std::mutex                m_errMutex;
    std::string                       m_lastError;
    void setError(const std::string& e);
};

} // namespace llm
} // namespace jdc
