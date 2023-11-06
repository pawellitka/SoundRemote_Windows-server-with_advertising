#pragma once

#include "NetDefines.h"
#include "Keystroke.h"

class Clients;
struct ClientInfo;

class Server {
	using KeystrokeCallback = std::function<void(const Keystroke& keystroke)>;
public:
	Server(int clientPort, int serverPort, boost::asio::io_context& ioContext, std::shared_ptr<Clients> clients);
	~Server();
	void onClientsUpdate(std::forward_list<ClientInfo> clients);
	void sendAudio(Audio::Compression compression, std::vector<char> data);
	void setKeystrokeCallback(KeystrokeCallback callback);
private:
	boost::asio::awaitable<void> receive(boost::asio::ip::udp::socket& socket);
	void processConnect(const Net::Address& address, const std::span<char>& packet);
	void processDisconnect(const Net::Address& address);
	void processSetFormat(const Net::Address& address, const std::span<char>& packet);
	void processKeystroke(const std::span<char>& packet) const;
	void processKeepAlive(const Net::Address& address) const;
	void send(const Net::Address& address, const std::shared_ptr<std::vector<char>> packet);
	void handleSend(const std::shared_ptr<std::vector<char>> packet, const boost::system::error_code& ec, std::size_t bytes);
	void keepalive();

	void startMaintenanceTimer();
	void maintain(boost::system::error_code ec);

	boost::asio::ip::udp::socket socketSend_;
	boost::asio::ip::udp::socket socketReceive_;
	boost::asio::steady_timer maintainenanceTimer_;
	int clientPort_;
	KeystrokeCallback keystrokeCallback_;
	std::shared_ptr<Clients> clients_;
	std::unordered_map<Audio::Compression, std::forward_list<Net::Address>> clientsCache_;
};
