// ollama_interface.cpp
// ----------------------------------------------------------------------------
// libcurl-based implementation of OllamaInterface.
//
// Notes on implementation choices:
//
//  - We initialise libcurl globally exactly once per process using a
//    function-local static, so the user doesn't have to call curl_global_init
//    explicitly. libcurl's global state is thread-hostile during init/cleanup
//    but fine to use afterwards from multiple threads as long as each thread
//    uses its own easy handle. We create a fresh easy handle per request,
//    which keeps the code simple at the cost of skipping connection reuse.
//    That cost is negligible because Ollama generation latency dwarfs HTTP
//    setup time.
//
//  - Cancellation uses CURLOPT_XFERINFOFUNCTION: returning non-zero causes
//    libcurl to abort the transfer. The atomic flag is polled on every
//    progress tick.
//
//  - Pull is implemented as a streaming POST. Ollama emits one JSON object
//    per line, so we accumulate bytes in a buffer and split on '\n'. We
//    don't try to parse partial JSON.
// ----------------------------------------------------------------------------

#include "ollama_interface.h"

#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace jdc {
namespace llm {

// ---------------------------------------------------------------------------
// One-shot global init
// ---------------------------------------------------------------------------
namespace {

struct CurlGlobal
{
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

void ensureCurlInit()
{
    static CurlGlobal g;
    (void)g;
}

// libcurl write callback: appends to std::string
size_t writeToString(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    const size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userdata);
    out->append(static_cast<const char*>(ptr), total);
    return total;
}

// Streaming write callback. Splits incoming bytes on '\n' and forwards
// complete lines to the callback. Returns 0 to abort on callback failure.
struct StreamCtx
{
    std::string                                                buffer;
    std::function<bool(const std::string&)>                    onLine;
    std::atomic<bool>*                                         cancelFlag;
    bool                                                       aborted = false;
};

size_t writeStreaming(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    const size_t total = size * nmemb;
    auto* ctx = static_cast<StreamCtx*>(userdata);

    if (ctx->cancelFlag && ctx->cancelFlag->load())
    {
        ctx->aborted = true;
        return 0; // abort transfer
    }

    ctx->buffer.append(static_cast<const char*>(ptr), total);

    // Process complete lines
    size_t pos;
    while ((pos = ctx->buffer.find('\n')) != std::string::npos)
    {
        std::string line = ctx->buffer.substr(0, pos);
        ctx->buffer.erase(0, pos + 1);
        if (line.empty()) continue;
        if (ctx->onLine && !ctx->onLine(line))
        {
            ctx->aborted = true;
            return 0;
        }
    }
    return total;
}

// Progress callback used purely for cooperative cancellation.
int progressCancel(void* clientp,
                   curl_off_t /*dltotal*/, curl_off_t /*dlnow*/,
                   curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
{
    auto* flag = static_cast<std::atomic<bool>*>(clientp);
    return (flag && flag->load()) ? 1 : 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// OllamaInterface
// ---------------------------------------------------------------------------
OllamaInterface::OllamaInterface(const std::string& endpoint)
    : m_endpoint(endpoint)
{
    ensureCurlInit();
    // Strip trailing slash for clean URL concatenation
    while (!m_endpoint.empty() && m_endpoint.back() == '/')
        m_endpoint.pop_back();
}

OllamaInterface::~OllamaInterface() = default;

void OllamaInterface::setEndpoint(const std::string& endpoint)
{
    m_endpoint = endpoint;
    while (!m_endpoint.empty() && m_endpoint.back() == '/')
        m_endpoint.pop_back();
}

void OllamaInterface::cancel()  { m_cancelRequested.store(true);  }
void OllamaInterface::reset()   { m_cancelRequested.store(false); m_lastError.clear(); }
void OllamaInterface::setError(const std::string& m) { m_lastError = m; }

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------
std::string OllamaInterface::httpPost(const std::string& url,
                                      const std::string& body,
                                      long timeoutSec,
                                      int* httpStatus)
{
    if (httpStatus) *httpStatus = 0;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        setError("curl_easy_init failed");
        return {};
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, std::min<long>(timeoutSec, 10));
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L); // safe in multithreaded apps
    // Cancellation
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,         0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,   progressCancel);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA,       &m_cancelRequested);

    CURLcode res = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (httpStatus) *httpStatus = static_cast<int>(status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            setError("Request cancelled by user");
        else
            setError(std::string("HTTP POST failed: ") + curl_easy_strerror(res));
        return {};
    }

    return response;
}

std::string OllamaInterface::httpGet(const std::string& url,
                                     long timeoutSec,
                                     int* httpStatus)
{
    if (httpStatus) *httpStatus = 0;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        setError("curl_easy_init failed");
        return {};
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET,        1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, std::min<long>(timeoutSec, 10));
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,         0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,   progressCancel);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA,       &m_cancelRequested);

    CURLcode res = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (httpStatus) *httpStatus = static_cast<int>(status);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            setError("Request cancelled by user");
        else
            setError(std::string("HTTP GET failed: ") + curl_easy_strerror(res));
        return {};
    }
    return response;
}

bool OllamaInterface::httpPostStream(const std::string& url,
                                     const std::string& body,
                                     long timeoutSec,
                                     const std::function<bool(const std::string&)>& onChunk)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        setError("curl_easy_init failed");
        return false;
    }

    StreamCtx ctx;
    ctx.onLine     = onChunk;
    ctx.cancelFlag = &m_cancelRequested;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  writeStreaming);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, std::min<long>(timeoutSec, 10));
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);

    CURLcode res = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    // Flush any trailing line that didn't end in \n
    if (!ctx.buffer.empty() && !ctx.aborted && ctx.onLine)
    {
        ctx.onLine(ctx.buffer);
        ctx.buffer.clear();
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        if (ctx.aborted || res == CURLE_WRITE_ERROR)
            setError(m_cancelRequested.load() ? "Request cancelled by user"
                                              : "Streaming callback aborted");
        else
            setError(std::string("HTTP streaming POST failed: ") + curl_easy_strerror(res));
        return false;
    }
    if (status < 200 || status >= 300)
    {
        setError("HTTP status " + std::to_string(status));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
bool OllamaInterface::isServiceRunning()
{
    clearLastError();
    int status = 0;
    // Ollama's root returns "Ollama is running" 200 by default.
    std::string body = httpGet(m_endpoint + "/", 5, &status);
    if (status >= 200 && status < 500) // even a 404 means SOMETHING is there
    {
        // 200 is the clean case; we still consider 4xx as "service running but
        // perhaps different version". 5xx and transport failures are treated
        // as not-running.
        return status >= 200 && status < 500;
    }
    return false;
}

std::vector<ModelInfo> OllamaInterface::listModels()
{
    clearLastError();
    std::vector<ModelInfo> out;

    int status = 0;
    std::string body = httpGet(m_endpoint + "/api/tags", 10, &status);
    if (status < 200 || status >= 300)
    {
        if (m_lastError.empty())
            setError("Failed to list models: HTTP " + std::to_string(status));
        return out;
    }

    try
    {
        json j = json::parse(body);
        if (j.contains("models") && j["models"].is_array())
        {
            for (const auto& m : j["models"])
            {
                ModelInfo info;
                if (m.contains("name") && m["name"].is_string())
                    info.name = m["name"].get<std::string>();
                if (m.contains("modified_at") && m["modified_at"].is_string())
                    info.modifiedAt = m["modified_at"].get<std::string>();
                if (m.contains("size") && m["size"].is_number())
                    info.sizeBytes = m["size"].get<long long>();
                if (!info.name.empty())
                    out.push_back(std::move(info));
            }
        }
    }
    catch (const std::exception& e)
    {
        setError(std::string("Failed to parse /api/tags response: ") + e.what());
    }
    return out;
}

bool OllamaInterface::pullModel(const std::string& modelName,
                                const std::function<void(const PullProgress&)>& progressCb)
{
    clearLastError();
    if (modelName.empty())
    {
        setError("pullModel: model name is empty");
        return false;
    }

    json req;
    req["name"]   = modelName;
    req["stream"] = true;

    bool sawSuccess = false;
    std::string errorFromServer;

    auto onLine = [&](const std::string& line) -> bool
    {
        try
        {
            json chunk = json::parse(line);
            PullProgress p;
            if (chunk.contains("status") && chunk["status"].is_string())
                p.status = chunk["status"].get<std::string>();
            if (chunk.contains("completed") && chunk["completed"].is_number())
                p.completed = chunk["completed"].get<long long>();
            if (chunk.contains("total") && chunk["total"].is_number())
                p.total = chunk["total"].get<long long>();
            if (p.status == "success") { sawSuccess = true; p.done = true; }

            if (chunk.contains("error") && chunk["error"].is_string())
                errorFromServer = chunk["error"].get<std::string>();

            if (progressCb) progressCb(p);
        }
        catch (...)
        {
            // Ignore unparseable lines (e.g. blank keep-alive).
        }
        // Continue streaming
        return true;
    };

    // Pulling a model can take a very long time on first download. Use a
    // generous timeout - 30 minutes - but still let the cancel flag interrupt.
    bool ok = httpPostStream(m_endpoint + "/api/pull", req.dump(),
                             /*timeoutSec=*/1800, onLine);

    if (!errorFromServer.empty())
    {
        setError("Ollama pull error: " + errorFromServer);
        return false;
    }
    if (!ok) return false;
    if (!sawSuccess)
    {
        setError("Pull completed without success marker");
        return false;
    }
    return true;
}

std::string OllamaInterface::generate(const std::string& model,
                                      const std::string& prompt,
                                      const GenerateOptions& opts)
{
    clearLastError();

    if (model.empty()) { setError("generate: model is empty");   return {}; }
    if (prompt.empty()){ setError("generate: prompt is empty");  return {}; }

    json req;
    req["model"]  = model;
    req["prompt"] = prompt;
    req["stream"] = false;
    req["options"]["temperature"] = opts.temperature;
    req["options"]["top_p"]       = opts.topP;
    req["options"]["num_predict"] = opts.maxTokens;
    if (!opts.system.empty()) req["system"] = opts.system;

    int status = 0;
    std::string body = httpPost(m_endpoint + "/api/generate", req.dump(),
                                opts.timeoutSec, &status);
    if (body.empty())
        return {};

    if (status < 200 || status >= 300)
    {
        // Try to extract a server-side error
        try
        {
            json j = json::parse(body);
            if (j.contains("error") && j["error"].is_string())
            {
                setError("Ollama error: " + j["error"].get<std::string>());
                return {};
            }
        }
        catch (...) { /* fall through */ }
        setError("Ollama HTTP " + std::to_string(status));
        return {};
    }

    try
    {
        json j = json::parse(body);
        if (j.contains("response") && j["response"].is_string())
            return j["response"].get<std::string>();
        if (j.contains("error") && j["error"].is_string())
        {
            setError("Ollama error: " + j["error"].get<std::string>());
            return {};
        }
        setError("Ollama response had no 'response' field");
    }
    catch (const std::exception& e)
    {
        setError(std::string("Failed to parse /api/generate response: ") + e.what());
    }
    return {};
}

std::string OllamaInterface::makeRequest(const std::string& model,
                                         const std::string& prompt,
                                         int maxTokens,
                                         double temperature)
{
    GenerateOptions opts;
    opts.maxTokens   = maxTokens;
    opts.temperature = temperature;
    return generate(model, prompt, opts);
}

} // namespace llm
} // namespace jdc
