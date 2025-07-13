#include "MP3Decoder.h"
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <fstream>  
#include <cstring>
#include "dr_mp3.h"

constexpr float SOFT_LIMIT_THRESHOLD = 0.9441f;

template <typename T>
T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

bool MP3Decoder::load(const std::string& path) {
    if (!drmp3_init_file(&mp3, path.c_str(), nullptr)) {
        return false;
    }
    
    initialized = true;

    drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);

    totalFrames = static_cast<uint64_t>(frameCount);

    sampleRate = mp3.sampleRate;
   
    channels = mp3.channels;
    
    bitrateKbps = 0;

    return true;
}

size_t MP3Decoder::decode(short* buffer, size_t framesToRead) {

    if (!initialized) return 0;

    //uint64_t framesDecoded = 0;

    // PCM-Daten vom Decoder holen
    size_t framesDecoded = drmp3_read_pcm_frames_s16(&mp3, framesToRead, buffer);    

     // Soft Limiter anwenden
    for (size_t i = 0; i < framesDecoded * mp3.channels; ++i) {
        float fsample = buffer[i] / 32768.0f;
       // float threshold = 0.9f;
       if (std::abs(fsample) > SOFT_LIMIT_THRESHOLD) {
             fsample = SOFT_LIMIT_THRESHOLD * (fsample > 0 ? 1.0f : -1.0f);
        }
        buffer[i] = static_cast<short>(std::clamp(fsample * 32767.0f, -32768.0f, 32767.0f));
    }

    // 👉 Hier Cursor aktualisieren:
    m_currentFrame += framesDecoded;

    return framesDecoded;
    //return drmp3_read_pcm_frames_s16(&mp3, framesToRead, buffer);    
}

bool MP3Decoder::seek(int frame) {

    bool success = drmp3_seek_to_pcm_frame(&mp3, frame) != DRMP3_FALSE;

    if (success) {

        m_currentFrame = frame; 
    
    }

    return success;

}

int MP3Decoder::getSampleRate() const {

    return initialized ? static_cast<int>(mp3.sampleRate) : 0;

}

int MP3Decoder::getChannels() const {

    return initialized ? static_cast<int>(mp3.channels) : 0;
    
}

uint64_t MP3Decoder::getCursor() const {

    return m_currentFrame; // oder wie du die Position speicherst

}

uint64_t MP3Decoder::getTotalFrames() const {

    return totalFrames; // oder wie du die Gesamtanzahl der Frames speicherst

}

int MP3Decoder::getBitrateKbps() const {
    return bitrateKbps;
}

void MP3Decoder::readID3v1Tag(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return;

    std::streamsize size = file.tellg();
    if (size < 128) return;

    file.seekg(-128, std::ios::end);
    char tag[128];
    file.read(tag, 128);

    if (std::strncmp(tag, "TAG", 3) == 0) {
        auto& t = decodedAudio.title;
        auto& a = decodedAudio.artist;
        auto& al = decodedAudio.album;

        t.assign(tag + 3, 30);
        a.assign(tag + 33, 30);
        al.assign(tag + 63, 30);

        auto trim = [](std::string& s) {
            s.erase(s.find_last_not_of(" \0", std::string::npos) + 1);
        };
        trim(t); trim(a); trim(al);
    }
}
