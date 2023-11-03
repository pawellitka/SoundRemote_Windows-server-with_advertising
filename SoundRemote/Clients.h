#pragma once

#include "AudioUtil.h"
#include "NetDefines.h"

struct ClientInfo;

class Clients {
	using TimePoint = std::chrono::steady_clock::time_point;
	using ClientsUpdateCallback = std::function<void(std::forward_list<ClientInfo>)>;
	using BitratesUpdateCallback = std::function<void(std::set<Audio::Bitrate>)>;
public:
	Clients(int timeoutSeconds = 5);
	void add(const Net::Address& address, Audio::Bitrate bitrate);
	void setBitrate(const Net::Address& address, Audio::Bitrate bitrate);
	void keep(const Net::Address& address);
	void remove(const Net::Address& address);
	void addClientsListener(ClientsUpdateCallback listener);
	void setBitrateListener(BitratesUpdateCallback listener);
	void maintain();

private:
	class Client {
	public:
		Client(Audio::Bitrate bitrate);
		void updateLastContact();
		void setBitrate(Audio::Bitrate bitrate);
		TimePoint lastContact() const;
		Audio::Bitrate bitrate() const;
	private:
		Audio::Bitrate bitrate_ = Audio::Bitrate::none;
		TimePoint lastContact_{};
	};
	
	void onClientsUpdate();
	void onBitratesUpdate();

	const int timeoutSeconds_;
	std::unordered_map<Net::Address, std::unique_ptr<Client>> clients_;
	std::shared_mutex clientsMutex_;
	std::set<Audio::Bitrate> usedBitrates_;
	// listeners are not synchronized, they are modified only on program start
	std::forward_list<ClientsUpdateCallback> clientsListeners_;
	BitratesUpdateCallback bitratesListener_;
};

struct ClientInfo {
	Net::Address address;
	Audio::Bitrate bitrate;
	ClientInfo(Net::Address addr, Audio::Bitrate br) : address(addr), bitrate(br) {}
};
