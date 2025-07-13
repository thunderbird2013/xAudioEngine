#include "WavDecoder.h"
#include <iostream>
#include <fstream>   // std::ifstream
#include <string>    // std::string
#include <algorithm> // std::find
#include <cstdint>   // uint32_t usw.

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

    // Basisdaten setzen
    decodedAudio.sampleRate  = wav.sampleRate;
    decodedAudio.channels    = wav.channels;
    decodedAudio.totalFrames = wav.totalPCMFrameCount;

 // 🎯 Versuche INFO-Chunks auszulesen
    std::ifstream file(path, std::ios::binary);
    if (!file) return true; // Datei konnte nicht geöffnet werden → keine Metadaten

    file.seekg(12); // Skip "RIFF" + size + "WAVE"

    char chunkID[4];
    uint32_t chunkSize;

    while (file.read(chunkID, 4) && file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
        std::string id(chunkID, 4);

        if (id == "LIST") {
            char listType[4];
            file.read(listType, 4);
            if (std::string(listType, 4) == "INFO") {
                size_t listEnd = static_cast<size_t>(file.tellg()) + chunkSize - 4;

                while (file.tellg() < listEnd) {
                    char infoID[4];
                    uint32_t infoSize = 0;
                    if (!file.read(infoID, 4) || !file.read(reinterpret_cast<char*>(&infoSize), 4))
                        break;

                    std::string key(infoID, 4);
                    std::string value(infoSize, '\0');
                    if (!file.read(&value[0], infoSize))
                        break;

                    // Entferne mögliche Nullterminierung
                    value.erase(std::find(value.begin(), value.end(), '\0'), value.end());

                    if (key == "INAM") decodedAudio.title = value;
                    else if (key == "IART") decodedAudio.artist = value;
                    else if (key == "IPRD") decodedAudio.album = value;

                    if (infoSize % 2 == 1)
                        file.seekg(1, std::ios::cur); // Padding
                }
            } else {
                file.seekg(chunkSize - 4, std::ios::cur); // Skip andere LISTs
            }
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

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
size_t WavDecoder::getCurrentFrame() const {
    return static_cast<size_t>(getCursor());
}
