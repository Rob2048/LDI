#pragma once

#include <stdint.h>

uint8_t* imageLoadRgba(const char* FileName, int* Width, int* Height, int* Channels);
void imageFree(uint8_t* ImageData);