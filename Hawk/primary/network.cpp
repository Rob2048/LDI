#include "network.h"

#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

struct ldiNet {
	int socket;
};

ldiNet* networkCreate() {
	ldiNet* result = new ldiNet;
	result->socket = -1;

	return result;
}

void networkDestroy(ldiNet* Net) {
	delete Net;
}

void networkDisconnect(ldiNet* Net) {
	if (Net->socket != -1) {
		std::cout << "Terminating socket\n";
		close(Net->socket);
		Net->socket = -1;
	}
}

int networkConnect(ldiNet* Net, const char* Hostname, int Port) {
	std::cout << "Connecting to: " << Hostname << ":" << Port << "\n";

	networkDisconnect(Net);

	int sockFd;
	struct sockaddr_in servaddr;

	sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockFd == -1) {
		std::cout << "Socket creation failed\n";
		return 1;
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(Hostname);
	servaddr.sin_port = htons(Port);

	fcntl(sockFd, F_SETFL, O_NONBLOCK);

	int connectResult = connect(sockFd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sockFd, &set);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	connectResult = select(sockFd + 1, NULL, &set, NULL, &timeout);

	// std::cout << "Select: " << connectResult << "\n";

	// int val = 69;
	// socklen_t len;
	// if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &val, &len) == -1) {
	// 	std::cout << "Getsocketopt error: " << errno << "\n";
	// } else {
	// 	std::cout << "Getsocketopt val: " << val << "\n";
	// }

	if (connectResult == 1) {
		// NOTE: Clear O_NONBLOCKING flag. (Will clear all flags)
		// fcntl(sockFd, F_SETFL, 0);
		Net->socket = sockFd;
		std::cout << "Connected to server\n";
		return 0;
	}
	// std::cout << "Select result: " << connectResult << "\n";
	// }

	// Failed.
	close(sockFd);
	return 1;

	// exit(1);

	// if (connectResult != 0) {
	// 	if (errno != EINPROGRESS) {
	// 		std::cout << "Could not connect to server\n";
	// 		return 1;
	// 	} else {
	// 		std::cout << "In progress\n";
	// 	}
	// }
}

int networkRead(ldiNet* Net, uint8_t* Data, int Size) {
	int readBytes = 0;

	while (true) {
		int result = read(Net->socket, Data + readBytes, Size - readBytes);

		if (result == -1) {
			if (errno == EAGAIN) {
				return readBytes;
			} else {
				std::cout << "Read -1 " << errno << "\n";
				break;
			}
		} else if (result > 0) {
			std::cout << "Read bytes: " << result << "\n";
			readBytes += result;
		} else if (result == 0) {
			break;
		}
	}

	networkDisconnect(Net);
	return -1;
}

int networkWrite(ldiNet* Net, uint8_t* Data, int Size) {
	// TODO: Make sure to write all bytes.
	int sent = 0;

	while (sent < Size) {
		int result = send(Net->socket, Data + sent, Size - sent, MSG_NOSIGNAL);

		// std::cout << "Send result: " << result << "\n";

		if (result == -1) {
			// std::cout << "Can't write to connection " << errno << "\n";

			if (errno == EAGAIN) {
				// TODO: Wait until socket becomes writable?
			} else {
				networkDisconnect(Net);
				return 1;
			}
		} else if (result <= Size) {
			sent += result;
			// std::cout << "Wrote bytes" << sent << "/" << Size << "\n";
			// networkDisconnect(Net);
			// return 1;
		}
	}

	return 0;
}
