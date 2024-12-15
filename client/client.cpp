#include <iostream>
#include <cassert>
#include <regex>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static int exec_prog(const char **argv)
{
    pid_t   my_pid;

    if (0 == (my_pid = fork())) {
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

    return 0;
}

int main(int argc, char* argv[]) {
	int port = 10001;
	if (argc > 2) {
		port = std::stoi(argv[2]);
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd != -1);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(argv[1]);

	assert(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0);

	while(true) {
		char buf[2048];
		int length = recv(sockfd, buf, sizeof(buf), 0);
		if (length == 0) {
			break;
		}
		std::string str { buf, length };
		std::regex r{"(.*);(.*);(.*)"};
		std::sregex_iterator it{str.begin(), str.end(), r};
		std::sregex_iterator end;
		for (; it != end; ++it) {
			std::smatch match = *it;
			std::string m1 = match[1].str();
			std::string m2 = match[2].str();
			const char* prog[] = {"/usr/bin/twmnc", "-t", m1.c_str(), "-c", m2.c_str(), 0};
			exec_prog(prog);
		}
	}
	return 0;
}
