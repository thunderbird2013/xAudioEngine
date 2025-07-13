#pragma once
#include "AudioDecoder.h"
#include "stb_vorbis_all.h"

struct stb_vorbis;
//struct stb_vorbis_info; // forward declaration

class OggDecoder : public AudioDecoder {
public:
    OggDecoder();
    ~OggDecoder() override;

    bool load(const std::string& path) override;
    size_t decode(short* buffer, size_t framesToRead) override;
    bool seek(int frame) override;
    int getSampleRate() const override;
    uint64_t getTotalFrames() const override;
    int getChannels() const override;
    uint64_t getCursor() const override;

    const DecodedAudio& getDecodedAudio() const override { return decodedAudio; }

private:
    struct stb_vorbis* vorbis = nullptr;
    stb_vorbis_info info{};
    DecodedAudio decodedAudio;
    //bool initialized = false;
};

