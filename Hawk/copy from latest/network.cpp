#include "network.h"

#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>

const int CLIENT_BUFFER_SIZE = 1024 * 64;

struct ldiNetworkClient {
	int id;
	struct sockaddr_in addr;
	int socket;
	uint8_t rxBuffer[CLIENT_BUFFER_SIZE];
	int rxBufferPos = 0;
};

struct ldiNet {
	int socket;
	int listenSocket;
	int nextClientId;
	std::vector<ldiNetworkClient> clients;
};

ldiNet* networkInit() {
	ldiNet* result = new ldiNet;
	result->socket = -1;
	result->nextClientId = 1;

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

int networkListen(ldiNet* Net, int Port) {
	std::cout << "Starting listen server on port: " << Port << "\n";

	Net->listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (Net->listenSocket == -1) {
		std::cout << "Socket creation failed\n";
		return 1;
	}

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(Port);

	int enable = 1;
	if (setsockopt(Net->listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
		std::cout << "Socket setsockopt failed\n";
		return 1;
	}

	if (setsockopt(Net->listenSocket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) != 0) {
		std::cout << "Socket setsockopt failed\n";
		return 1;
	}

	if ((bind(Net->listenSocket, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		std::cout << "Socket bind failed\n";
		return 1;
	}

	if ((listen(Net->listenSocket, 4)) != 0) {
		std::cout << "Listen failed\n";
		return 1;
	}

	fcntl(Net->listenSocket, F_SETFL, O_NONBLOCK);
	
	std::cout << "Listening...\n";

	return 0;
}

int networkCloseClient(ldiNet* Net, int Idx) {
	close(Net->clients[Idx].socket);
	Net->clients.erase(Net->clients.begin() + Idx);

	return 0;
}

int networkClearClientRead(ldiNet* Net, int ClientID) {
	for (size_t i = 0; i < Net->clients.size(); i++) {
		if (Net->clients[i].id == ClientID) {
			Net->clients[i].rxBufferPos = 0;
			return 0;
		}
	}

	return 0;
}

ldiNetworkLoopResult networkServerLoop(ldiNet* Net) {
	ldiNetworkLoopResult result = {};
	result.type = ldiNetworkTypeFailure;

	struct pollfd fds[64];
	int nfds = Net->clients.size() + 1;

	fds[0].fd = Net->listenSocket;
	fds[0].events = POLLIN;

	for (size_t i = 0; i < Net->clients.size(); i++) {
		fds[i + 1].fd = Net->clients[i].socket;
		fds[i + 1].events = POLLIN;
	}

	int pollResult = poll(fds, nfds, 0);

	if (pollResult < 0) {
		std::cout << "Poll failed\n";
		result.type = ldiNetworkTypeFailure;
		return result;
	} else if (pollResult != 0) {
		// NOTE: Read all clients.
		// std::cout << "Got poll result: " << pollResult << "\n";

		if (fds[0].revents & POLLIN) {
			std::cout << "New client\n";

			struct sockaddr_in cli;
			socklen_t len = sizeof(cli);

			int clientFd = accept(Net->listenSocket, (struct sockaddr*)&cli, &len);
			if (clientFd < 0) {
				std::cout << "Server accept failed\n";
				result.type = ldiNetworkTypeFailure;
				return result;
			} else {
				std::cout << "Server accepted client\n";

				fcntl(clientFd, F_SETFL, O_NONBLOCK);

				ldiNetworkClient client;
				client.socket = clientFd;
				client.addr = cli;
				client.id = Net->nextClientId++;
				Net->clients.push_back(client);

				result.type = ldiNetworkTypeClientNew;
				result.id = client.id;
				return result;
			}
		}

		for (size_t i = 0; i < Net->clients.size(); i++) {
			int revents = fds[i + 1].revents;
			ldiNetworkClient* client = &Net->clients[i];
			result.id = client->id;

			if (revents & POLLIN) {
				while (true) {
					// std::cout << "Client " << i << " has data\n";
					int readResult = read(client->socket, client->rxBuffer + client->rxBufferPos, CLIENT_BUFFER_SIZE - client->rxBufferPos);

					if (readResult == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							// Read all bytes, do processing.
							// std::cout << "Read all bytes\n";
							result.type = ldiNetworkTypeClientRead;
							result.data = client->rxBuffer;
							result.size = client->rxBufferPos;
							return result;
						} else {
							std::cout << "Client socket read error: " << errno << "\n";
							networkCloseClient(Net, i);
							result.type = ldiNetworkTypeClientLost;
							return result;
						}
					} else if (readResult > 0) {
						std::cout << "Read bytes: " << readResult << "\n";
						client->rxBufferPos += readResult;

						if (client->rxBufferPos == CLIENT_BUFFER_SIZE) {
							// std::cout << "Client receive buffer full\n";
							networkCloseClient(Net, i);
							result.type = ldiNetworkTypeClientLost;
							return result;
						}
					} else if (readResult == 0) {
						// std::cout << "Read result == 0\n";
						networkCloseClient(Net, i);
						result.type = ldiNetworkTypeClientLost;
						return result;
					}
				}
			} else if (revents & POLLHUP || revents & POLLERR) {
				// std::cout << "Client " << i << " has hung up or had error\n";
				networkCloseClient(Net, i);
				result.type = ldiNetworkTypeClientLost;
				return result;
			}
		}
	}

	result.type = ldiNetworkTypeNone;
	return result;
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

int networkClientWrite(ldiNet* Net, int ClientId, uint8_t* Data, int Size) {
	ldiNetworkClient* client = nullptr;

	for (size_t i = 0; i < Net->clients.size(); i++) {
		if (Net->clients[i].id == ClientId) {
			client = &Net->clients[i];
			break;
		}
	}

	if (client == nullptr) {
		std::cout << "Client not found\n";
		return 1;
	}

	int sent = 0;

	while (sent < Size) {
		int result = send(client->socket, Data + sent, Size - sent, MSG_NOSIGNAL);

		// std::cout << "Send result: " << result << "\n";

		if (result == -1) {
			// std::cout << "Can't write to connection " << errno << "\n";

			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// TODO: Wait until socket becomes writable.
			} else {
				// TODO: Should disconnect client here, but possibly caught in next poll cycle?
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
