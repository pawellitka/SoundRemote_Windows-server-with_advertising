#include <vector>
#include <sstream>
#include <tuple>

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
}

namespace {
	using ::testing::Combine;
	using ::testing::TestWithParam;
	using ::testing::Values;
	using ::testing::TestParamInfo;

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
	class CompressionFromNetworkValue : public TestWithParam<std::tuple<Net::Packet::CompressionType, std::optional<Compression>>> {
	protected:
		void SetUp() override {
			std::tie(netCompression_, expected_) = GetParam();
		}
		Net::Packet::CompressionType netCompression_ = {};
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
}
