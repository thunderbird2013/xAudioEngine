//
// AudioDecoder Interface
// -----------------------
// Dieses Interface definiert eine einheitliche Abstraktion für verschiedene Audio-Decoder, 
// die innerhalb der Audio_Core-Engine verwendet werden. Es ermöglicht das Laden, Dekodieren, 
// Spulen und Abrufen von Metadaten beliebiger Audioformate.
//
// Implementierungen dieses Interfaces kapseln die format-spezifische Logik für MP3, OGG, FLAC, WAV usw.,
// und liefern PCM-Daten an die AudioEngine zur weiteren Verarbeitung und Wiedergabe.
//
// Methodenübersicht:
// - load(...)          Lädt und initialisiert eine Audiodatei.
// - decode(...)        Liest und dekodiert eine bestimmte Anzahl von PCM-Frames.
// - seek(...)          Springt zu einem bestimmten Frame innerhalb des Audiostreams.
// - getSampleRate()    Gibt die Sample-Rate (Hz) des geladenen Audiomaterials zurück.
// - getChannels()      Gibt die Anzahl der Audiokanäle zurück (z.B. Mono, Stereo).
// - getCursor()        Gibt die aktuelle Abspielposition (in Frames) zurück.
// - getTotalFrames()   Gibt die Gesamtanzahl an Frames in der Datei zurück.
//
// Copyright (C) 2025 by the xAudioEngine project contributors.
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
    virtual             ~AudioDecoder() = default;
    virtual bool        load(const std::string& path) = 0;
    virtual size_t      decode(short* buffer, size_t framesToRead) = 0;
    virtual bool        seek(int frame) = 0;
    virtual int         getSampleRate() const = 0;
    virtual int         getChannels() const = 0;
    virtual size_t      getCurrentFrame() const = 0;
    virtual uint64_t    getCursor() const = 0;
    virtual uint64_t    getTotalFrames() const = 0;
    virtual bool        isFinished() const {
        return getCurrentFrame() >= getTotalFrames();
    }

    virtual const DecodedAudio& getDecodedAudio() const {
        static DecodedAudio empty;
    return empty;
    }

    
};