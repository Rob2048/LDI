#include "network.h"
#pragma comment (lib, "Ws2_32.lib")

int networkInit(ldiServer* Server, const char* Port) {
	WSADATA wsaData;
	int iResult;

	Server->listenSocket = INVALID_SOCKET;
	Server->clientSocket = INVALID_SOCKET;

	std::cout << "Starting server - Port: " << Port << "\n" << std::flush;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "Net startup failed: " << iResult << "\n";
		return false;
	}

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, Port, &hints, &result);
	if (iResult != 0) {
		std::cout << "Getaddrinfo failed: " << iResult << "\n";
		WSACleanup();
		return false;
	}

	Server->listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (Server->listenSocket == INVALID_SOCKET) {
		std::cout << "Listen socket failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	u_long nonBlocking = 1;
	if (ioctlsocket(Server->listenSocket, FIONBIO, &nonBlocking) != 0) {
		std::cout << "Could not set blocking\n";
		freeaddrinfo(result);
		closesocket(Server->listenSocket);
		WSACleanup();
		return false;
	}
	//setsockopt(listenSocket, 0, NOBLOCK

	iResult = bind(Server->listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Bind failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(Server->listenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	iResult = listen(Server->listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Listen failed: " << WSAGetLastError() << "\n";
		closesocket(Server->listenSocket);
		WSACleanup();
		return false;
	}

	std::cout << "Net started successfully\n" << std::flush;

	//closesocket(listenSocket);
}

void _initClient(ldiServer* Server, SOCKET Socket) {
	Server->clientSocket = Socket;
	Server->recvSize = 0;
	Server->recvPacketState = 0;
	Server->recvPacketPayloadLen = 0;
}

void _terminateClient(ldiServer* Server) {
	if (Server->clientSocket != INVALID_SOCKET) {
		closesocket(Server->clientSocket);
	}

	Server->clientSocket = INVALID_SOCKET;
}

u_long _checkRecvBytes(SOCKET Socket) {
	u_long result = 0;
	
	if (ioctlsocket(Socket, FIONREAD, &result) < 0) {
		std::cout << "Recv read bytes error\n";
		exit(1);
	}

	return result;
}

int networkUpdate(ldiServer* Server, ldiPacketView* PacketView) {
	// Accept
	
	SOCKET acceptResult = accept(Server->listenSocket, NULL, NULL);
	
	if (acceptResult == INVALID_SOCKET) {
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			// NOTE: Just blocking...
		} else {
			// TODO: Handle more critical error.
			std::cout << "Accept socket error: " << error << "\n";
			exit(1);
		}
	} else {
		// NOTE: Got client socket.
		if (Server->clientSocket != INVALID_SOCKET) {
			std::cout << "Warning: client already connected!\n";
		}

		_initClient(Server, acceptResult);
		std::cout << "Got client connection " << (int)Server->clientSocket << "\n";
	}

	if (Server->clientSocket != INVALID_SOCKET) {
		while (true) {
			if (Server->recvPacketState == 0) {
				// NOTE: Packet header (Int: payload len).
				int recvBytes = recv(Server->clientSocket, (char*)Server->recvBuffer + Server->recvSize, 4 - Server->recvSize, 0);

				if (recvBytes == 0) {
					// NOTE: Connection is gracefully terminated.
					_terminateClient(Server);
					std::cout << "Client disconnected\n";
					break;
				} else if (recvBytes == SOCKET_ERROR) {
					int error = WSAGetLastError();

					if (error == WSAEWOULDBLOCK) {
						// NOTE: Just blocking, so terminate loop.
					} else {
						std::cout << "Critical socket error: " << error << "\n";
						_terminateClient(Server);
					}

					break;
				} else { 
					// NOTE: Valid data read.
					//std::cout << "Header bytes: " << recvBytes << "\n";

					Server->recvSize += recvBytes;

					if (Server->recvSize == 4) {
						Server->recvPacketPayloadLen = *(int*)(&Server->recvBuffer[0]);
						Server->recvPacketState = 1;
						//std::cout << "Got packet length: " << Server->recvPacketPayloadLen << "\n";
					}
				}
			}
			
			if (Server->recvPacketState == 1) {
				// NOTE: Packet payload.
				int bytesToRecv = Server->recvPacketPayloadLen + 4 - Server->recvSize;
				int recvBytes = recv(Server->clientSocket, (char*)Server->recvBuffer + Server->recvSize, bytesToRecv, 0);

				if (recvBytes == 0) {
					// NOTE: Connection is gracefully terminated.
					_terminateClient(Server);
					std::cout << "Client disconnected\n";
					break;
				} else if (recvBytes == SOCKET_ERROR) {
					int error = WSAGetLastError();

					if (error == WSAEWOULDBLOCK) {
						// NOTE: Just blocking, so terminate loop.
					} else {
						std::cout << "Critical socket error: " << error << "\n";
						_terminateClient(Server);
					}

					break;
				} else {
					// NOTE: Valid data read.
					//std::cout << "Payload bytes: " << recvBytes << "\n";
					Server->recvSize += recvBytes;

					if (Server->recvSize == Server->recvPacketPayloadLen + 4) {
						//std::cout << "Got entire packet\n";
						PacketView->data = Server->recvBuffer;
						PacketView->size = Server->recvSize;
						Server->recvPacketState = 0;
						Server->recvSize = 0;

						return 1;
					}
				}
			}
		}
	}

	return 0;
}