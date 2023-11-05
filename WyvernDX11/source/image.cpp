#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define __STDC_LIB_EXT1__
#include "stb_image_write.h"

// NOTE: Always returns a 4 channel image!
uint8_t* imageLoadRgba(const char* FileName, int* Width, int* Height, int* Channels) {
	return stbi_load(FileName, Width, Height, Channels, 4);
}

void imageFree(uint8_t* ImageData) {
	stbi_image_free(ImageData);
}

int imageWrite(const char* FileName, int Width, int Height, int Components, int StrideInBytes, const void* Data) {
	return stbi_write_png(FileName, Width, Height, Components, Data, StrideInBytes);
}




