#pragma once

#include <stdint.h>
#include <boost/asio/ip/address.hpp>

namespace Net {
	namespace Packet {
		// Header fields
		using SignatureType = uint16_t;
		using CategoryType = uint8_t;
		using SizeType = uint16_t;
		// Data fields
		using RequestIdType = uint16_t;
		using CompressionType = uint8_t;
		using KeyType = uint8_t;
		using ModsType = uint8_t;
		// Header data
		constexpr int headerSize = sizeof SignatureType + sizeof CategoryType + sizeof SizeType;
		constexpr int signatureOffset = 0;
		constexpr int categoryOffset = sizeof SignatureType;
		constexpr int sizeOffset = sizeof SignatureType + sizeof CategoryType;
		constexpr int dataOffset = headerSize;
		// Packet data
		constexpr int keystrokeSize = sizeof KeyType + sizeof ModsType;
		constexpr int ackSize = sizeof RequestIdType;
		struct ConnectData {
			RequestIdType requestId;
			CompressionType compression;
			static const int size = sizeof CompressionType + sizeof RequestIdType;
		};
		struct SetFormatData {
			RequestIdType requestId;
			CompressionType compression;
			static const int size = sizeof CompressionType + sizeof RequestIdType;
		};

		constexpr SignatureType protocolSignature = 0xA571u;

		enum class Category: CategoryType {
			Error = 0,
			Connect = 0x01u,
			Disconnect = 0x02u,
			SetFormat = 0x03u,
			Keystroke = 0x10u,
			AudioDataUncompressed = 0x20u,
			AudioDataOpus = 0x21u,
			ClientKeepAlive = 0x30u,
			ServerKeepAlive = 0x31u,
			Ack = 0xF0u
		};
	}
	using Address = boost::asio::ip::address;

	const uint16_t defaultServerPort = 15711u;
	const uint16_t defaultClientPort = 15712u;

	const int inputPacketSize = 1024;
}
