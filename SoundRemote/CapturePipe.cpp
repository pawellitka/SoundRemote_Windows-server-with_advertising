#include <boost/asio.hpp>
#include <coroutine>
#include <unordered_set>

#include "Util.h"
#include "AudioUtil.h"
#include "AudioCapture.h"
#include "AudioResampler.h"
#include "EncoderOpus.h"
#include "Server.h"
#include "Clients.h"
#include "CapturePipe.h"

struct task {
    struct promise_type;
    using handle_t = std::coroutine_handle<promise_type>;

    struct promise_type {
        task get_return_object() {
            return handle_t::from_promise(*this);
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {
            auto exception = std::current_exception();
            std::rethrow_exception(exception);
        }
    };

    handle_t h_;
    task(handle_t h) :h_{ h } {}
    ~task() { /*h_.destroy();*/ }
    operator handle_t() const { return h_; }
    //task(const task&) = delete;
};

CapturePipe::CapturePipe(const std::wstring& deviceId, std::shared_ptr<Server> server, boost::asio::io_context& ioContext, bool muted):
    device_(deviceId), server_(server), io_context_(ioContext), muted_(muted) {
    //throw std::runtime_error("CapturePipe::ctr");
    Audio::Format requestedFormat;
    audioCapture_ = std::make_unique<AudioCapture>(deviceId, requestedFormat, ioContext);
    if (audioCapture_->resampleRequired()) {
        auto capturedWaveFormat = audioCapture_->capturedWaveFormat();
        auto requestedWaveFormat = audioCapture_->requestedWaveFormat();
        audioResampler_ = std::make_unique<AudioResampler>(capturedWaveFormat, requestedWaveFormat, pcmAudioBuffer_);
    }
    opusInputSize_ = EncoderOpus::getInputSize(EncoderOpus::getFrameSize(Audio::Opus::SampleRate::khz_48), Audio::Opus::Channels::stereo);
    encoder_ = std::make_unique<EncoderOpus>(Audio::Bitrate::kbps_192, Audio::Opus::SampleRate::khz_48, Audio::Opus::Channels::stereo);
}

CapturePipe::~CapturePipe() {
    stop();
}

void CapturePipe::start() {
    pipeCoro_ = std::make_unique<task>(process());
}

float CapturePipe::getPeakValue() const {
    return audioCapture_->getPeakValue();
}

void CapturePipe::setMuted(bool muted) {
    muted_ = muted;
}

void CapturePipe::onClientsUpdate(std::forward_list<ClientInfo> clients) {
    std::unordered_set<Audio::Bitrate> newBitrates;
    std::unordered_set<Audio::Bitrate> repeatingBitrates;
    for (auto&& client : clients) {
        if (encoders_.contains(client.bitrate)) {
            repeatingBitrates.insert(client.bitrate);
        } else {
            newBitrates.insert(client.bitrate);
        }
    }

    if (newBitrates.empty() && repeatingBitrates.size() == encoders_.size()) {
        return;
    }

    std::erase_if(encoders_, [&](const auto& item) {
        return !repeatingBitrates.contains(item.first);
    });
    for (auto&& it: newBitrates) {
        encoders_[it] = std::make_unique<EncoderOpus>(it, Audio::Opus::SampleRate::khz_48,
            Audio::Opus::Channels::stereo);
    }
}

task CapturePipe::process() {
    //throw std::runtime_error("CapturePipe::process start");
    auto audioCapture = audioCapture_->capture();
    for (;;) {
        auto capturedAudio = co_await audioCapture;
        if (!muted_) {
            auto server = server_.lock();
            if (server && haveClients()) {
                process(capturedAudio, server);
            }
        }
        //throw std::runtime_error("CapturePipe::process loop");
        audioCapture.h_();
    }
}

void CapturePipe::stop() {
    if (pipeCoro_) {
        pipeCoro_->h_.destroy();
        pipeCoro_.reset();
    }
}

bool CapturePipe::haveClients() const {
    return !encoders_.empty();
}

void CapturePipe::process(std::span<char> pcmAudio, std::shared_ptr<Server> server) {
    if (audioCapture_->resampleRequired()) {
        audioResampler_->resample(pcmAudio);
    } else {
        pcmAudioBuffer_.sputn(pcmAudio.data(), pcmAudio.size());
    }
    while (pcmAudioBuffer_.data().size() >= encoder_->inputLength()) {
        for (auto&& [bitrate, encoder] : encoders_) {
            if (bitrate == Audio::Bitrate::none) {
                std::vector<char> pcm(reinterpret_cast<const char*>(pcmAudioBuffer_.data().data())[0],
                    reinterpret_cast<const char*>(pcmAudioBuffer_.data().data())[encoder_->inputLength()]);
                server->sendAudio(bitrate, pcm);
            } else {
                std::vector<char> encodedPacket(Audio::Opus::maxPacketSize);
                const auto packetSize = encoder->encode(static_cast<const char*>(pcmAudioBuffer_.data().data()), encodedPacket.data());
                encodedPacket.resize(packetSize);
                if (packetSize > 0) {
                    server->sendAudio(bitrate, encodedPacket);
                }
            }
        }
        pcmAudioBuffer_.consume(encoder_->inputLength());
    }
}
