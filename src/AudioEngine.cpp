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
#include <cassert>
#include <sstream>
#include <aubio/aubio.h>

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

    lastFilePath = path; // Speichere den Pfad für spätere BPM-Analyse

    if (!decoder->load(path)) {
        logError("Decoder konnte Datei nicht laden: " + path);
        return false;
    }

    // Nur Parameter setzen, noch kein Gerät initialisieren!
    sampleRate = decoder->getSampleRate();
    channels   = decoder->getChannels();

    return true;
}

bool AudioEngine::loadURL(const std::string& inputUrl) {
    std::string url = inputUrl;

    // Prüfe auf .m3u oder .pls
    std::string ext = std::filesystem::path(url).extension().string();
    if (ext == ".m3u" || ext == ".pls") {
        logDebug("Playlist erkannt – versuche, Stream-URL zu extrahieren...");
        std::string resolved = resolvePlaylist(url);
        if (resolved.empty()) {
            logError("Konnte Playlist nicht auflösen.");
            return false;
        }
        logDebug("Gefundene Stream-URL: " + resolved);
        url = resolved;
    }

    if (ma_device_get_state(&device) == ma_device_state_started) {
        stop();
        logDebug("Audiowiedergabe gestoppt, um neuen Stream zu laden.");
    }

    streamBuffer = std::make_shared<StreamingBuffer>(512 * 1024); // 512 KB Puffer
    downloader = std::make_unique<StreamingDownloader>(*streamBuffer); // Downloader initialisieren

    if (!downloader->start(url)) {
        logError("Stream konnte nicht gestartet werden.");
        return false;
    }

    // ⏳ Warten, bis Daten vorhanden
    int timeout = 0;
    while (streamBuffer->getBufferedBytes() < 16 * 1024 && timeout++ < 200) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    decoder = std::make_unique<StreamMP3Decoder>(*streamBuffer);
    if (!decoder || decoder->getSampleRate() == 0) {
        logError("StreamMP3Decoder konnte nicht initialisiert werden.");
        return false;
    }

    sampleRate = decoder->getSampleRate();
    channels = decoder->getChannels();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = [](ma_device* device, void* output, const void*, ma_uint32 frameCount) {
        auto* engine = static_cast<AudioEngine*>(device->pUserData);
        assert(engine && engine->decoder);
        short* out = static_cast<short*>(output);
        size_t framesRead = engine->decoder->decode(out, frameCount);
        if (framesRead < frameCount) {
            std::memset(out + framesRead * engine->channels, 0,
                        (frameCount - framesRead) * engine->channels * sizeof(short));
        }
    };
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        logError("Fehler beim Initialisieren des Audiogeräts.");
        return false;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        logError("Fehler beim Starten des Audiogeräts.");
        return false;
    }

    logDebug("Stream-Device erfolgreich gestartet.");
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
      //  logDebug("Audiowiedergabe gestoppt.");
    }
    Playing = false;
}

void AudioEngine::setVolume(float volume) {
    ma_device_set_master_volume(&device, volume);
    logDebug("Lautstärke gesetzt: " + std::to_string(volume));
}

bool AudioEngine::seek(int frame) {
    if (decoder && decoder->seek(frame)) {
        //logDebug("Audio auf Frame " + std::to_string(frame) + " gesetzt.");
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


bool AudioEngine::initAudioDevice() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = AudioEngine::maCallback;
    config.pUserData         = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        logError("miniaudio-Gerät konnte nicht initialisiert werden.");
        return false;
    }

       // Versuche, Standardgerät auszugeben (Index 0 ist meist Standard)
    ma_context context;
    ma_context_config ctxCfg = ma_context_config_init();
    if (ma_context_init(nullptr, 0, &ctxCfg, &context) == MA_SUCCESS) {
        ma_device_info* pPlaybackDevices = nullptr;
        uint32_t playbackDeviceCount = 0;

        if (ma_context_get_devices(&context, &pPlaybackDevices, &playbackDeviceCount, nullptr, nullptr) == MA_SUCCESS) {
            if (playbackDeviceCount > 0) {
                logDebug("Standard-Ausgabegerät (vermutlich): " + std::string(pPlaybackDevices[0].name));
            }
        }

        ma_context_uninit(&context);
    }

    logDebug("miniaudio-Gerät erfolgreich initialisiert.");
    return true;
}

bool AudioEngine::initAudioDeviceWithAdapter(int deviceIndex) {
    ma_device_info* pPlaybackDevices = nullptr;
    uint32_t playbackDeviceCount = 0;

    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        logError("miniaudio-Kontext konnte nicht initialisiert werden.");
        return false;
    }

    if (ma_context_get_devices(&context, &pPlaybackDevices, &playbackDeviceCount, nullptr, nullptr) != MA_SUCCESS) {
        logError("Fehler beim Abrufen der Wiedergabegeräte.");
        ma_context_uninit(&context);
        return false;
    }

    if (deviceIndex < 0 || static_cast<uint32_t>(deviceIndex) >= playbackDeviceCount) {
        logError("Ungültiger Geräteindex: " + std::to_string(deviceIndex));
        ma_context_uninit(&context);
        return false;
    }

    ma_device_id selectedDeviceID = pPlaybackDevices[deviceIndex].id;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format    = ma_format_s16;
    config.playback.channels  = channels;
    config.sampleRate         = sampleRate;
    config.playback.pDeviceID = &selectedDeviceID;
    config.dataCallback       = AudioEngine::maCallback;
    config.pUserData          = this;

    // ✅ WICHTIG: nullptr für default context → miniaudio verwaltet es selbst
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        logError("miniaudio-Gerät konnte nicht initialisiert werden.");
        ma_context_uninit(&context);
        return false;
    }

    logDebug("Initialisiertes Ausgabegerät: " + std::string(pPlaybackDevices[deviceIndex].name));
    ma_context_uninit(&context);  // ist optional, kann wegfallen

    logDebug("Init OK: sampleRate = " + std::to_string(sampleRate) +
         ", channels = " + std::to_string(channels));

    ma_device_state state = ma_device_get_state(&device);
    logDebug("Gerätestatus: " + std::to_string(state)); // 2 = started

    return true;
}


void AudioEngine::maCallback(ma_device* device, void* output, const void*, ma_uint32 frameCount) {
    auto* engine = static_cast<AudioEngine*>(device->pUserData);
    short* out = static_cast<short*>(output);

    size_t framesRead = engine->decoder->decode(out, frameCount);


    if (framesRead < frameCount) {
        std::memset(out + framesRead * engine->channels, 0,
                    (frameCount - framesRead) * engine->channels * sizeof(short));
    }

    if (framesRead == 0) {
        engine->Playing = false;
        return;
    }
}

std::vector<std::string> AudioEngine::getAvailableOutputDevices() {
    std::vector<std::string> result;

    ma_context context;
    ma_context_config ctxConfig = ma_context_config_init();
    if (ma_context_init(nullptr, 0, &ctxConfig, &context) != MA_SUCCESS) {
        logError("miniaudio-Kontext konnte nicht initialisiert werden.");
        return result;
    }

    ma_device_info* pPlaybackDevices = nullptr;
    uint32_t playbackDeviceCount = 0;

    if (ma_context_get_devices(&context, &pPlaybackDevices, &playbackDeviceCount, nullptr, nullptr) == MA_SUCCESS) {
        for (uint32_t i = 0; i < playbackDeviceCount; ++i) {
            std::string name = std::string(pPlaybackDevices[i].name);
            result.push_back(name);
          //  logDebug("Gefundenes Ausgabegerät [" + std::to_string(i) + "]: " + name);
        }
    } else {
        logError("Konnte Ausgabegeräte nicht ermitteln.");
    }

    ma_context_uninit(&context);
    return result;
}

std::string AudioEngine::resolvePlaylist(const std::string& playlistUrl) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string data;
    curl_easy_setopt(curl, CURLOPT_URL, playlistUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || data.empty()) return "";

    std::istringstream stream(data);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line[0] != '#') {
            return line;
        }
    }
    return "";
}

float AudioEngine::analyzeBPM(float* input, uint_t numSamples) {
    uint_t samplerate = sampleRate;
    uint_t hopSize = 512;
    uint_t winSize = 1024;

    aubio_tempo_t* tempo = new_aubio_tempo("default", winSize, hopSize, samplerate);
    aubio_tempo_set_silence(tempo, -90.0f); // niedriger, damit leise Onsets erkannt werden
    aubio_tempo_set_threshold(tempo, 0.1f); // empfindlicher machen

    fvec_t* vec = new_fvec(hopSize);
    fvec_t* out = new_fvec(1);

    for (uint_t i = 0; i + hopSize < numSamples; i += hopSize) {
        for (uint_t j = 0; j < hopSize; ++j) {
            vec->data[j] = input[i + j];
        }

        aubio_tempo_do(tempo, vec, out);
        
    }    


    float bpm = aubio_tempo_get_bpm(tempo);
    
    del_fvec(vec);
    del_fvec(out);

    del_aubio_tempo(tempo);

    int roundedBPM = static_cast<int>(std::round(bpm));
    //logDebug("BPM geschätzt: " + std::to_string(roundedBPM));
    return static_cast<float>(roundedBPM);
}

float AudioEngine::getBPM() {
    if (!decoder) {
        logDebug("Kein Decoder geladen – BPM nicht möglich.");
        return 0.0f;
    }

    if (lastFilePath.empty()) {
        logDebug("Kein Pfad für BPM-Analyse gespeichert.");
        return 0.0f;
    }
    std::unique_ptr<AudioDecoder> bpmDecoder;
    if (dynamic_cast<MP3Decoder*>(decoder.get())) {
        bpmDecoder = std::make_unique<MP3Decoder>();
    } else if (dynamic_cast<FlacDecoder*>(decoder.get())) {
        bpmDecoder = std::make_unique<FlacDecoder>();
    } else if (dynamic_cast<OggDecoder*>(decoder.get())) {
        bpmDecoder = std::make_unique<OggDecoder>();
    } else if (dynamic_cast<WavDecoder*>(decoder.get())) {
        bpmDecoder = std::make_unique<WavDecoder>();
    }

    if (!bpmDecoder || !bpmDecoder->load(lastFilePath)) {
        logDebug("BPM Decoder konnte Datei nicht erneut laden.");
        return 0.0f;
    }

    size_t totalFrames = bpmDecoder->getTotalFrames();
    std::vector<short> tempBuffer(totalFrames * bpmDecoder->getChannels());
    size_t framesDecoded = bpmDecoder->decode(tempBuffer.data(), totalFrames);

    // Umwandlung in float
    std::vector<float> floatBuffer(tempBuffer.size());
    for (size_t i = 0; i < tempBuffer.size(); ++i) {
        floatBuffer[i] = static_cast<float>(tempBuffer[i]) / 32768.0f;
    }

    return analyzeBPM(floatBuffer.data(), static_cast<uint_t>(floatBuffer.size()));
}

//IDV1-Tags Reading All decoders have this
std::string AudioEngine::getTitle() const {
    if (!decoder) return {};
    return decoder->getDecodedAudio().title;    
}

std::string AudioEngine::getArtist() const {
    if (!decoder) return {};
    return decoder->getDecodedAudio().artist;
}

std::string AudioEngine::getAlbum() const {
    if (!decoder) return {};
    return decoder->getDecodedAudio().album;
}




