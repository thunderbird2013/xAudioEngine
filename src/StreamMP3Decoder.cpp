#include "StreamMP3Decoder.h"
#include <string>
#include <iostream>

StreamMP3Decoder::StreamMP3Decoder(StreamingBuffer& buffer)
    : m_buffer(buffer) {
    
    // Initialisiere drmp3 ohne eigene Config – MP3-Header liefert alles
    initialized = drmp3_init(&mp3, &StreamMP3Decoder::readCallback, &StreamMP3Decoder::seekCallback, this, nullptr);

    if (initialized) {
        sampleRate = mp3.sampleRate;
        channels = mp3.channels;
    }
}

StreamMP3Decoder::~StreamMP3Decoder() {
    if (initialized) {
        drmp3_uninit(&mp3);
    }
}

bool StreamMP3Decoder::load(const std::string& path) {
    // Nicht verwendet für Streaming
    return false;
}

size_t StreamMP3Decoder::decode(short* output, size_t framesToRead) {
    if (!initialized) return 0;

    size_t frames = drmp3_read_pcm_frames_s16(&mp3, framesToRead, output);
    std::cout << "[DEBUG] Decoded frames: " << frames << "\n";
    currentFrame += frames;
    return frames;
}

bool StreamMP3Decoder::seek(int frame) {
    // Optional implementierbar – für Netzwerkstreams meist nicht sinnvoll
    return false;
}

int StreamMP3Decoder::getSampleRate() const {
    return sampleRate;
}

int StreamMP3Decoder::getChannels() const {
    return channels;
}

uint64_t StreamMP3Decoder::getCursor() const {
    return currentFrame;
}

uint64_t StreamMP3Decoder::getTotalFrames() const {
    return 0; // Bei Streams unbekannt
}

size_t StreamMP3Decoder::getCurrentFrame() const {
    return 0;  // oder dein interner Frame-Zähler
}

size_t StreamMP3Decoder::readCallback(void* pUserData, void* pBufferOut, size_t bytesToRead) {
    StreamMP3Decoder* self = static_cast<StreamMP3Decoder*>(pUserData);
    return self->m_buffer.read(static_cast<uint8_t*>(pBufferOut), bytesToRead);
}

drmp3_bool32 StreamMP3Decoder::seekCallback(void* pUserData, int offset, drmp3_seek_origin origin) {
    (void)pUserData;
    (void)offset;
    (void)origin;
    return DRMP3_FALSE; // Seeking bei Streaming nicht unterstützt
}
