#pragma once

// llmConfig.h
// ----------------------------------------------------------------------------
// Configuration data structures and persistence for the local Ollama-based
// LLM integration used by C Based Decompiler.
//
// The on-disk format is JSON, stored in a per-user location:
//     Linux/macOS: $XDG_CONFIG_HOME/jdc/ollama_settings.json
//                  (falls back to $HOME/.config/jdc/ollama_settings.json)
//     Windows:     %APPDATA%\jdc\ollama_settings.json
//
// All fields have safe defaults so the application still works when the
// config file is missing or malformed.
//
// Thread safety: the structs are plain data. load()/save() are not
// thread-safe and should be called from a single thread (typically the
// GUI thread on startup/shutdown).
// ----------------------------------------------------------------------------

#include <string>

namespace jdc {
namespace llm {

struct OllamaSettings
{
    std::string endpoint   = "http://localhost:11434"; // base URL, no trailing slash
    std::string modelName  = "mistral";                // fallback model
    double      temperature = 0.7;                     // 0.0 = deterministic, 1.0 = creative
    double      topP        = 0.9;                     // nucleus sampling
    int         maxTokens   = 500;                     // num_predict in Ollama options
    int         timeoutSec  = 120;                     // request timeout (Ollama can be slow)
};

enum class CommentStyle
{
    Simple,   // one-line // comments
    Doxygen,  // /** ... */ blocks with @param/@return
    Verbose   // multi-line // comments with reasoning
};

struct CommentingSettings
{
    bool         generateFunctionComments = true;
    bool         generateLineComments     = false;
    CommentStyle commentStyle             = CommentStyle::Simple;
    bool         autoDetectModels         = true;
    bool         cacheResults             = true;
};

struct Settings
{
    OllamaSettings     ollama;
    CommentingSettings commenting;

    // Load settings from the given JSON path. If the file doesn't exist or
    // is unreadable, a default-constructed Settings is returned and
    // outError (if non-null) is populated with a description.
    static Settings load(const std::string& path, std::string* outError = nullptr);

    // Save settings to the given JSON path. Creates parent directories as
    // needed. Returns true on success.
    static bool save(const Settings& s, const std::string& path,
                     std::string* outError = nullptr);

    // Returns the platform-appropriate default config path.
    static std::string defaultConfigPath();

    // Helpers to convert CommentStyle <-> string for the JSON file.
    static const char*   commentStyleToString(CommentStyle s);
    static CommentStyle  commentStyleFromString(const std::string& s);
};

} // namespace llm
} // namespace jdc
