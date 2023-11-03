#include "pch.h"
#include "EncoderOpus.h"
#include "AudioUtil.h"

namespace {
	using namespace Audio;
	
	class SampleRates : public testing::TestWithParam<Opus::SampleRate> {
	public:
		void SetUp() override { sampleRate_ = GetParam(); }
	protected:
		Opus::SampleRate sampleRate_ = {};
	};

	TEST_P(SampleRates, CreatesWithValidSampleRate)  {
		EXPECT_NO_THROW(EncoderOpus(Bitrate::kbps_128, sampleRate_, Opus::Channels::mono));
		EXPECT_NO_THROW(EncoderOpus(Bitrate::kbps_128, sampleRate_, Opus::Channels::stereo));
	}

	INSTANTIATE_TEST_SUITE_P(EncoderOpusTest, SampleRates, ::testing::Values(
		Opus::SampleRate::khz_8,
		Opus::SampleRate::khz_12,
		Opus::SampleRate::khz_16,
		Opus::SampleRate::khz_24,
		Opus::SampleRate::khz_48
	), [](const testing::TestParamInfo<SampleRates::ParamType>& info) {
		const int value = static_cast<int>(info.param);
		return std::to_string(value);
	});

	class Bitrates : public testing::TestWithParam<Bitrate> {
	public:
		void SetUp() override { bitrate_ = GetParam(); }
	protected:
		Bitrate bitrate_ = {};
	};

	TEST_P(Bitrates, CreatesWithValidBitrate) {
		EXPECT_NO_THROW(EncoderOpus(bitrate_, Opus::SampleRate::khz_48, Opus::Channels::mono));
		EXPECT_NO_THROW(EncoderOpus(bitrate_, Opus::SampleRate::khz_48, Opus::Channels::stereo));
	}

	INSTANTIATE_TEST_SUITE_P(EncoderOpusTest, Bitrates, ::testing::Values(
		Bitrate::kbps_64,
		Bitrate::kbps_128,
		Bitrate::kbps_192,
		Bitrate::kbps_256,
		Bitrate::kbps_320
		), [](const testing::TestParamInfo<Bitrates::ParamType>& info) {
		const int value = static_cast<int>(info.param);
		return std::to_string(value);
	});
}
