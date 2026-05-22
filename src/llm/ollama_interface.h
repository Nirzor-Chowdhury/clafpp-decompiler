#pragma once

// ollama_interface.h
// ----------------------------------------------------------------------------
// Low-level wrapper around a local Ollama HTTP API.
//
// Responsibilities:
//   * One-shot text generation (POST /api/generate, stream=false)
//   * Listing locally available models (GET /api/tags)
//   * Pulling a model with progress callbacks (POST /api/pull, streaming)
//   * Health check (GET / on the root)
//   * Cancellation of in-flight requests
//
// This class deliberately does NOT depend on wxWidgets so it can be used
// from worker threads or from CLI contexts.
//
// Thread safety:
//   * A single OllamaInterface instance is safe to use from one thread at a
//     time. The cancellation flag (cancel()) IS safe to call from another
//     thread while a request is in flight on a different thread - that is
//     its whole purpose.
//   * For concurrent requests from multiple threads, construct one
//     OllamaInterface per thread.
// ----------------------------------------------------------------------------

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace jdc {
namespace llm {

struct GenerateOptions
{
    int    maxTokens   = 500;    // maps to options.num_predict
    double temperature = 0.7;
    double topP        = 0.9;
    int    timeoutSec  = 120;
    // System prompt prepended on the server side. Empty -> not sent.
    std::string system;
};

struct ModelInfo
{
    std::string name;        // e.g. "mistral:latest"
    std::string modifiedAt;  // ISO-8601 string from Ollama
    long long   sizeBytes = 0;
};

struct PullProgress
{
    std::string status;          // e.g. "downloading", "verifying sha256 digest"
    long long   completed = 0;
    long long   total     = 0;
    bool        done      = false;
};

class OllamaInterface
{
public:
    explicit OllamaInterface(const std::string& endpoint = "http://localhost:11434");
    ~OllamaInterface();

    OllamaInterface(const OllamaInterface&)            = delete;
    OllamaInterface& operator=(const OllamaInterface&) = delete;

    // ---- Configuration -----------------------------------------------------
    void setEndpoint(const std::string& endpoint);
    const std::string& getEndpoint() const { return m_endpoint; }

    // ---- Health & discovery -----------------------------------------------
    // Returns true if Ollama responds on the configured endpoint within a
    // short timeout (5 s, independent of the main timeout).
    bool isServiceRunning();

    // GET /api/tags. On failure returns an empty vector and sets lastError.
    std::vector<ModelInfo> listModels();

    // POST /api/pull with stream=true. Calls progressCb on every progress
    // chunk it receives. Returns true if the model was pulled successfully.
    // progressCb may be empty.
    bool pullModel(const std::string& modelName,
                   const std::function<void(const PullProgress&)>& progressCb = {});

    // ---- Generation --------------------------------------------------------
    // POST /api/generate with stream=false. Returns the generated text on
    // success or an empty string on failure (check getLastError()).
    std::string generate(const std::string& model,
                         const std::string& prompt,
                         const GenerateOptions& opts = {});

    // Convenience overload matching the spec's signature.
    std::string makeRequest(const std::string& model,
                            const std::string& prompt,
                            int maxTokens = 500,
                            double temperature = 0.7);

    // ---- Cancellation ------------------------------------------------------
    // Cancel any in-flight HTTP request. Safe to call from any thread.
    // After calling cancel(), call reset() before issuing another request.
    void cancel();
    void reset();
    bool isCancelled() const { return m_cancelRequested.load(); }

    // ---- Errors ------------------------------------------------------------
    const std::string& getLastError() const { return m_lastError; }
    void clearLastError() { m_lastError.clear(); }

private:
    // Low-level HTTP helpers. Return body string; sets m_lastError on error.
    // httpStatus is filled in (0 on transport failure).
    std::string httpPost(const std::string& url,
                         const std::string& jsonBody,
                         long timeoutSec,
                         int* httpStatus);

    std::string httpGet(const std::string& url,
                        long timeoutSec,
                        int* httpStatus);

    // Streaming POST: each newline-delimited JSON chunk is delivered to
    // onChunk. Returns true on a clean 2xx termination.
    bool httpPostStream(const std::string& url,
                        const std::string& jsonBody,
                        long timeoutSec,
                        const std::function<bool(const std::string& jsonLine)>& onChunk);

    void setError(const std::string& msg);

    std::string       m_endpoint;
    std::string       m_lastError;
    std::atomic<bool> m_cancelRequested{false};
};

} // namespace llm
} // namespace jdc
