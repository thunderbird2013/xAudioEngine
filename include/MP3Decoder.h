#pragma once
#include "AudioDecoder.h"
#include "dr_mp3.h" 

class MP3Decoder : public AudioDecoder {
public:
    bool load(const std::string& path) override;
    size_t decode(short* buffer, size_t framesToRead) override;
    bool seek(int frame) override;
    int getSampleRate() const override;
    int getChannels() const override;
    uint64_t getCursor() const override;
    uint64_t getTotalFrames() const override;
    size_t getCurrentFrame() const override;
    int getBitrateKbps() const;
    const DecodedAudio& getDecodedAudio() const override { return decodedAudio; }

private:
    drmp3 mp3{};
    DecodedAudio decodedAudio;
    bool initialized = false;
    uint64_t m_currentFrame = 0;
    uint64_t totalFrames = 0;
    int sampleRate = 0;
    int channels = 0;
    int bitrateKbps = 0; 
    std::string title, artist, album;
    void readID3v1Tag(const std::string& path);
};
