#include "FlacDecoder.h"
#include <cstdint>
#include "dr_flac.h"

bool FlacDecoder::load(const std::string& path) {

    m_flac = drflac_open_file(path.c_str(), nullptr);
    if (!m_flac) return false;

    // Basisdaten setzen
    decodedAudio.sampleRate = m_flac->sampleRate;
    decodedAudio.channels = m_flac->channels;
    decodedAudio.totalFrames = m_flac->totalPCMFrameCount;
    
    return m_flac != nullptr;
}

size_t FlacDecoder::decode(short* buffer, size_t framesToRead) {
    if (!m_flac) return 0;
    return drflac_read_pcm_frames_s16(m_flac, framesToRead, buffer);
}

bool FlacDecoder::seek(int frame) {
    return m_flac && drflac_seek_to_pcm_frame(m_flac, frame) != 0;
}

uint64_t FlacDecoder::getTotalFrames() const {
    return m_flac->totalPCMFrameCount;
}

int FlacDecoder::getSampleRate() const {
    return m_flac ? static_cast<int>(m_flac->sampleRate) : 0;
}

int FlacDecoder::getChannels() const {
    return m_flac ? static_cast<int>(m_flac->channels) : 0;
}

uint64_t FlacDecoder::getCursor() const {
    return m_flac ? m_flac->currentPCMFrame : 0;
}   

size_t FlacDecoder::getCurrentFrame() const {
    return static_cast<size_t>(getCursor());
}

