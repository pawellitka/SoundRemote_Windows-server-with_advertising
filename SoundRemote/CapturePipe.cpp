#include <boost/asio.hpp>
#include <coroutine>

#include "Util.h"
#include "AudioUtil.h"
#include "AudioCapture.h"
#include "AudioResampler.h"
#include "EncoderOpus.h"
#include "Server.h"
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

CapturePipe::CapturePipe(const std::wstring& deviceId, std::shared_ptr<Server> server, boost::asio::io_context& ioContext):
    device_(deviceId), server_(server), io_context_(ioContext) {
    //throw std::runtime_error("CapturePipe::ctr");
    Audio::Format requestedFormat;
    audioCapture_ = std::make_unique<AudioCapture>(deviceId, requestedFormat, ioContext);
    if (audioCapture_->resampleRequired()) {
        auto capturedWaveFormat = audioCapture_->capturedWaveFormat();
        auto requestedWaveFormat = audioCapture_->requestedWaveFormat();
        audioResampler_ = std::make_unique<AudioResampler>(capturedWaveFormat, requestedWaveFormat, pcmAudioBuffer_);
    }
    encoder_ = std::make_unique<EncoderOpus>(requestedFormat);
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

task CapturePipe::process() {
    //throw std::runtime_error("CapturePipe::process start");
    auto audioCapture = audioCapture_->capture();
    for (;;) {
        auto capturedAudio = co_await audioCapture;
        auto server = server_.lock();
        if (server && server->hasClients()) {
            processAudio(capturedAudio, server);
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

void CapturePipe::processAudio(std::span<char> pcmAudio, std::shared_ptr<Server> server) {
    if (audioCapture_->resampleRequired()) {
        audioResampler_->resample(pcmAudio);
    } else {
        pcmAudioBuffer_.sputn(pcmAudio.data(), pcmAudio.size());
    }
    while (pcmAudioBuffer_.data().size() >= encoder_->inputLength()) {
        std::array<unsigned char, Audio::Opus::MAX_PACKET_SIZE> encodedPacket;
        const auto packetSize = encoder_->encode(static_cast<const char*>(pcmAudioBuffer_.data().data()), encodedPacket.data());
        if (packetSize > 0) {
            server->sendOpusPacket({ encodedPacket.data(), static_cast<size_t>(packetSize) });
        }
        pcmAudioBuffer_.consume(encoder_->inputLength());
    }
}
