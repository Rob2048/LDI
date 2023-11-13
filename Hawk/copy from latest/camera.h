#pragma once

#include <stdint.h>

struct LibcameraOutData {
	uint64_t request;
	uint8_t *imageData;
	uint32_t size;
	int64_t timestamp;
	int exposureTime;
	float analogGain;
	float lux;
};

struct ldiCamContext;

ldiCamContext* cameraCreateContext();
int cameraInit(ldiCamContext* Context);
int cameraStart(ldiCamContext* Context, int Width, int Height, int Fps, int ShutterSpeed, int Gain, int* FrameSize, int* FrameStride);
int cameraStop(ldiCamContext* Context);
int cameraSetFps(ldiCamContext* Context, int Fps);
int cameraSetControls(ldiCamContext* Context, int Exposure, int Gain);
bool cameraReadFrame(ldiCamContext* Context);

// int64_t cameraGetFrameTimestamp(ldiCamContext* Context);
// int cameraGetFrameDataSize(ldiCamContext* Context);
LibcameraOutData* cameraGetFrameData(ldiCamContext* Context);

void cameraCopyFrame(ldiCamContext* Context, uint8_t* Buffer);
int cameraReturnFrameBuffer(ldiCamContext* Context);