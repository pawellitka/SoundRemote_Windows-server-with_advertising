#pragma once

#include "NetDefines.h"
#include "Keystroke.h"

class Clients;
struct ClientInfo;

class Server {
	using ClientsUpdateCallback = std::function<void(std::forward_list<std::string>)>;
	using KeystrokeCallback = std::function<void(const Keystroke& keystroke)>;
public:
	Server(int clientPort, int serverPort, boost::asio::io_context& ioContext, std::shared_ptr<Clients> clients);
	~Server();
	void onClientsUpdate(std::forward_list<ClientInfo> clients);
	void sendAudio(Audio::Bitrate bitrate, std::vector<char> data);
	void setKeystrokeCallback(KeystrokeCallback callback);
private:
	boost::asio::awaitable<void> receive(boost::asio::ip::udp::socket& socket);
	void processConnect(const Net::Address& address, const std::span<unsigned char> packet);
	void processDisconnect(const Net::Address& address);
	void processSetFormat(const Net::Address& address, const std::span<unsigned char> packet);
	void processKeystroke(const std::span<unsigned char> packet) const;
	void processKeepAlive(const Net::Address& address) const;
	void send(const Net::Address& address, std::shared_ptr<std::vector<char>> packet);
	void keepalive();

	void startMaintenanceTimer();
	void maintain(boost::system::error_code ec);

	boost::asio::ip::udp::socket socketSend_;
	boost::asio::ip::udp::socket socketReceive_;
	boost::asio::steady_timer maintainenanceTimer_;
	int clientPort_;
	KeystrokeCallback keystrokeCallback_;
	std::shared_ptr<Clients> clients_;
	std::unordered_map<Audio::Bitrate, std::forward_list<Net::Address>> clientsCache_;
};
