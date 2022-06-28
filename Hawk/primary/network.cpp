#include "network.h"

#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

	if (connect(sockFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
		std::cout << "Could not connect to server\n";
		return 1;
	}

	Net->socket = sockFd;

	std::cout << "Connected to server\n";

	return 0;
}

int networkWrite(ldiNet* Net, uint8_t* Data, int Size) {
	// TODO: Make sure to write all bytes.

	int result = send(Net->socket, Data, Size, MSG_NOSIGNAL);

	std::cout << "Written: " << result << "\n";

	if (result == -1) {
		std::cout << "Can't write to connection\n";
		networkDisconnect(Net);
		return 1;
	} else if (result < Size) {
		std::cout << "Didn't write all bytes!\n";
		networkDisconnect(Net);
		return 1;
	}

	return 0;
}
