#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

uint8_t* imageLoadRgba(const char* FileName, int* Width, int* Height, int* Channels) {
	return stbi_load(FileName, Width, Height, Channels, 4);
}

void imageFree(uint8_t* ImageData) {
	stbi_image_free(ImageData);
}




