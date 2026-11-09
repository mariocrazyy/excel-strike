#include "HttpHelper.hpp"

int HttpHelper::Get(std::string url, json& response) {
	return GetInstance().GetImpl(url, response);
}

int HttpHelper::GetText(std::string url, std::string& response) {
	return GetInstance().GetTextImpl(url, response);
}

int HttpHelper::Post(std::string url, json body, json& response) {
	return GetInstance().PostImpl(url, body, response);
}

int HttpHelper::Download(std::string url, const std::filesystem::path& destination) {
	return GetInstance().DownloadImpl(url, destination);
}

int HttpHelper::GetImpl(std::string url, json& response) {
    std::string response_string;
    int http_status = GetTextImpl(std::move(url), response_string);
    if (http_status < 0)
        return http_status;

    try {
        response = json::parse(response_string);
    }
    catch (...) {
        return -2;
    }

    return http_status;
}

int HttpHelper::GetTextImpl(std::string url, std::string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    response.clear();
    WriteContext context{ &response, MAX_RESPONSE_SIZE };
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Excel-Strike-Updater/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOGF(WARNING, "HTTP GET failed: {} (curl error {})", curl_easy_strerror(res), static_cast<int>(res));
        return -1;
    }

    return static_cast<int>(http_code);
}

int HttpHelper::DownloadImpl(std::string url, const std::filesystem::path& destination) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    DownloadContext context;
    context.output.open(destination, std::ios::binary | std::ios::trunc);
    if (!context.output.is_open()) {
        curl_easy_cleanup(curl);
        LOGF(WARNING, "Could not open update file for writing");
        return -2;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Excel-Strike-Updater/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode result = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    context.output.close();
    if (result != CURLE_OK || !context.output) {
        LOGF(WARNING, "Update download failed: {}", curl_easy_strerror(result));
        std::error_code error;
        std::filesystem::remove(destination, error);
        return -1;
    }

    if (http_code != 200) {
        std::error_code error;
        std::filesystem::remove(destination, error);
    }

    return static_cast<int>(http_code);
}

int HttpHelper::PostImpl(std::string url, json body, json& response) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    std::string response_string;
    WriteContext context{ &response_string, MAX_RESPONSE_SIZE };
    std::string body_string = body.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_string.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body_string.size());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIMEOUT);

    // Set JSON header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return -1;

    try {
        response = json::parse(response_string);
    }
    catch (...) {
        return -2; // parse error
    }

    return static_cast<int>(http_code);
}

size_t HttpHelper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* context = static_cast<WriteContext*>(userp);
    const size_t total_size = size * nmemb;
    if (total_size > context->max_size - context->response->size())
        return 0;

    context->response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

size_t HttpHelper::FileWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* context = static_cast<DownloadContext*>(userp);
    const size_t total_size = size * nmemb;
    if (total_size > MAX_DOWNLOAD_SIZE - context->bytes_written)
        return 0;

    context->output.write(static_cast<const char*>(contents), static_cast<std::streamsize>(total_size));
    if (!context->output)
        return 0;

    context->bytes_written += total_size;
    return total_size;
}