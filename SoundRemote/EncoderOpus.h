#pragma once

struct OpusEncoder;

struct EncoderDeleter {
	void operator()(OpusEncoder* enc) const;
};

class EncoderOpus {
	using encoder_ptr = std::unique_ptr<OpusEncoder, EncoderDeleter>;
public:
	EncoderOpus(Audio::Format format);

	/// <summary>
	/// Encodes a frame of PCM audio.
	/// </summary>
	/// <param name="pcmAudio">: Input signal.</param>
	/// <param name="encodedPacket">: Buffer to contain the encoded packet.
	/// Must be at least <c>EncoderOpus::inputLength()</c> bytes size.</param>
	/// <returns>Encoded packet length in bytes. If the return value is 0 encoded packet does not need to be transmitted (DTX).</returns>
	int encode(const char* pcmAudio, unsigned char* encodedPacket);
	//Required input data length in bytes
	size_t inputLength() const;
private:
	encoder_ptr encoder_;
	std::size_t frameSize_;
	std::size_t inputPacketSize_;
};
