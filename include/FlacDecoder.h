#pragma once
#include "AudioDecoder.h"
#include "dr_flac.h"

class FlacDecoder : public AudioDecoder {
public:
    bool load(const std::string& path) override;
    size_t decode(short* buffer, size_t framesToRead) override;
    bool seek(int frame) override;
    int getSampleRate() const override;
    int getChannels() const override;
    uint64_t getCursor() const override;
    uint64_t getTotalFrames() const override; 
    size_t getCurrentFrame() const override;
    const DecodedAudio& getDecodedAudio() const override { return decodedAudio; }

private:
    drflac* m_flac = nullptr;
    DecodedAudio decodedAudio;
};