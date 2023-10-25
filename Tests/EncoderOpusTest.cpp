#include "pch.h"
#include "EncoderOpus.h"
#include "AudioUtil.h"

namespace {
	using namespace Audio::Opus;
	
	class EncoderOpusTest : public testing::TestWithParam<Audio::Opus::SampleRate> {
	public:
		void SetUp() override { sampleRate_ = GetParam(); }
	protected:
		SampleRate sampleRate_ = {};
	};

	TEST_P(EncoderOpusTest, CreatesWithValidSampleRate)  {
		EXPECT_NO_THROW(EncoderOpus(Audio::Bitrate::kbps_128, sampleRate_, Channels::mono));
		EXPECT_NO_THROW(EncoderOpus(Audio::Bitrate::kbps_128, sampleRate_, Channels::stereo));
	}

	INSTANTIATE_TEST_SUITE_P(SampleRates, EncoderOpusTest, ::testing::Values(
		SampleRate::khz_8,
		SampleRate::khz_12,
		SampleRate::khz_16,
		SampleRate::khz_24,
		SampleRate::khz_48
	), [](const testing::TestParamInfo<EncoderOpusTest::ParamType>& info) {
		const int value = static_cast<int>(info.param);
		return std::to_string(value);
	});
}
