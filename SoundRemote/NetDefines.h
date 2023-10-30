#pragma once

#include <stdint.h>
#include <boost/asio/ip/address.hpp>

namespace Net {
	namespace Packet {
		//Header fields
		using SignatureType = uint16_t;
		using CategoryType = uint8_t;
		using SizeType = uint16_t;

		const int headerSize = sizeof SignatureType + sizeof CategoryType + sizeof SizeType;
		const int signatureOffset = 0;
		const int categoryOffset = sizeof SignatureType;
		const int sizeOffset = sizeof SignatureType + sizeof CategoryType;
		const int dataOffset = headerSize;

		const SignatureType protocolSignature = 0xA571u;
		enum Category: CategoryType {
			Error = 0,
			ClientKeepAlive = 0x01u,
			ServerKeepAlive = 0x02u,
			ClientCommand = 0x10u,
			AudioDataOpus = 0x20u,
			AudioDataPcm = 0x21u
		};

		namespace Keystroke {
			using Key = uint32_t;
			using Mods = uint32_t;

			const int dataSize = sizeof Key + sizeof Mods;
		}
	}
	using Address = boost::asio::ip::address;

	const uint16_t defaultServerPort = 15711u;
	const uint16_t defaultClientPort = 15712u;

	const int inputPacketSize = 1024;
}
