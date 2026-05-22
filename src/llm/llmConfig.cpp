// llmConfig.cpp
// ----------------------------------------------------------------------------
// Implements load/save for the LLM settings JSON file and computes the
// platform-appropriate default config location.
// ----------------------------------------------------------------------------

#include "llmConfig.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#if defined(_WIN32)
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <pwd.h>
    #include <unistd.h>
    #include <sys/types.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace jdc {
namespace llm {

// ---------------------------------------------------------------------------
// CommentStyle <-> string
// ---------------------------------------------------------------------------
const char* Settings::commentStyleToString(CommentStyle s)
{
    switch (s)
    {
        case CommentStyle::Simple:  return "simple";
        case CommentStyle::Doxygen: return "doxygen";
        case CommentStyle::Verbose: return "verbose";
    }
    return "simple";
}

CommentStyle Settings::commentStyleFromString(const std::string& s)
{
    if (s == "doxygen") return CommentStyle::Doxygen;
    if (s == "verbose") return CommentStyle::Verbose;
    return CommentStyle::Simple;
}

// ---------------------------------------------------------------------------
// Default config path
// ---------------------------------------------------------------------------
std::string Settings::defaultConfigPath()
{
#if defined(_WIN32)
    // %APPDATA%\jdc\ollama_settings.json
    PWSTR pathW = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pathW)))
    {
        // Convert wide -> utf8 the simple way for ASCII paths; for
        // robustness we use WideCharToMultiByte.
        int len = WideCharToMultiByte(CP_UTF8, 0, pathW, -1, nullptr, 0, nullptr, nullptr);
        std::string base(len > 0 ? len - 1 : 0, '\0');
        if (len > 0)
        {
            WideCharToMultiByte(CP_UTF8, 0, pathW, -1, base.data(), len, nullptr, nullptr);
        }
        CoTaskMemFree(pathW);
        fs::path p = fs::path(base) / "jdc" / "ollama_settings.json";
        return p.string();
    }
    // Fallback: current dir
    return (fs::path("jdc") / "ollama_settings.json").string();
#else
    // XDG: $XDG_CONFIG_HOME or $HOME/.config
    const char* xdg  = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    fs::path base;
    if (xdg && *xdg)
    {
        base = xdg;
    }
    else if (home && *home)
    {
        base = fs::path(home) / ".config";
    }
    else
    {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) base = fs::path(pw->pw_dir) / ".config";
        else                  base = ".";
    }
    return (base / "jdc" / "ollama_settings.json").string();
#endif
}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------
Settings Settings::load(const std::string& path, std::string* outError)
{
    Settings s; // start with defaults

    std::error_code ec;
    if (!fs::exists(path, ec))
    {
        if (outError) *outError = "Config file does not exist (using defaults): " + path;
        return s;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        if (outError) *outError = "Could not open config file: " + path;
        return s;
    }

    try
    {
        json j;
        in >> j;

        if (j.contains("ollama") && j["ollama"].is_object())
        {
            const auto& o = j["ollama"];
            if (o.contains("endpoint")    && o["endpoint"].is_string())    s.ollama.endpoint    = o["endpoint"].get<std::string>();
            if (o.contains("modelName")   && o["modelName"].is_string())   s.ollama.modelName   = o["modelName"].get<std::string>();
            if (o.contains("temperature") && o["temperature"].is_number()) s.ollama.temperature = o["temperature"].get<double>();
            if (o.contains("topP")        && o["topP"].is_number())        s.ollama.topP        = o["topP"].get<double>();
            if (o.contains("maxTokens")   && o["maxTokens"].is_number())   s.ollama.maxTokens   = o["maxTokens"].get<int>();
            if (o.contains("timeout")     && o["timeout"].is_number())     s.ollama.timeoutSec  = o["timeout"].get<int>();
        }

        if (j.contains("commenting") && j["commenting"].is_object())
        {
            const auto& c = j["commenting"];
            if (c.contains("generateFunctionComments") && c["generateFunctionComments"].is_boolean())
                s.commenting.generateFunctionComments = c["generateFunctionComments"].get<bool>();
            if (c.contains("generateLineComments") && c["generateLineComments"].is_boolean())
                s.commenting.generateLineComments = c["generateLineComments"].get<bool>();
            if (c.contains("commentStyle") && c["commentStyle"].is_string())
                s.commenting.commentStyle = commentStyleFromString(c["commentStyle"].get<std::string>());
            if (c.contains("autoDetectModels") && c["autoDetectModels"].is_boolean())
                s.commenting.autoDetectModels = c["autoDetectModels"].get<bool>();
            if (c.contains("cacheResults") && c["cacheResults"].is_boolean())
                s.commenting.cacheResults = c["cacheResults"].get<bool>();
        }
    }
    catch (const std::exception& e)
    {
        if (outError) *outError = std::string("Failed to parse config JSON: ") + e.what();
        // Return whatever we managed to parse plus defaults for the rest.
    }

    return s;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
bool Settings::save(const Settings& s, const std::string& path, std::string* outError)
{
    try
    {
        fs::path p(path);
        std::error_code ec;
        if (p.has_parent_path())
        {
            fs::create_directories(p.parent_path(), ec);
            // ignore ec: if dir already exists this is fine
        }

        json j;
        j["ollama"]["endpoint"]    = s.ollama.endpoint;
        j["ollama"]["modelName"]   = s.ollama.modelName;
        j["ollama"]["temperature"] = s.ollama.temperature;
        j["ollama"]["topP"]        = s.ollama.topP;
        j["ollama"]["maxTokens"]   = s.ollama.maxTokens;
        j["ollama"]["timeout"]     = s.ollama.timeoutSec;

        j["commenting"]["generateFunctionComments"] = s.commenting.generateFunctionComments;
        j["commenting"]["generateLineComments"]     = s.commenting.generateLineComments;
        j["commenting"]["commentStyle"]             = commentStyleToString(s.commenting.commentStyle);
        j["commenting"]["autoDetectModels"]         = s.commenting.autoDetectModels;
        j["commenting"]["cacheResults"]             = s.commenting.cacheResults;

        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open())
        {
            if (outError) *outError = "Could not open config file for writing: " + path;
            return false;
        }
        out << j.dump(2);
        return true;
    }
    catch (const std::exception& e)
    {
        if (outError) *outError = std::string("Failed to save config: ") + e.what();
        return false;
    }
}

} // namespace llm
} // namespace jdc
