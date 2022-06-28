#pragma once

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define NETWORK_RECVBUFF_SIZE (1024 * 1024 * 16)

struct ldiServer {
	SOCKET listenSocket;
	SOCKET clientSocket;

	uint8_t recvBuffer[NETWORK_RECVBUFF_SIZE];
	int recvSize;
	int recvPacketState;
	int recvPacketPayloadLen;
};

struct ldiPacketView {
	uint8_t* data;
	int size;
};

struct ldiProtocolHeader {
	int packetSize;
	int opcode;
};

struct ldiProtocolImageHeader {
	ldiProtocolHeader header;
	int width;
	int height;
};

int networkInit(ldiServer* Server, const char* Port);
int networkUpdate(ldiServer* Server, ldiPacketView* PacketView);
