#include "AudioEngine.h"
#include "MP3Decoder.h"
#include "FlacDecoder.h"
#include "OggDecoder.h"
#include "WavDecoder.h"
#include "Logger.h"
#include "StreamingBuffer.h"
#include "StreamMP3Decoder.h"
#include <filesystem>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <memory>
#include <curl/curl.h>


AudioEngine::AudioEngine() {
    logDebug("xAudioEngine Copyright (c) 2025 by Matthias Stoltze");
    logDebug("AudioEngine erstellt.");
    logDebug("libcurl Version: " + std::string(curl_version()));
    curl_global_init(CURL_GLOBAL_DEFAULT);  // Initialisiere libcurl    
}

AudioEngine::~AudioEngine() {
    ma_device_uninit(&device);
    logDebug("AudioEngine zerstört und Gerät freigegeben.");
}

bool AudioEngine::loadFile(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    logDebug("Datei wird geladen: " + path);

    if (ext == ".mp3") {
        decoder = std::make_unique<MP3Decoder>();
        logDebug("MP3Decoder ausgewählt.");
    } else if (ext == ".flac") {
        decoder = std::make_unique<FlacDecoder>();
        logDebug("FlacDecoder ausgewählt.");
    } else if (ext == ".ogg") {
        decoder = std::make_unique<OggDecoder>();
        logDebug("OggDecoder ausgewählt.");
    } else if (ext == ".wav") {
        decoder = std::make_unique<WavDecoder>();
        logDebug("WavDecoder ausgewählt.");    
    } else {
        logError("Nicht unterstützte Dateierweiterung: " + ext);
        return false;
    }

    if (!decoder->load(path)) {
        logError("Decoder konnte Datei nicht laden: " + path);
        return false;
    }

    sampleRate = decoder->getSampleRate();
    channels = decoder->getChannels();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = [](ma_device* device, void* output, const void*, ma_uint32 frameCount) 
    {
        AudioEngine* engine = static_cast<AudioEngine*>(device->pUserData);
        short* out = static_cast<short*>(output);

        size_t framesRead = engine->decoder->decode(out, frameCount);
        if (framesRead < frameCount) {
            std::memset(out + framesRead * engine->channels, 0,
                         (frameCount - framesRead) * engine->channels * sizeof(short));
        }

        if (framesRead == 0) {
            engine->Playing = false;            
        }

        
    };
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        logError("miniaudio-Gerät konnte nicht initialisiert werden.");
        return false;
    }

    logDebug("miniaudio-Gerät erfolgreich initialisiert.");
    return true;
}

bool AudioEngine::loadURL(const std::string& url) {

    //if (ma_device_get_state(&device) == ma_device_state_started) {
    //    stop();
    //    logDebug("Audiowiedergabe gestoppt, um URL zu laden.");
   // }

    streamBuffer = std::make_shared<StreamingBuffer>(512 * 1024);

    std::thread([url, buffer = streamBuffer]() {
        CURL* curl = curl_easy_init();
        if (!curl) return;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char* ptr, size_t size, size_t nmemb, void* userdata) -> 
        size_t {
            auto* buf = static_cast<StreamingBuffer*>(userdata);
            return buf->write(reinterpret_cast<uint8_t*>(ptr), size * nmemb);
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer.get());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);          
    }).detach();

    while (streamBuffer->getBufferedBytes() < 16 * 1024) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    decoder = std::make_unique<StreamMP3Decoder>(*streamBuffer);
    if (!decoder) {
        logError("StreamMP3Decoder konnte nicht erzeugt werden.");
        return false;
    }

    sampleRate = decoder->getSampleRate();
    channels = decoder->getChannels();

    if (decoder->getSampleRate() == 0 || decoder->getChannels() == 0) {
    logError("Ungültiger MP3-Stream – Decoder konnte keine gültigen Parameter lesen.");
    return false;
}

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = [](ma_device* device, void* output, const void*, ma_uint32 frameCount) {
        auto* engine = static_cast<AudioEngine*>(device->pUserData);
        short* out = static_cast<short*>(output);
        size_t framesRead = engine->decoder->decode(out, frameCount);
        if (framesRead < frameCount) {
            std::memset(out + framesRead * engine->channels, 0, (frameCount - framesRead) * engine->channels * sizeof(short));
        }
    };
    config.pUserData = this;

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        logError("Fehler beim Initialisieren des Audiogeräts.");
        return false;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        logError("Audiogerät konnte nicht gestartet werden.");
        return false;
    }
    return true;
}



void AudioEngine::play() {
    if (ma_device_start(&device) != MA_SUCCESS) {
        logError("Audiowiedergabe konnte nicht gestartet werden.");
    } else {
        logDebug("Audiowiedergabe gestartet.");
    }
    
     Playing = true;
}

bool AudioEngine::isPlaying() const {   
    return Playing ; // Hier wird der Status der Wiedergabe zurückgegeben
}

void AudioEngine::stop() {
    if (ma_device_stop(&device) != MA_SUCCESS) {
        logError("Audiowiedergabe konnte nicht gestoppt werden.");
    } else {
        logDebug("Audiowiedergabe gestoppt.");
    }
    Playing = false;
}

void AudioEngine::setVolume(float volume) {
    ma_device_set_master_volume(&device, volume);
    logDebug("Lautstärke gesetzt: " + std::to_string(volume));
}

bool AudioEngine::seek(int frame) {
    if (decoder && decoder->seek(frame)) {
        logDebug("Audio auf Frame " + std::to_string(frame) + " gesetzt.");
        return true;
    }
    logError("Seeking fehlgeschlagen.");
    return false;
}


void AudioEngine::logError(const std::string& message) {
    Logger::log("[ERROR] " + message);
}

void AudioEngine::logDebug(const std::string& message) {
    Logger::log("[DEBUG] " + message);
}

int AudioEngine::getCurrentFrame() const {
    if (decoder) {
        return static_cast<int>(decoder->getCursor());
    }
    return 0;
}

int AudioEngine::getSampleRate() const {
    return decoder ? decoder->getSampleRate() : 0;
}

uint64_t AudioEngine::getTotalFrames() const {
    return decoder ? decoder->getTotalFrames() : 0;
}

double AudioEngine::getTotalTimeSeconds() const {
    auto sr = getSampleRate();
    return sr > 0 ? static_cast<double>(getTotalFrames()) / sr : 0.0;
}

float AudioEngine::getVolume() const {
    return volume;
}

double AudioEngine::getCurrentTimeSeconds() const {
    if (getSampleRate() == 0) return 0.0;
    return static_cast<double>(decoder->getCursor()) / getSampleRate();
}



