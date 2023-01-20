#pragma once

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define NETWORK_RECVBUFF_SIZE (1024 * 1024 * 16)

struct ldiServer {
	std::thread				workerThread;
	std::atomic_bool		workerThreadRunning = true;

	SOCKET					listenSocket;
	SOCKET					clientSocket;

	uint8_t					recvBuffer[NETWORK_RECVBUFF_SIZE];
	int						recvSize;
	int						recvPacketState;
	int						recvPacketPayloadLen;

	uint8_t					packetBuffer[NETWORK_RECVBUFF_SIZE];
	int						packetSize;

	std::atomic_bool		packetAvailable = false;
	std::mutex				packetMutex;
	std::condition_variable	packetCondVar;

	std::mutex				waitForPacketMutex;
	std::condition_variable waitForPacketCondVar;
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

struct ldiProtocolSettings {
	ldiProtocolHeader header;
	int shutterSpeed;
	int analogGain;
};

struct ldiProtocolMode {
	ldiProtocolHeader header;
	int mode;
};

int networkInit(ldiServer* Server, const char* Port);
void networkDestroy(ldiServer* Server);
int networkUpdate(ldiServer* Server, ldiPacketView* PacketView);
int networkSend(ldiServer* Server, uint8_t* Data, int Size);
bool networkGetPacket(ldiServer* Server, ldiPacketView* PacketView);
bool networkWaitForPacket(ldiServer* Server, ldiPacketView* PacketView);
