#include <boost/asio.hpp>

#include "NetUtil.h"
#include "Util.h"
#include "Clients.h"
#include "Server.h"

using boost::asio::ip::udp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using namespace std::chrono_literals;

Server::Server(int clientPort, int serverPort, boost::asio::io_context& ioContext, std::shared_ptr<Clients> clients) :
    clientPort_(clientPort),
    clients_(clients),
    socketSend_(ioContext, udp::v4()),
    socketReceive_(ioContext, udp::endpoint(udp::v4(), serverPort)),
    maintainenanceTimer_(ioContext) {

    //co_spawn(ioContext, receive(std::move(socket)), detached);
    co_spawn(ioContext, receive(socketReceive_), detached);
    startMaintenanceTimer();
}

Server::~Server() {
    socketReceive_.shutdown(udp::socket::shutdown_receive);
    socketReceive_.close();
    socketSend_.shutdown(udp::socket::shutdown_send);
    socketSend_.close();
}

void Server::onClientsUpdate(std::forward_list<ClientInfo> clients) {
    std::unordered_map<Audio::Bitrate, std::forward_list<Net::Address>> newClients;
    for (auto&& client: clients) {
        if (!newClients.contains(client.bitrate)) {
            newClients[client.bitrate] = std::forward_list<Net::Address>();
        }
        newClients[client.bitrate].push_front(client.address);
    }
    clientsCache_ = std::move(newClients);
}

void Server::sendAudio(Audio::Bitrate bitrate, std::vector<char> data) {
    auto category = Net::Packet::Category::AudioDataOpus;
    if (bitrate == Audio::Bitrate::none) {
        category = Net::Packet::Category::AudioDataUncompressed;
    }
    auto packet = std::make_shared<std::vector<char>>(
        Net::assemblePacket(category, { data.data(), data.size() })
    );
    for (auto&& address : clientsCache_[bitrate]) {
        send(address, packet);
    }
}

void Server::setKeystrokeCallback(KeystrokeCallback callback) {
    keystrokeCallback_ = callback;
}

awaitable<void> Server::receive(udp::socket& socket) {
    //throw std::exception("Server::receive");
    std::array<unsigned char, Net::inputPacketSize> datagram{};
    udp::endpoint sender;
    //Have to handle errors here or write custom completion handler for co_spawn()
    try {
        for (;;) {
            auto nBytes = co_await socket.async_receive_from(boost::asio::buffer(datagram), sender, use_awaitable);
            std::span receivedData = { datagram.data(), nBytes };
            auto category = Net::getPacketCategory(receivedData);
            switch (category) {
            case Net::Packet::Category::Connect:
                processConnect(sender.address(), receivedData);
                break;
            case Net::Packet::Category::Disconnect:
                processDisconnect(sender.address());
                break;
            case Net::Packet::Category::SetFormat:
                processSetFormat(sender.address(), receivedData);
                break;
            case Net::Packet::Category::Keystroke:
                processKeystroke(receivedData);
                break;
            case Net::Packet::Category::ClientKeepAlive:
                processKeepAlive(sender.address());
                break;
            default:
                break;
            }
        }
    }
    catch (const boost::system::system_error& se) {
        if (se.code() != boost::asio::error::operation_aborted) {
            Util::showError(Util::contructAppExceptionText("Receive", se.what()));
            std::exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception& e) {
        Util::showError(Util::contructAppExceptionText("Receive", e.what()));
        std::exit(EXIT_FAILURE);
    }
    catch (...) {
        Util::showError("Receive: unknown error");
        std::exit(EXIT_FAILURE);
    }
}

void Server::processConnect(const Net::Address& address, const std::span<unsigned char> packet) {
    const auto connectData = Net::getConnectData(packet);
    if (!connectData) { return; }
    auto bitrate = Net::bitrateFromNetworkValue(connectData->bitrate);
    if (!bitrate) { return; }
    clients_->add(address, *bitrate);

    send(address, std::make_shared<std::vector<char>>(
        Net::createAckPacket(connectData->requestId)
    ));
}

void Server::processDisconnect(const Net::Address& address) {
    clients_->remove(address);
}

void Server::processSetFormat(const Net::Address& address, const std::span<unsigned char> packet) {
    const auto setFormatData = Net::getSetFormatData(packet);
    if (!setFormatData) { return; }
    auto bitrate = Net::bitrateFromNetworkValue(setFormatData->bitrate);
    if (!bitrate) { return; }
    clients_->setBitrate(address, *bitrate);

    send(address, std::make_shared<std::vector<char>>(
        Net::createAckPacket(setFormatData->requestId)
    ));
}

void Server::processKeystroke(const std::span<unsigned char> packet) const {
    const std::optional<Keystroke> keystroke = Net::getKeystroke(packet);
    if (!keystroke) { return; }
    keystroke->emulate();
    if (keystrokeCallback_) {
        keystrokeCallback_(*keystroke);
    }
}

void Server::processKeepAlive(const Net::Address& address) const {
    clients_->keep(address);
}

void Server::send(const Net::Address& address, std::shared_ptr<std::vector<char>> packet) {
    auto destination = udp::endpoint(address, clientPort_);
    socketSend_.async_send_to(boost::asio::buffer(packet->data(), packet->size()), destination,
        //shared_ptr with the packet is passed to keep data alive until the handler call
        [packet](boost::system::error_code ec, auto bytes) mutable {
            if (ec) {
                throw std::runtime_error(Util::contructAppExceptionText("Server send", ec.what()));
            }
        });
}

void Server::startMaintenanceTimer() {
    maintainenanceTimer_.expires_after(1s);
    maintainenanceTimer_.async_wait(std::bind(&Server::maintain, this, std::placeholders::_1));
}

void Server::maintain(boost::system::error_code ec) {
    if (ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        } else {
            throw std::runtime_error(Util::contructAppExceptionText("Timer maintain", ec.what()));
        }
    }
    keepalive();
    clients_->maintain();

    startMaintenanceTimer();
}

void Server::keepalive() {
    if (clientsCache_.empty()) { return; }
    auto packet = std::make_shared<std::vector<char>>(Net::assemblePacket(Net::Packet::Category::ServerKeepAlive));
    for (auto&& [bitrate, addresses] : clientsCache_) {
        for (auto&& address : addresses) {
            send(address, packet);
        }
    }
}
