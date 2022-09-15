#pragma once

#include <stdint.h>

struct ldiImage {
	int width;
	int height;
	uint8_t* data;
};

uint8_t* imageLoadRgba(const char* FileName, int* Width, int* Height, int* Channels);
void imageFree(uint8_t* ImageData);