#include "StreamingBuffer.h"
#include <cstring>

StreamingBuffer::StreamingBuffer(size_t size)
    : buffer(size), capacity(size) {}

size_t StreamingBuffer::availableRead() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (full) return capacity;
    if (head >= tail) return head - tail;
    return capacity - (tail - head);
}

size_t StreamingBuffer::availableWrite() const {
    std::lock_guard<std::mutex> lock(mutex);
    return capacity - availableRead();
}

size_t StreamingBuffer::write(const void* data, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex);
    size_t space = availableWrite();
    size_t toWrite = std::min(bytes, space);

    for (size_t i = 0; i < toWrite; ++i) {
        buffer[head] = ((char*)data)[i];
        head = (head + 1) % capacity;
        if (head == tail) full = true;
    }

    dataAvailable.notify_all();
    return toWrite;
}

size_t StreamingBuffer::read(void* out, size_t bytes) {
    std::unique_lock<std::mutex> lock(mutex);
    dataAvailable.wait(lock, [&] { return availableRead() > 0; });

    size_t toRead = std::min(bytes, availableRead());

    for (size_t i = 0; i < toRead; ++i) {
        ((char*)out)[i] = buffer[tail];
        tail = (tail + 1) % capacity;
        full = false;
    }

    return toRead;
}

size_t StreamingBuffer::getWritePos() const {
   std::lock_guard<std::mutex> lock(mutex);
   return head;
}

size_t StreamingBuffer::getReadPos() const {
    std::lock_guard<std::mutex> lock(mutex);
    return tail;
}

size_t StreamingBuffer::getBufferedBytes() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (full) return capacity;
    if (head >= tail) {
        return head - tail;
    } else {
        return capacity - (tail - head);
    }
}
