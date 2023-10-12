#include <stdexcept>
#include <set>
#include <memory>
#include <opus/opus.h>

#include "Util.h"
#include "AudioUtil.h"
#include "EncoderOpus.h"

namespace {
    bool isFormatSupported(const Audio::Format& format) {
        std::set<int> sampleRates{ 8000, 12000, 16000, 24000, 48000 };
        if (sampleRates.find(format.sampleRate) == sampleRates.end())
            return false;
        if (format.channelCount > 2 || format.channelCount < 1)
            return false;
        if (format.sampleType != Audio::SampleType::SignedInt)
            return false;
        if (format.sampleSize != 16)
            return false;
        if (format.byteOrder != Util::Endian::Little)
            return false;
        return true;
    }
}

EncoderOpus::EncoderOpus(Audio::Format format) {
    if (!isFormatSupported(format)) {
        Audio::processError(0, Audio::Location::ENCODER_UNSUPPORTED_FORMAT);
    }
    // Number of samples per frame
    frameSize_ = Audio::Opus::FRAME_LENGTH * format.sampleRate / 1000;
    // Bytes per input packet
    inputPacketSize_ = frameSize_ * format.channelCount * format.sampleSize / 8;

    int error;
    encoder_ = encoder_ptr(opus_encoder_create(format.sampleRate, format.channelCount, OPUS_APPLICATION_AUDIO, &error), EncoderDeleter());
    if (OPUS_OK != error || nullptr == encoder_) {
        Audio::processError(error, Audio::Location::ENCODER_CREATE);
    }
    auto ret = opus_encoder_ctl(encoder_.get(), OPUS_SET_BITRATE(Audio::Opus::BITRATE));
    if (ret != OPUS_OK) {
        Audio::processError(ret, Audio::Location::ENCODER_SET_BITRATE);
    };
}

int EncoderOpus::encode(const char* pcmAudio, unsigned char* encodedPacket) {
    const opus_int32 encodeResult = opus_encode(encoder_.get(), reinterpret_cast<const opus_int16*>(pcmAudio), static_cast<int>(frameSize_),
        encodedPacket, Audio::Opus::MAX_PACKET_SIZE);
    // If DTX is on and the return value is 2 bytes or less, then the packet does not need to be transmitted.
    if (encodeResult >= 0 && encodeResult <= 2) {
        return 0;
    }
    if (encodeResult < 0 || encodeResult > Audio::Opus::MAX_PACKET_SIZE) {
        Audio::processError(encodeResult, Audio::Location::ENCODER_ENCODE);
    }
    return encodeResult;
}

std::size_t EncoderOpus::inputLength() const {
    return inputPacketSize_;
}

//---EncoderDeleter---

void EncoderDeleter::operator()(OpusEncoder* enc) const {
    opus_encoder_destroy(enc);
}
