#include "keystroke.h"

#include <span>
#include <optional>

namespace Net {
	namespace Packet {
		//Header fields
		using signature_t = uint16_t;
		using type_t = uint8_t;
		using len_t = uint16_t;

		const int HEADER_SIZE = sizeof signature_t + sizeof type_t + sizeof len_t;
		const int SIGNATURE_OFFSET = 0;
		const int TYPE_OFFSET = sizeof signature_t;
		const int LEN_OFFSET = sizeof signature_t + sizeof type_t;
		const int DATA_OFFSET = HEADER_SIZE;

		const signature_t PROTOCOL_SIGNATURE = 0xA571u;
		enum Type : type_t {
			Error = 0,
			ClientKeepAlive = 0x01u,
			ServerKeepAlive = 0x02u,
			ClientCommand	= 0x10u,
			AudioDataOpus	= 0x20u
		};

		namespace Keystroke {
			using key_t = uint32_t;
			using mods_t = uint32_t;

			const int DATA_SIZE = sizeof key_t + sizeof mods_t;
		}
	}
	// Default ports
	const uint16_t DEFAULT_PORT_IN = 15711u;
	const uint16_t DEFAULT_PORT_OUT = 15712u;
	// Incoming packet size limit
	const int IN_PACKET_SIZE = 1024;

	/// <summary>
	/// Gets string representations of the IPv4 IP addresses.
	/// </summary>
	/// <returns>List of addresses or an empty list if an error occurs.</returns>
	std::forward_list<std::wstring> getLocalAddresses();

	std::vector<unsigned char> assemblePacket(const Net::Packet::type_t packetType, std::span<unsigned char> data = {});

	bool hasValidHeader(std::span<unsigned char> packet);
	Packet::type_t getPacketType(std::span<unsigned char> packet);
	std::optional<Keystroke> getKeystroke(std::span<unsigned char> packet);
};
