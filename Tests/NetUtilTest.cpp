#include <vector>
#include <sstream>
#include <tuple>
#include <span>

#include "pch.h"
#include "NetUtil.h"

namespace {
	std::vector<char> initPacket(std::initializer_list<unsigned int> data) {
		std::vector<char> result(data.size());
		for (int i = 0; i < data.size(); ++i) {
			result[i] = static_cast<char>(*(data.begin() + i));
		}
		return result;
	}

	constexpr char audioDataArr[]{ 0xFAu, 0xFBu, 0x01u, 0x12u };
	constexpr std::span<const char> audioData{ audioDataArr };
}

namespace {
	using ::testing::Combine;
	using ::testing::TestWithParam;
	using ::testing::Values;
	using ::testing::TestParamInfo;
	using namespace Net::Packet;

	// createAudioPacket
	TEST(Net_createAudioPacket, createsValidPacketUncompressed) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0x20, 0x09, 0, 0xFA, 0xFB, 0x01, 0x12 });

		const auto actual = Net::createAudioPacket(Category::AudioDataUncompressed, audioData);

		EXPECT_EQ(actual, expected);
	}

	TEST(Net_createAudioPacket, createsValidPacketOpus) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0x21, 0x09, 0, 0xFA, 0xFB, 0x01, 0x12 });

		const auto actual = Net::createAudioPacket(Category::AudioDataOpus, audioData);

		EXPECT_EQ(actual, expected);
	}

	// createKeepAlivePacket
	TEST(Net_createKeepAlivePacket, createsValidPacket) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0x31, 0x05, 0 });

		const auto actual = Net::createKeepAlivePacket();

		EXPECT_EQ(actual, expected);
	}

	// createDisconnectPacket
	TEST(Net_createDisconnectPacket, createsValidPacket) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0x02, 0x05, 0 });

		const auto actual = Net::createDisconnectPacket();

		EXPECT_EQ(actual, expected);
	}

	// compressionFromNetworkValue
	using Audio::Compression;
	class CompressionFromNetworkValue : public TestWithParam<std::tuple<CompressionType, std::optional<Compression>>> {
	protected:
		void SetUp() override {
			std::tie(netCompression_, expected_) = GetParam();
		}
		CompressionType netCompression_ = {};
		std::optional<Compression> expected_ = {};
	};

	TEST_P(CompressionFromNetworkValue, ReturnsCorrectCompression) {
		auto actual = Net::compressionFromNetworkValue(netCompression_);
		EXPECT_EQ(expected_, actual);
	}

	INSTANTIATE_TEST_SUITE_P(Net, CompressionFromNetworkValue, Values(
		std::tuple{ 0, Compression::none },
		std::tuple{ 1, Compression::kbps_64 },
		std::tuple{ 2, Compression::kbps_128 },
		std::tuple{ 3, Compression::kbps_192 },
		std::tuple{ 4, Compression::kbps_256 },
		std::tuple{ 5, Compression::kbps_320 }
	),[](const TestParamInfo<CompressionFromNetworkValue::ParamType>& info) {
		return std::to_string(static_cast<int>(std::get<1>(info.param).value()));
		}
	);

	// createAckConnectPacket
	TEST(Net_createAckConnectPacket, createsValidPacket) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0xF0, 0x0B, 0, 0xD5, 0xDD, Net::protocolVersion, 0, 0, 0 });

		RequestIdType requestId = 0xDDD5;
		const auto actual = Net::createAckConnectPacket(requestId);

		EXPECT_EQ(actual, expected);
	}

	// createAckSetFormatPacket
	TEST(Net_createAckSetFormatPacket, createsValidPacket) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0xF0, 0x0B, 0, 0xF1, 0xF0, 0, 0, 0, 0 });

		RequestIdType requestId = 0xF0F1;
		const auto actual = Net::createAckSetFormatPacket(requestId);

		EXPECT_EQ(actual, expected);
	}
}
