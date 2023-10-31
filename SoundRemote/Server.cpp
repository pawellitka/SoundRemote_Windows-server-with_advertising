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

Server::Server(int clientPort, int serverPort, boost::asio::io_context& ioContext) :
    clientPort_(clientPort),
    socketSend_(ioContext, udp::v4()),
    maintainenanceTimer_(ioContext) {
    // Create receiving socket
    socketReceive_ = std::make_unique<udp::socket>(ioContext, udp::endpoint(udp::v4(), serverPort));

    //co_spawn(ioContext, receive(std::move(socket)), detached);
    clientList_ = std::make_unique<ClientList>();
    co_spawn(ioContext, receive(*socketReceive_), detached);
    startMaintenanceTimer();
}

Server::~Server() {
    socketReceive_->shutdown(udp::socket::shutdown_receive);
    socketReceive_->close();
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
    clients_ = std::move(newClients);
}

void Server::sendOpusPacket(std::span<char> data) {
    auto opusPacket = std::make_shared<std::vector<char>>(Net::assemblePacket(Net::Packet::AudioDataOpus, data));
    send(opusPacket);
}

void Server::sendAudio(Audio::Bitrate bitrate, std::vector<char> data) {
    auto category = Net::Packet::AudioDataOpus;
    if (bitrate == Audio::Bitrate::none) {
        category = Net::Packet::AudioDataPcm;
    }
    auto packet = std::make_shared<std::vector<char>>(
        Net::assemblePacket(category, { data.data(), data.size() })
    );
    for (auto&& address : clients_[bitrate]) {
        send(address, packet);
    }
}

void Server::setClientListCallback(ClientsUpdateCallback callback) {
    clientList_->setClientListCallback(callback);
}

void Server::setKeystrokeCallback(KeystrokeCallback callback) {
    keystrokeCallback_ = callback;
}

bool Server::hasClients() const {
    return !clientList_->empty();
}

awaitable<void> Server::receive(udp::socket& socket) {
    //throw std::exception("Server::receive");
    std::array<unsigned char, Net::inputPacketSize> datagram;
    udp::endpoint sender;
    //Have to handle errors here or write custom completion handler for co_spawn()
    try {
        for (;;) {
            auto nBytes = co_await socket.async_receive_from(boost::asio::buffer(datagram), sender, use_awaitable);
            const bool parseSuccess = parsePacket({ datagram.data(), nBytes });
            if (parseSuccess) {
                clientList_->keepClient(sender.address());
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

bool Server::parsePacket(const std::span<unsigned char> packet) const {
    if (!Net::hasValidHeader(packet)) {
        return false;
    }
    const auto packetType = Net::getPacketCategory(packet);
    switch (packetType) {
    case Net::Packet::ClientCommand: {
        const std::optional<Keystroke> keystroke = Net::getKeystroke(packet);
        if (keystroke) {
            keystroke->emulate();
            if (keystrokeCallback_) {
                keystrokeCallback_(keystroke.value());
            }
        } else {
            return false;
        }
    }
        break;
    case Net::Packet::ClientKeepAlive:
        break;
    default:
        // Unrecognized packet
        return false;
    }
    return true;
}

void Server::send(std::shared_ptr<std::vector<char>> packet) {
    auto destination = udp::endpoint(udp::v4(), clientPort_);
    for (const auto& clientAddress : clientList_->clients()) {
        destination.address(clientAddress);
        socketSend_.async_send_to(boost::asio::buffer(packet->data(), packet->size()), destination,
            //shared_ptr with the packet is passed to keep data alive until the handler call
            [packet](boost::system::error_code ec, auto bytes) mutable {
                if (ec) {
                    throw std::runtime_error(Util::contructAppExceptionText("Send", ec.what()));
                }
            });
    }
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

void Server::sendKeepAlive() {
    if (!hasClients()) { return; }
    auto packet = std::make_shared<std::vector<char>>(Net::assemblePacket(Net::Packet::ServerKeepAlive));
    send(packet);
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
    clientList_->maintain();
    sendKeepAlive();

    startMaintenanceTimer();
}

//-------ClientList------

Server::ClientList::ClientList(int clientTimeoutSeconds) :
    timeoutSeconds_(clientTimeoutSeconds) {
}

void Server::ClientList::maintain() {
    bool updated = false;
    const auto now = std::chrono::steady_clock::now();
    for (auto it = clients_.begin(); it != clients_.end();) {
        std::chrono::duration<double> elapsedSeconds = now - it->second;
        if (elapsedSeconds.count() > timeoutSeconds_) {
            it = clients_.erase(it);
            updated = true;
        } else {
            ++it;
        }
    }
    if (updated) onUpdate();
}

std::forward_list<Server::address_t> Server::ClientList::clients() const {
    std::forward_list<address_t> result;
    for (auto&& elem : clients_) {
        result.push_front(elem.first);
    }
    return result;
}

bool Server::ClientList::empty() const {
    return clients_.empty();
}

void Server::ClientList::keepClient(address_t clientAddress) {
    const bool newClient = !clients_.contains(clientAddress);
    clients_[clientAddress] = std::chrono::steady_clock::now();
    if (newClient) {
        onUpdate();
    }
}

void Server::ClientList::setClientListCallback(ClientsUpdateCallback callback) {
    updateCallback_ = callback;
}

void Server::ClientList::onUpdate() const {
    if (updateCallback_) {
        std::forward_list<std::string> addresses;
        for (auto&& elem : clients_) {
            addresses.push_front(elem.first.to_string());
        }
        updateCallback_(addresses);
    }
}
