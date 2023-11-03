#include <span>
#include <optional>

#include "AudioUtil.h"
#include "NetDefines.h"
#include "keystroke.h"

namespace Net {
	/// <summary>
	/// Gets string representations of the IPv4 IP addresses.
	/// </summary>
	/// <returns>List of addresses or an empty list if an error occurs.</returns>
	std::forward_list<std::wstring> getLocalAddresses();

	/// <summary>
	/// Converts a bitrate value used in the network protocol to a value of the <c>Audio::Bitrate</c> enum.
	/// </summary>
	/// <param name="bitrate">Network protocol bitrate value</param>
	/// <returns><c>std::optional</c> containing an <c>Audio::Bitrate</c> value or <c>std::nullopt</c> if the passed
	/// argument is not a valid bitrate value.</returns>
	std::optional<Audio::Bitrate> bitrateFromNetworkValue(Net::Packet::BitrateType bitrate);

	std::vector<char> assemblePacket(const Net::Packet::Category category, std::span<char> data = {});
	std::vector<char> createAckPacket(Net::Packet::RequestIdType requestId);

	bool hasValidHeader(std::span<unsigned char> packet);
	Net::Packet::Category getPacketCategory(std::span<unsigned char> packet);
	std::optional<Keystroke> getKeystroke(std::span<unsigned char> packet);
	std::optional<Net::Packet::ConnectData> getConnectData(std::span<unsigned char> packet);
	std::optional<Net::Packet::SetFormatData> getSetFormatData(std::span<unsigned char> packet);
};
