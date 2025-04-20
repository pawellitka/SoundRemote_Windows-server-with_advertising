#include <forward_list>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "AudioUtil.h"
#include "Keystroke.h"
#include "NetDefines.h"

namespace Net {
	/// <summary>
	/// Gets string representations of the IPv4 IP addresses.
	/// </summary>
	/// <returns>List of addresses or an empty list if an error occurs.</returns>
	std::forward_list<std::wstring> getLocalAddresses();
	/// <summary>
	/// Gets raw IPv4 IP addresses.
	/// </summary>
	/// <param name="addrTableSize">address of the list of the addresses</param>
	/// <returns>List of addresses.</returns>
	std::vector<char> getRawLocalAddressesTable(DWORD* addrTableSize);

	/// <summary>
	/// Converts a compression value used in the network protocol to a value of the <c>Audio::Compression</c> enum.
	/// </summary>
	/// <param name="compression">Network protocol compression value</param>
	/// <returns><c>std::optional</c> containing an <c>Audio::Compression</c> value or <c>std::nullopt</c> if the passed
	/// argument is not a valid compression value.</returns>
	std::optional<Audio::Compression> compressionFromNetworkValue(Net::Packet::CompressionType compression);

	std::vector<char> createAudioPacket(
		Net::Packet::Category category,
		Net::Packet::SequenceNumberType sequenceNumber,
		const std::span<const char>& audioData
		);
	std::vector<char> createKeepAlivePacket();
	std::vector<char> createAdvertisePacket();
	std::vector<char> createDisconnectPacket();
	std::vector<char> createAckConnectPacket(Net::Packet::RequestIdType requestId);
	std::vector<char> createAckSetFormatPacket(Net::Packet::RequestIdType requestId);

	Net::Packet::Category getPacketCategory(const std::span<char>& packet);
	std::optional<Keystroke> getKeystroke(const std::span<char>& packet);
	std::optional<Net::Packet::ConnectData> getConnectData(const std::span<char>& packet);
	std::optional<Net::Packet::SetFormatData> getSetFormatData(const std::span<char>& packet);
};
