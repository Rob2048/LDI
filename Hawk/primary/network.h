#pragma once

#include <stdint.h>

struct ldiNet;

ldiNet* networkCreate();
void networkDestroy(ldiNet* Net);

int networkConnect(ldiNet* Net, const char* Hostname, int Port);
int networkWrite(ldiNet* Net, uint8_t* Data, int Size);
int networkRead(ldiNet* Net, uint8_t* Data, int Size);