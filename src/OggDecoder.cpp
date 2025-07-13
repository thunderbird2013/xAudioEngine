#include "stb_vorbis_all.h"
#include <cstdint>
#include "OggDecoder.h"

OggDecoder::OggDecoder() = default;

OggDecoder::~OggDecoder() {
    if (vorbis) stb_vorbis_close(vorbis);
}

bool OggDecoder::load(const std::string& path) {
    int error;

    vorbis = stb_vorbis_open_filename(path.c_str(), &error, nullptr);
    if (!vorbis) return false;

    info = stb_vorbis_get_info(vorbis);

    stb_vorbis_seek_start(vorbis);

    stb_vorbis_comment comment = stb_vorbis_get_comment(vorbis);

    for (int i = 0; i < comment.comment_list_length; ++i) {
   
        std::string entry(comment.comment_list[i]);
        auto pos = entry.find('=');
      
        if (pos != std::string::npos) {
            std::string key = entry.substr(0, pos);
            std::string val = entry.substr(pos + 1);

            if (key == "TITLE")       decodedAudio.title  = val;
            else if (key == "ARTIST") decodedAudio.artist = val;
            else if (key == "ALBUM")  decodedAudio.album  = val;
        }

    }

    return true;

}

size_t OggDecoder::decode(short* buffer, size_t framesToRead) {
    if (!vorbis) return 0;
    return stb_vorbis_get_samples_short_interleaved(vorbis, info.channels, buffer, static_cast<int>(framesToRead * info.channels));
}

bool OggDecoder::seek(int frame) {
    return vorbis && stb_vorbis_seek(vorbis, frame) != 0;
}

int OggDecoder::getSampleRate() const {
    return vorbis ? info.sample_rate : 0;
}

int OggDecoder::getChannels() const {
    return vorbis ? info.channels : 0;
}

uint64_t OggDecoder::getCursor() const {
    return stb_vorbis_get_sample_offset(vorbis);
}

uint64_t OggDecoder::getTotalFrames() const {
    return stb_vorbis_stream_length_in_samples(vorbis);
}

size_t OggDecoder::getCurrentFrame() const {
    return static_cast<size_t>(getCursor());
}
