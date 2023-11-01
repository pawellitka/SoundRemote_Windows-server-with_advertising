#include <vector>
#include <forward_list>
#include <string>
#include <cassert>

#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "NetUtil.h"

#pragma comment(lib, "iphlpapi.lib")

namespace {
	uint32_t readUInt32Le(std::span<unsigned char> data, size_t offset) {
		assert((offset + 4) <= data.size_bytes());
		uint32_t result = (data[offset + 0] << 0) | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
		return result;
	}

	uint16_t readUInt16Le(std::span<unsigned char> data, size_t offset) {
		assert((offset + 2) <= data.size_bytes());
		uint16_t result = (data[offset + 0] << 0) | (data[offset + 1] << 8);
		return result;
	}

	uint8_t readUInt8(std::span<unsigned char> data, size_t offset) {
		assert((offset + 1) <= data.size_bytes());
		uint8_t result = data[offset];
		return result;
	}

	void writeUInt16Le(uint16_t value, std::span<char> dest, size_t offset) {
		assert((offset + 2) <= dest.size_bytes());
		dest[offset] = value >> 0;
		dest[offset + 1] = value >> 8;
	}

	void writeUInt8(uint8_t value, std::span<char> dest, size_t offset) {
		assert((offset + 1) <= dest.size_bytes());
		dest[offset] = value;
	}
};

std::forward_list<std::wstring> Net::getLocalAddresses() {
	// Allocate MIB_IPADDRTABLE
	std::vector<char> buffer{ sizeof(MIB_IPADDRTABLE) };
	MIB_IPADDRTABLE* addrTable = reinterpret_cast<MIB_IPADDRTABLE*>(buffer.data());
	DWORD addrTableSize = 0;
	if (ERROR_INSUFFICIENT_BUFFER == GetIpAddrTable(addrTable, &addrTableSize, 0)) {
		buffer.resize(addrTableSize);
		addrTable = reinterpret_cast<MIB_IPADDRTABLE*>(buffer.data());
	}

	// Fill MIB_IPADDRTABLE
	if (NO_ERROR != GetIpAddrTable(addrTable, &addrTableSize, 0)) {
		return {};
	}

	// Get addresses from MIB_IPADDRTABLE
	std::forward_list<std::wstring> result;
	// For an IPv4 address, buffer should be large enough to hold at least 16 characters.
	std::wstring addr(16, 0);
	IN_ADDR IPAddr{};
	for (int i = addrTable->dwNumEntries - 1; i >= 0; --i) {
		IPAddr.S_un.S_addr = static_cast<u_long>(addrTable->table[i].dwAddr);
		auto res = InetNtopW(AF_INET, &IPAddr, addr.data(), addr.size());
		if (res == nullptr) {
			return {};
		}
		// Trim extra 0s at the end
		result.push_front(addr.c_str());
	}
	return result;
}

std::vector<char> Net::assemblePacket(const Net::Packet::Category category, std::span<char> packetData) {
	const Net::Packet::SizeType packetLen = Net::Packet::headerSize + static_cast<Net::Packet::SizeType>(packetData.size_bytes());
	std::vector<char> result(packetLen);
	std::span<char> resultData{ result.data(), packetLen };
	int offset = 0;
	writeUInt16Le(Net::Packet::protocolSignature, resultData, Net::Packet::signatureOffset);
	writeUInt8(static_cast<Net::Packet::CategoryType>(category), resultData, Net::Packet::categoryOffset);
	writeUInt16Le(packetLen, resultData, Net::Packet::sizeOffset);
	std::copy_n(packetData.data(), packetData.size_bytes(), result.data() + Net::Packet::dataOffset);
	return result;
}

bool Net::hasValidHeader(std::span<unsigned char> packet) {
	if (packet.size_bytes() < Packet::headerSize) {
		return false;
	}
	const Packet::SignatureType signature = readUInt16Le(packet, 0);
	if (signature != Packet::protocolSignature) {
		return false;
	}
	return true;
}

Net::Packet::Category Net::getPacketCategory(std::span<unsigned char> packet) {
	return static_cast<Net::Packet::Category>(readUInt8(packet, Net::Packet::categoryOffset));
}

std::optional<Keystroke> Net::getKeystroke(std::span<unsigned char> packet) {
	if (static_cast<int>(packet.size()) < Packet::dataOffset + Packet::Keystroke::dataSize) {
		return std::nullopt;
	}
	int offset = Packet::dataOffset;
	Packet::Keystroke::Key key = readUInt8(packet, offset);
	offset += sizeof(key);
	Packet::Keystroke::Mods mods = readUInt8(packet, offset);
	return Keystroke{ static_cast<int>(key), static_cast<int>(mods) };
}

std::optional<Audio::Bitrate> Net::getBitrate(std::span<unsigned char> packet) {
	if (static_cast<int>(packet.size()) < Packet::dataOffset + Packet::setFormatSize) {
		return std::nullopt;
	}
	int offset = Packet::dataOffset;
	Packet::BitrateType bitrate = readUInt8(packet, offset);
	return static_cast<Audio::Bitrate>(bitrate);
}
