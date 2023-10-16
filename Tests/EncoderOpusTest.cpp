#include "pch.h"
#include "EncoderOpus.h"
#include "AudioUtil.h"

namespace {
	using namespace Audio::Opus;
	
	class EncoderOpusTest : public testing::TestWithParam<Audio::Opus::SampleRate> {
	public:
		void SetUp() override { sampleRate_ = GetParam(); }
	protected:
		SampleRate sampleRate_ = SampleRate::_8k;
	};

	TEST_P(EncoderOpusTest, CreatesWithValidSampleRate)  {
		EXPECT_NO_THROW(EncoderOpus(sampleRate_, Channels::mono));
		EXPECT_NO_THROW(EncoderOpus(sampleRate_, Channels::stereo));
	}

	INSTANTIATE_TEST_SUITE_P(SampleRates, EncoderOpusTest, ::testing::Values(
		SampleRate::_8k,
		SampleRate::_12k,
		SampleRate::_16k,
		SampleRate::_24k,
		SampleRate::_48k
	), [](const testing::TestParamInfo<EncoderOpusTest::ParamType>& info) {
		const int value = static_cast<int>(info.param);
		return std::to_string(value);
	});
}
