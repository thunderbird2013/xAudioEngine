#include "StreamingDownloader.h"
#include <curl/curl.h>
#include <iostream>

StreamingDownloader::StreamingDownloader(StreamingBuffer& buf)
    : buffer(buf), running(false) {}

StreamingDownloader::~StreamingDownloader() {
    stop();
}

bool StreamingDownloader::start(const std::string& url) {
    if (running) return false;
    running = true;

    downloadThread = std::thread(&StreamingDownloader::downloadLoop, this, url);
    return true;
}

void StreamingDownloader::stop() {
    running = false;
    if (downloadThread.joinable()) {
        downloadThread.join();
    }
}

size_t StreamingDownloader::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* self = static_cast<StreamingDownloader*>(userdata);
    size_t totalBytes = size * nmemb;
    self->buffer.write(reinterpret_cast<const uint8_t*>(ptr), totalBytes);
    return totalBytes;
}

void StreamingDownloader::downloadLoop(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[StreamingDownloader] curl_easy_init() failed!\n";
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamingDownloader::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[StreamingDownloader] Download error: " << curl_easy_strerror(res) << "\n";
    }

    curl_easy_cleanup(curl);
    running = false;
}
