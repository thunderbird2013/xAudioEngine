#pragma once
#include <string>
#include <memory>
#include <vector>

#include "AudioEngine.h"
#include "miniaudio.h"
#include "AudioDecoder.h"

#if defined(_WIN32)
  #if defined(AUDIOENGINE_STATIC)
    #define AUDIOENGINE_API
  #elif defined(AUDIOENGINE_EXPORTS)
    #define AUDIOENGINE_API __declspec(dllexport)
  #else
    #define AUDIOENGINE_API __declspec(dllimport)
  #endif
#else
  #define AUDIOENGINE_API
#endif

class AUDIOENGINE_API AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool loadFile(const std::string& path);
    void play();
    void stop();
    void setVolume(float volume);
    float getVolume() const;
    bool seek(int frame);
    bool isPlaying() const;
    int  getCurrentFrame() const;
    int  getSampleRate() const;
    uint64_t getTotalFrames() const;
    double getTotalTimeSeconds() const;
    double getCurrentTimeSeconds() const;


private:

    void logError(const std::string& message);
    void logDebug(const std::string& message);
    
    std::unique_ptr<AudioDecoder> decoder; // Aktueller Audio-Decoder

    float volume = 1.0f; // Lautstärke (0.0 - 1.0)
    int sampleRate = 0;
    int channels = 0;
    bool Playing = true;
    ma_device device;    // Audio-Ausgabegerät
};
