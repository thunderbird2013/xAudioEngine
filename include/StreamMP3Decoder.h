#pragma once

#include "AudioDecoder.h"
#include "dr_mp3.h"
#include "StreamingBuffer.h"

class StreamMP3Decoder : public AudioDecoder {
public:
    explicit StreamMP3Decoder(StreamingBuffer& buffer);
    ~StreamMP3Decoder();

    bool load(const std::string& path) override;
    size_t decode(short* buffer, size_t framesToRead) override;
    bool seek(int frame) override;

    int getSampleRate() const override;
    int getChannels() const override;
    uint64_t getCursor() const override;
    uint64_t getTotalFrames() const override;

private:
    StreamingBuffer& m_buffer;
    drmp3 mp3{};
    bool initialized = false;
    int sampleRate = 0;
    int channels = 0;
    uint64_t currentFrame = 0;

    static size_t readCallback(void* pUserData, void* pBufferOut, size_t bytesToRead);
    static drmp3_bool32 seekCallback(void* pUserData, int offset, drmp3_seek_origin origin);
};

