#include <vector>

#include "pch.h"
#include "NetUtil.h"

std::vector<char> initPacket(std::initializer_list<unsigned int> data) {
	std::vector<char> result(data.size());
	for (int i = 0; i < data.size(); ++i) {
		result[i] = static_cast<char>(*(data.begin() + i));
	}
	return result;
}

namespace {
	TEST(Net_createKeepAlivePacket, createsValidPacket) {
		std::vector<char> expected = initPacket({ 0x71, 0xA5, 0x31, 0x05, 0 });

		const auto actual = Net::createKeepAlivePacket();

		EXPECT_EQ(actual, expected);
	}
}
