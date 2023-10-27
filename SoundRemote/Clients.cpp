#include <set>
#include <unordered_map>
#include <boost/asio.hpp>

#include "AudioUtil.h"
#include "NetUtil.h"
#include "Clients.h"

Clients::Clients(int timeoutSeconds) : timeoutSeconds_(timeoutSeconds) {
}

void Clients::add(Net::Address address, Audio::Bitrate bitrate) {
	const std::unique_lock lock(clientsMutex_);
	if (clients_.contains(address)) {
		if (clients_[address]->bitrate() == bitrate) {
			return;
		}
		clients_[address]->setBitrate(bitrate);
		onClientsUpdate();
	} else {
		clients_[address] = std::make_unique<Client>(bitrate);		
		onClientsUpdate();
	}
}

void Clients::setBitrate(Net::Address address, Audio::Bitrate bitrate) {
	const std::unique_lock lock(clientsMutex_);
	if (!clients_.contains(address) || clients_[address]->bitrate() == bitrate) {
		return;
	}
	clients_[address]->setBitrate(bitrate);
	onClientsUpdate();
}

void Clients::keep(Net::Address address) {
	const std::unique_lock lock(clientsMutex_);
	if (!clients_.contains(address)) {
		return;
	}
	clients_[address]->updateLastContact();
}

void Clients::addClientsListener(ClientsUpdateCallback listener) {
	clientsListeners_.push_front(listener);
}

void Clients::setBitrateListener(BitratesUpdateCallback listener) {
	bitratesListener_ = listener;
}

void Clients::maintain() {
	const std::unique_lock lock(clientsMutex_);
	bool clientRemoved = false;
	const auto now = std::chrono::steady_clock::now();
	for (auto&& it = clients_.begin(); it != clients_.end();) {
		std::chrono::duration<float> elapsedSeconds = now - it->second->lastContact();
		if (elapsedSeconds.count() > timeoutSeconds_) {
			it = clients_.erase(it);
			clientRemoved = true;
		} else {
			++it;
		}
	}
	if (clientRemoved) {
		onClientsUpdate();
	}
}

void Clients::onClientsUpdate() {
	//onBitratesUpdate();
	if (clientsListeners_.empty()) { return; }
	std::forward_list<ClientInfo> clientInfos;
	for (auto&& client : clients_) {
		clientInfos.push_front({ client.first, client.second->bitrate() });
	}
	for (auto&& listener: clientsListeners_) {
		listener(clientInfos);
	}
}

void Clients::onBitratesUpdate() {
	if (!bitratesListener_) { return; }
	std::set<Audio::Bitrate> bitrates;
	for (auto&& client : clients_) {
		bitrates.insert(client.second->bitrate());
	}
	if (bitrates != usedBitrates_) {
		usedBitrates_ = bitrates;
		bitratesListener_(bitrates);
	}
}

// Client

Clients::Client::Client(Audio::Bitrate bitrate): bitrate_(bitrate) {
	updateLastContact();
}

void Clients::Client::updateLastContact() {
	lastContact_ = std::chrono::steady_clock::now();
}

void Clients::Client::setBitrate(Audio::Bitrate bitrate) {
	bitrate_ = bitrate;
}

Clients::TimePoint Clients::Client::lastContact() const {
	return lastContact_;
}

Audio::Bitrate Clients::Client::bitrate() const {
	return bitrate_;
}
