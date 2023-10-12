#pragma once

#include <span>

class AudioCapture;
class AudioResampler;
class EncoderOpus;
class Server;
struct task;

class CapturePipe {
public:
	CapturePipe(const std::wstring& deviceId, std::shared_ptr<Server> server, boost::asio::io_context& io_context);
	~CapturePipe();
	void start();
	float getPeakValue() const;
private:
	// Capturing coroutine
	task process();
	// Destroys the capturing coroutine
	void stop();
	void processAudio(std::span<char> pcmAudio, std::shared_ptr<Server> server);

	boost::asio::io_context& io_context_;
	std::unique_ptr<task> pipeCoro_;
	std::unique_ptr<AudioCapture> audioCapture_;
	std::unique_ptr<AudioResampler> audioResampler_;
	std::unique_ptr<EncoderOpus> encoder_;
	std::weak_ptr<Server> server_;
	const std::wstring device_;
	boost::asio::streambuf pcmAudioBuffer_;
};
