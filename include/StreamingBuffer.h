#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>

class StreamingBuffer {
public:
    StreamingBuffer(size_t size);
    
    size_t write(const void* data, size_t bytes);
    size_t read(void* out, size_t bytes);
    
    size_t availableRead() const;
    size_t availableWrite() const;

    size_t getWritePos() const;
    size_t getReadPos() const;

    size_t getBufferedBytes() const;

private:
    std::vector<char> buffer;
    size_t head = 0;
    size_t tail = 0;
    size_t capacity;
    bool full = false;

    mutable std::mutex mutex;
    std::condition_variable dataAvailable;
};
