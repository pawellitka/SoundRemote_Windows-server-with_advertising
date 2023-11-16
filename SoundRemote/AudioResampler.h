#pragma once

#include <span>

#include <boost/asio/streambuf.hpp>

#include <sal.h>
#include <atlcomcli.h>	//CComPtr
#include <mmreg.h>      //WAVEFORMATEXTENSIBLE

struct IMFTransform;

class AudioResampler {
public:
	AudioResampler(_In_ const WAVEFORMATEXTENSIBLE* inputFormat, _In_ const WAVEFORMATEXTENSIBLE* outputFormat,
		_In_ boost::asio::streambuf& outBuffer);
	~AudioResampler();
    void resample(_In_ const std::span<char>& pcmAudio);
private:
	CComPtr<IMFTransform> transform_;
	boost::asio::streambuf& outBuffer_;
	double outBufferSizeMultiplier_ = 0.0;
};
