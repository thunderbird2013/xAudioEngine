#include "WavDecoder.h"

WavDecoder::WavDecoder() {}

WavDecoder::~WavDecoder() {
    if (initialized) {
        drwav_uninit(&wav);
    }
}

bool WavDecoder::load(const std::string& path) {
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        initialized = false;
        return false;
    }
    initialized = true;
    currentFrame = 0;
    return true;
}

size_t WavDecoder::decode(short* buffer, size_t framesToRead) {
    if (!initialized) return 0;
    size_t framesDecoded = drwav_read_pcm_frames_s16(&wav, framesToRead, buffer);
    currentFrame += framesDecoded;
    return framesDecoded;
}

bool WavDecoder::seek(int frame) {
    if (!initialized) return false;
    bool result = drwav_seek_to_pcm_frame(&wav, frame);
    if (result) currentFrame = frame;
    return result;
}

int WavDecoder::getSampleRate() const {
    return static_cast<int>(wav.sampleRate);
}

int WavDecoder::getChannels() const {
    return wav.channels;
}

uint64_t WavDecoder::getCursor() const {
    return currentFrame;
}

uint64_t WavDecoder::getTotalFrames() const {
    return wav.totalPCMFrameCount;
}
