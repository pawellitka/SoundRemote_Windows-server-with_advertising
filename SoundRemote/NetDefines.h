#pragma once

#include <stdint.h>
#include <boost/asio/ip/address.hpp>

namespace Net {
	namespace Packet {
		//Header fields
		using Signature = uint16_t;
		using PacketType = uint8_t;
		using PacketSize = uint16_t;

		const int HEADER_SIZE = sizeof Signature + sizeof PacketType + sizeof PacketSize;
		const int SIGNATURE_OFFSET = 0;
		const int TYPE_OFFSET = sizeof Signature;
		const int LEN_OFFSET = sizeof Signature + sizeof PacketType;
		const int DATA_OFFSET = HEADER_SIZE;

		const Signature PROTOCOL_SIGNATURE = 0xA571u;
		enum Type : PacketType {
			Error = 0,
			ClientKeepAlive = 0x01u,
			ServerKeepAlive = 0x02u,
			ClientCommand = 0x10u,
			AudioDataOpus = 0x20u
		};

		namespace Keystroke {
			using Key = uint32_t;
			using Mods = uint32_t;

			const int DATA_SIZE = sizeof Key + sizeof Mods;
		}
	}
	using Address = boost::asio::ip::address;
	// Default ports
	const uint16_t DEFAULT_PORT_IN = 15711u;
	const uint16_t DEFAULT_PORT_OUT = 15712u;
	// Incoming packet size limit
	const int IN_PACKET_SIZE = 1024;
}
