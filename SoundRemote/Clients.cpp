#include "Clients.h"

Clients::Clients(int timeoutSeconds) : timeoutSeconds_(timeoutSeconds) {}

void Clients::add(const Net::Address& address, Audio::Compression compression) {
	const std::unique_lock lock(clientsMutex_);
	if (clients_.contains(address)) {
		clients_[address]->updateLastContact();
		if (clients_[address]->compression() == compression) {
			return;
		}
		clients_[address]->setCompression(compression);
		onClientsUpdate();
	} else {
		clients_[address] = std::make_unique<Client>(compression);		
		onClientsUpdate();
	}
}

void Clients::setCompression(const Net::Address& address, Audio::Compression compression) {
	const std::unique_lock lock(clientsMutex_);
	if (!clients_.contains(address) || clients_[address]->compression() == compression) {
		return;
	}
	clients_[address]->setCompression(compression);
	onClientsUpdate();
}

void Clients::keep(const Net::Address& address) {
	const std::unique_lock lock(clientsMutex_);
	if (!clients_.contains(address)) {
		return;
	}
	clients_[address]->updateLastContact();
}

void Clients::remove(const Net::Address& address) {
	const std::unique_lock lock(clientsMutex_);
	if (clients_.erase(address)) {
		onClientsUpdate();
	}
}

void Clients::addClientsListener(ClientsUpdateCallback listener) {
	clientsListeners_.push_front(listener);
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
	if (clientsListeners_.empty()) { return; }
	std::forward_list<ClientInfo> clientInfos;
	for (auto&& client : clients_) {
		clientInfos.push_front({ client.first, client.second->compression() });
	}
	for (auto&& listener: clientsListeners_) {
		listener(clientInfos);
	}
}

// Client

Clients::Client::Client(Audio::Compression compression): compression_(compression) {
	updateLastContact();
}

void Clients::Client::updateLastContact() {
	lastContact_ = std::chrono::steady_clock::now();
}

void Clients::Client::setCompression(Audio::Compression compression) {
	compression_ = compression;
}

Clients::TimePoint Clients::Client::lastContact() const {
	return lastContact_;
}

Audio::Compression Clients::Client::compression() const {
	return compression_;
}

bool operator==(const ClientInfo& lhs, const ClientInfo& rhs) {
	return lhs.address == rhs.address &&
		lhs.compression == rhs.compression;
}