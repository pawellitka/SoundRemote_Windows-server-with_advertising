#pragma once

#include "AudioUtil.h"

struct OpusEncoder;

struct EncoderDeleter {
	void operator()(OpusEncoder* enc) const;
};

class EncoderOpus {
	using encoder_ptr = std::unique_ptr<OpusEncoder, EncoderDeleter>;
public:
	EncoderOpus(Audio::Compression compression, Audio::Opus::SampleRate sampleRate, Audio::Opus::Channels channels);

	/// <summary>
	/// Encodes a frame of PCM audio.
	/// </summary>
	/// <param name="pcmAudio">: Input signal in 16 bit signed int format.</param>
	/// <param name="encodedPacket">: Buffer to contain the encoded packet.
	/// Must be at least <c>EncoderOpus::inputLength()</c> bytes size.</param>
	/// <returns>Encoded packet length in bytes. If the return value is 0 encoded packet does not need to be transmitted (DTX).</returns>
	int encode(const char* pcmAudio, char* encodedPacket);
	//Required input data length in bytes for 16 bit sample size.
	int inputLength() const;
	static int getFrameSize(Audio::Opus::SampleRate sampleRate);
	static int getInputSize(int frameSize, Audio::Opus::Channels channels);
private:

	encoder_ptr encoder_;
	// Number of samples per frame
	int frameSize_;
	// Bytes per input packet for 16 bit sample size
	int inputPacketSize_;

	EncoderOpus(const EncoderOpus&) = delete;
	EncoderOpus& operator= (const EncoderOpus&) = delete;
};
