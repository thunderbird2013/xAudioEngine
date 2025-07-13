#pragma once
#include "AudioDecoder.h"
#include <cstdint>
#include <dr_wav.h>

class WavDecoder : public AudioDecoder {
public:
    WavDecoder();
    ~WavDecoder();

    bool load(const std::string& path) override;
    size_t decode(short* buffer, size_t framesToRead) override;
    bool seek(int frame) override;
    int getSampleRate() const override;
    int getChannels() const override;
    uint64_t getCursor() const override;
    uint64_t getTotalFrames() const override;

    const DecodedAudio& getDecodedAudio() const override { return decodedAudio; }

private:
    drwav wav;
    DecodedAudio decodedAudio;
    bool initialized = false;
    uint64_t currentFrame = 0;
};
