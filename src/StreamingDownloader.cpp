#include "StreamingDownloader.h"
#include "Logger.h"
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
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  //  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamingDownloader::writeCallback);
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
    size_t totalSize = size * nmemb;
    auto* self = static_cast<StreamingDownloader*>(userdata);

    if (!self->headerSkipped) {
        self->headerBuffer.append(ptr, totalSize);

        size_t headerEnd = self->headerBuffer.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            size_t startOfMP3 = headerEnd + 4;
            self->buffer.write(reinterpret_cast<const uint8_t*>(self->headerBuffer.data() + startOfMP3),
                               self->headerBuffer.size() - startOfMP3);
            self->headerSkipped = true;
        }

        return totalSize;
    } else {
        return self->buffer.write(reinterpret_cast<const uint8_t*>(ptr), totalSize);
    }
});


    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[StreamingDownloader] Download error: " << curl_easy_strerror(res) << "\n";
    }

        char* contentType = nullptr;
        CURLcode infoRes = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
        if (infoRes == CURLE_OK && contentType) {
            std::string typeStr = contentType;
            logDebug("Server Content-Type: " + typeStr);

            if (typeStr.find("audio/mpeg") == std::string::npos) {
                std::cerr << "[ERROR] Kein gültiger MP3-Stream! Content-Type: " << typeStr << "\n";
            }
        } else {
            logDebug("Server Content-Type konnte nicht ermittelt werden (ICY?)");
        }



    curl_easy_cleanup(curl);
    running = false;
}

void StreamingDownloader::logError(const std::string& message) {
    Logger::log("[ERROR] " + message);
}

void StreamingDownloader::logDebug(const std::string& message) {
    Logger::log("[DEBUG] " + message);
}
