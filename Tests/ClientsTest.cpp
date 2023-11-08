#include <barrier>
#include <forward_list>
#include <functional>
#include <memory>
#include <thread>

#include "pch.h"
#include "Clients.h"

namespace {
	using namespace std::placeholders;

	class ClientsListener {
	public:
		virtual void onClientsUpdate(std::forward_list<ClientInfo> clients) = 0;
	};

	class MockClientsListener : public ClientsListener {
	public:
		MOCK_METHOD(void, onClientsUpdate, (std::forward_list<ClientInfo> clients), (override));
	};

    class ClientsTest : public testing::Test {
    protected:
        void SetUp() override {
            clients_ = std::make_unique<Clients>();
        }
		void TearDown() override {
			clients_.release();
		}
        std::unique_ptr<Clients> clients_;
    };

	TEST_F(ClientsTest, Add) {
		const auto threadCount = 5;
		const auto threadAdds = 50;
		MockClientsListener listener;
		EXPECT_CALL(listener, onClientsUpdate).Times(threadCount * threadAdds);

		clients_->addClientsListener(std::bind(&MockClientsListener::onClientsUpdate, &listener, _1));
		std::barrier barrier(threadCount);
		auto addClients = [&](int start) {
			barrier.arrive_and_wait();
			for (int i = start; i < start + threadAdds; i++) {
				auto address = "192.168.0." + std::to_string(i);
				clients_->add(boost::asio::ip::make_address_v4(address), Audio::Compression::none);
			}
			};
		std::vector<std::jthread> threads(threadCount);
		for (auto i = 0; i < threadCount * threadAdds; i+= threadAdds) {
			threads.emplace_back(addClients, i);
		}
	}
}