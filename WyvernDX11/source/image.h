#pragma once

#include <stdint.h>

struct ldiImage {
	int width;
	int height;
	uint8_t* data;
};

struct ldiImageFloat {
	int width;
	int height;
	float* data;
};

struct ldiDispImage {
	int width;
	int height;
	float* data;
	uint8_t* normalData;
};

uint8_t* imageLoadRgba(const char* FileName, int* Width, int* Height, int* Channels);
void imageFree(uint8_t* ImageData);
int imageWrite(const char* FileName, int Width, int Height, int Components, int StrideInBytes, const void* Data);