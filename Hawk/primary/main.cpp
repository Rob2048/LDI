#include "LibCamera.h"
#include <iostream>
#include "lz4.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define IMG_WIDTH 1600
#define IMG_HEIGHT 1300

uint8_t pixelBuffer[IMG_WIDTH * IMG_HEIGHT];

// uint8_t* compressedBuffer;
// int compressedBufferSize = 0;

int64_t platformGetMicrosecond() {
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

void processImage(uint8_t* Data, int Size) {
	int64_t t0 = platformGetMicrosecond();

	int pixelCount = IMG_WIDTH * IMG_HEIGHT;

	// int result = LZ4_compress_default((const char*)Data, (char*)compressedBuffer, Size, compressedBufferSize);

	// NOTE: 10bit to 8bit conversion.
	for (int i = 0; i < pixelCount; ++i) {
		uint16_t pixel = ((uint16_t*)Data)[i];
		pixel = pixel >> 2;
		pixelBuffer[i] = (uint8_t)pixel;

		// uint8_t pixel = (Data[i * 2 + 1] << 6) | (Data[i * 2 + 0] >> 2);
		// pixelBuffer[i] = pixel;
	}

	t0 = platformGetMicrosecond() - t0;
	// std::cout << "Frame: " << result << " " << (t0 / 1000.0) << " ms\n";
	std::cout << "Frame: " << (Size / 1024) << " KB to " << (sizeof(pixelBuffer) / 1024) << " KB in " << (t0 / 1000.0) << " ms\n";

	static bool savedToFile = false;
	if (!savedToFile) {
		stbi_write_png("raw.png", IMG_WIDTH, IMG_HEIGHT, 1, pixelBuffer, IMG_WIDTH);
		savedToFile = true;

		// FILE* fd = fopen("raw.img", "wb");
		// fwrite(pixelBuffer, sizeof(pixelBuffer), 1, fd);
		// fclose(fd);
	}
}

int main() {
	// int decompressedSize = IMG_WIDTH * IMG_HEIGHT * 2;
	// compressedBufferSize = LZ4_compressBound(decompressedSize);
	// std::cout << "Decompressed size: " << decompressedSize << " Compressed size: " << compressedBufferSize << "\n";
	// compressedBuffer = new uint8_t[compressedBufferSize];

	time_t start_time = time(0);
	int frame_count = 0;
	
	LibCamera cam;

	// formats::RGB888
	int ret = cam.initCamera(IMG_WIDTH, IMG_HEIGHT, formats::SRGGB10, 4, 0);

	// NOTE: https://github.com/raspberrypi/libcamera-apps/blob/main/core/libcamera_app.cpp
	ControlList controls_;
	int64_t frame_time = 1000000 / 10;
	controls_.set(controls::FrameDurationLimits, { frame_time, frame_time });
	controls_.set(controls::ExposureTime, 60000);
	cam.set(controls_);
	
	if (!ret) {
		bool flag;
		LibcameraOutData frameData;
		cam.startCamera();
		
		while (true) {
			flag = cam.readFrame(&frameData);
			if (!flag) {
				usleep(1000);
				continue;
			}

			processImage(frameData.imageData, frameData.size);

			frame_count++;
			if ((time(0) - start_time) >= 1){
				printf("fps: %d\n", frame_count);
				frame_count = 0;
				start_time = time(0);
			}
			cam.returnFrameBuffer(frameData);
		}

		cam.stopCamera();
	}
	
	cam.closeCamera();

	return 0;
}