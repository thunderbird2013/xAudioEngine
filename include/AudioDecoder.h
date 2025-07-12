// AudioDecoder Interface
// This file defines the interface for audio decoders in the Audio_Core project.
// It includes methods for loading audio files, decoding audio data, seeking within the audio stream, and retrieving metadata such as sample rate and channel count.
// Copyright (C) 2025 by the Audio_Core project contributors.
//

#pragma once    
#include <string>
#include <vector>

struct DecodedAudio {
    std::vector<short> pcm;
    int sampleRate = 0;
    int channels = 0;
    int totalFrames = 0;
    std::string title, artist, album;
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;
    virtual bool load(const std::string& path) = 0;
    virtual size_t decode(short* buffer, size_t framesToRead) = 0;
    virtual bool seek(int frame) = 0;
    virtual int getSampleRate() const = 0;
    virtual int getChannels() const = 0;
    virtual uint64_t getCursor() const = 0;
    virtual uint64_t getTotalFrames() const = 0;
};