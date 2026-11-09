#pragma once

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif

#include <curl/curl.h>
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

class HttpHelper {
public:
    ~HttpHelper() = default;
    HttpHelper(const HttpHelper&) = delete;
    HttpHelper(HttpHelper&&) = delete;
    HttpHelper& operator=(const HttpHelper&) = delete;
    HttpHelper& operator=(HttpHelper&&) = delete;

    static int Get(std::string url, json& response);
    static int GetText(std::string url, std::string& response);
    static int Post(std::string url, json body, json& response);
    static int Download(std::string url, const std::filesystem::path& destination);
private:
    static constexpr long REQUEST_TIMEOUT = 15;
    static constexpr std::size_t MAX_RESPONSE_SIZE = 1024 * 1024;
    static constexpr std::size_t MAX_DOWNLOAD_SIZE = 100 * 1024 * 1024;

    struct WriteContext {
        std::string* response;
        std::size_t max_size;
    };

    struct DownloadContext {
        std::ofstream output;
        std::size_t bytes_written = 0;
    };

    HttpHelper() {};

    static HttpHelper& GetInstance()
    {
        static HttpHelper i{};
        return i;
    }

    int GetImpl(std::string url, json& response);
    int GetTextImpl(std::string url, std::string& response);
    int PostImpl(std::string url, json body, json& response);
    int DownloadImpl(std::string url, const std::filesystem::path& destination);

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t FileWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};
