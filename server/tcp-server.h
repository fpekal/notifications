#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

template<class Func>
class tcp_server {
public:
	tcp_server(Func func, int port) : func(func), port(port) {
		listen_thread = std::jthread(&tcp_server::listen_thread_func, this);
		cleanup_thread = std::jthread(&tcp_server::cleanup_thread_func, this);
	}


private:
	void listen_thread_func() {
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		assert(socket_fd != -1);

		int reuse = 1;
		int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		assert(ret != -1);

		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(port);

		ret = bind(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
		assert(ret != -1);

		ret = listen(socket_fd, 16);
		assert(ret != -1);

		while (true) {
			struct sockaddr_in client_address;
			socklen_t client_address_size = sizeof(client_address);
			int client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_address, &client_address_size);
			auto finished_flag = std::make_shared<bool>(false);
			std::jthread client_thread = std::jthread(&tcp_server::client_thread_func, this, client_socket_fd, finished_flag);
			client_mutex.lock();
			clients.emplace_back(finished_flag, std::move(client_thread));
			client_mutex.unlock();
		}
	}

	void client_thread_func(int client_socket_fd, std::shared_ptr<bool> finished_flag) {
		func(client_socket_fd);
		close(client_socket_fd);

		client_mutex.lock();
		*finished_flag = true;
		finished = true;
		client_cv.notify_one();
		client_mutex.unlock();
	}

	void cleanup_thread_func() {
		while(true) {
			std::unique_lock<std::mutex> lock(client_mutex);
			client_cv.wait(lock, [this] { return finished; });
			finished = false;

			std::erase_if(clients, [](const std::pair<std::shared_ptr<bool>, std::jthread>& client) {
				return *client.first;
			});
		}
	}

	Func func;
	int port;

	int socket_fd;
	std::jthread listen_thread;
	std::jthread cleanup_thread;
	std::vector<std::pair<std::shared_ptr<bool>, std::jthread>> clients;

	std::mutex client_mutex;
	std::condition_variable client_cv;
	bool finished = false;
};
