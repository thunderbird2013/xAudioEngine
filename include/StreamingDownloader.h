#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "StreamingBuffer.h"

class StreamingDownloader {
public:
    StreamingDownloader(StreamingBuffer& buffer);
    ~StreamingDownloader();

    bool start(const std::string& url);
    void stop();
    std::string getContentType() const { return contentType; }

private:
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    void downloadLoop(const std::string& url);

    void logError(const std::string& message);
    void logDebug(const std::string& message);

    StreamingBuffer& buffer;
    std::thread downloadThread;
    std::atomic<bool> running;
    std::string contentType; 

    bool headerSkipped = false;
    std::string headerBuffer;
};
