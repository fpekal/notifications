#pragma once
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class tcp_client {
public:
	tcp_client(const std::string& host, int port) {
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		assert(socket_fd != -1);
		
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(host.c_str());
		server_address.sin_port = htons(port);
		
		int ret = connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
		assert(ret != -1);
	}

	std::string send(const std::string& message) {
		send(socket_fd, message.c_str(), message.size(), 0);
		char buffer[1024];
		int length = recv(socket_fd, buffer, 1024, 0);
		buffer[length] = '\0';
		return std::string(buffer);
	}

private:
	int socket_fd;
}
