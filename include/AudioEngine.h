//
// AudioEngine
// -----------
// Die AudioEngine-Klasse ist das zentrale Steuerungsmodul zur Audiowiedergabe. 
// Sie kapselt ein Ausgabegerät (ma_device), verwaltet den AudioDecoder und 
// steuert Lautstärke, Wiedergabe, Position und Streaming.
//
// Die Engine unterstützt verschiedene Audioformate über eine einheitliche Decoder-Schnittstelle.
// Audio wird gestreamt und verarbeitet über miniaudio (https://miniaud.io).
//
// Öffentliche Methoden:
// - loadFile(...)               Lädt und initialisiert eine Audiodatei über einen Decoder.
// - play(), stop()              Startet bzw. stoppt die Wiedergabe.
// - setVolume(...), getVolume() Setzt oder liest die aktuelle Lautstärke (0.0 - 1.0).
// - seek(...)                   Springt zu einer bestimmten Frame-Position im Stream.
// - isPlaying()                 Gibt zurück, ob Audio aktiv abgespielt wird.
// - getCurrentFrame()           Liefert die aktuelle Abspielposition in Frames.
// - getSampleRate()             Gibt die Sample-Rate der aktuellen Datei zurück.
// - getTotalFrames()            Gesamtanzahl der PCM-Frames im Stream.
// - getTotalTimeSeconds()       Wiedergabedauer in Sekunden.
// - getCurrentTimeSeconds()     Aktuelle Wiedergabezeit in Sekunden.
//
// Private Member:
// - decoder                     Der aktuelle Decoder (z. B. MP3, OGG, FLAC...)
// - device                      Das miniaudio-Gerät zur Audiowiedergabe.
// - volume                      Lautstärkewert (0.0 bis 1.0).
// - sampleRate, channels        Technische Metadaten des geladenen Audios.
// - Playing                     Wiedergabestatus-Flag.
//
// Copyright (C) 2025 by the Audio_Core project contributors.
//

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <memory>

#include "AudioEngine.h"
#include "miniaudio.h"
#include "AudioDecoder.h"
#include "StreamingBuffer.h"
#include "StreamingDownloader.h"


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
    bool loadURL(const std::string& url);
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
    std::shared_ptr<StreamingBuffer> streamBuffer; 
    std::unique_ptr<StreamingDownloader> downloader;


    float volume = 1.0f; // Lautstärke (0.0 - 1.0)
    int sampleRate = 0;
    int channels = 0;
    bool Playing = true;
    ma_device device;    // Audio-Ausgabegerät
};
