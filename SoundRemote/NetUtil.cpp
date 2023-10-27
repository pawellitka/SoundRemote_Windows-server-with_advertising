#include <vector>
#include <array>
#include <forward_list>
#include <string>
#include <cassert>
#include <boost/asio.hpp>

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

	void writeUInt16Le(uint16_t value, std::span<unsigned char> dest, size_t offset) {
		assert((offset + 2) <= dest.size_bytes());
		dest[offset] = value >> 0;
		dest[offset + 1] = value >> 8;
	}

	void writeUInt8(uint8_t value, std::span<unsigned char> dest, size_t offset) {
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
	IN_ADDR IPAddr;
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

std::vector<unsigned char> Net::assemblePacket(const Net::Packet::type_t packetType, std::span<unsigned char> packetData) {
	const Net::Packet::len_t packetLen = Net::Packet::HEADER_SIZE + static_cast<Net::Packet::len_t>(packetData.size_bytes());
	std::vector<unsigned char> result(packetLen);
	std::span<unsigned char> resultData{ result.data(), packetLen };
	int offset = 0;
	writeUInt16Le(Net::Packet::PROTOCOL_SIGNATURE, resultData, Net::Packet::SIGNATURE_OFFSET);
	writeUInt8(packetType, resultData, Net::Packet::TYPE_OFFSET);
	writeUInt16Le(packetLen, resultData, Net::Packet::LEN_OFFSET);
	std::copy_n(packetData.data(), packetData.size_bytes(), result.data() + Net::Packet::DATA_OFFSET);
	return result;
}

bool Net::hasValidHeader(std::span<unsigned char> packet) {
	if (packet.size_bytes() < Packet::HEADER_SIZE) {
		return false;
	}
	const Packet::signature_t signature = readUInt16Le(packet, 0);
	if (signature != Packet::PROTOCOL_SIGNATURE) {
		return false;
	}
	return true;
}

Net::Packet::type_t Net::getPacketType(std::span<unsigned char> packet) {
	Packet::type_t result = readUInt8(packet, Packet::TYPE_OFFSET);
	return result;
}

std::optional<Keystroke> Net::getKeystroke(std::span<unsigned char> packet) {
	if (packet.size() < Packet::DATA_OFFSET + Packet::Keystroke::DATA_SIZE) {
		return std::nullopt;
	}
	int offset = Packet::DATA_OFFSET;
	Packet::Keystroke::key_t key = readUInt32Le(packet, offset);
	offset += sizeof(key);
	Packet::Keystroke::mods_t mods = readUInt32Le(packet, offset);
	return Keystroke{ static_cast<int>(key), static_cast<int>(mods) };
}
