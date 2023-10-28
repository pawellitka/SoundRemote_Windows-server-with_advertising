#include <span>
#include <optional>

#include "NetDefines.h"
#include "keystroke.h"

namespace Net {
	/// <summary>
	/// Gets string representations of the IPv4 IP addresses.
	/// </summary>
	/// <returns>List of addresses or an empty list if an error occurs.</returns>
	std::forward_list<std::wstring> getLocalAddresses();

	std::vector<unsigned char> assemblePacket(const Net::Packet::CategoryType category, std::span<unsigned char> data = {});

	bool hasValidHeader(std::span<unsigned char> packet);
	Packet::CategoryType getPacketCategory(std::span<unsigned char> packet);
	std::optional<Keystroke> getKeystroke(std::span<unsigned char> packet);
};
