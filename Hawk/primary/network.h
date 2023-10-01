#pragma once

#include <stdint.h>

struct ldiNet;

enum ldiNetworkType {
	ldiNetworkTypeFailure,
	ldiNetworkTypeNone,
	ldiNetworkTypeClientNew,
	ldiNetworkTypeClientRead,
	ldiNetworkTypeClientLost,
};

struct ldiNetworkLoopResult {
	ldiNetworkType type;
	int id;
	uint8_t* data;
	int size;
};

ldiNet* networkInit();
void networkDestroy(ldiNet* Net);

int networkListen(ldiNet* Net, int Port);
ldiNetworkLoopResult networkServerLoop(ldiNet* Net);
int networkClearClientRead(ldiNet* Net, int ClientID);
int networkClientWrite(ldiNet* Net, int ClientId, uint8_t* Data, int Size);

int networkConnect(ldiNet* Net, const char* Hostname, int Port);
int networkWrite(ldiNet* Net, uint8_t* Data, int Size);
int networkRead(ldiNet* Net, uint8_t* Data, int Size);