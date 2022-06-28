#include "LibCamera.h"
#include <iostream>
#include <stdint.h>
// #include "lz4.h"
#include "network.h"

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

	
	// std::cout << "Frame: " << result << " " << (t0 / 1000.0) << " ms\n";
	// std::cout << "Frame: " << (Size / 1024) << " KB to " << (sizeof(pixelBuffer) / 1024) << " KB in " << (t0 / 1000.0) << " ms\n";

	// static bool savedToFile = false;
	// if (!savedToFile) {
	// 	stbi_write_png("raw.png", IMG_WIDTH, IMG_HEIGHT, 1, pixelBuffer, IMG_WIDTH);
	// 	savedToFile = true;

	// 	// FILE* fd = fopen("raw.img", "wb");
	// 	// fwrite(pixelBuffer, sizeof(pixelBuffer), 1, fd);
	// 	// fclose(fd);
	// }
}

int main() {
	// int decompressedSize = IMG_WIDTH * IMG_HEIGHT * 2;
	// compressedBufferSize = LZ4_compressBound(decompressedSize);
	// std::cout << "Decompressed size: " << decompressedSize << " Compressed size: " << compressedBufferSize << "\n";
	// compressedBuffer = new uint8_t[compressedBufferSize];

	LibCamera cam;

	// formats::RGB888
	int ret = cam.initCamera(IMG_WIDTH, IMG_HEIGHT, formats::SRGGB10, 4, 0);

	if (ret) {
		std::cout << "Cam init failure\n";
		cam.closeCamera();
		return 1;
	}

	// NOTE: https://github.com/raspberrypi/libcamera-apps/blob/main/core/libcamera_app.cpp
	ControlList controls_;
	int64_t frame_time = 1000000 / 10;
	controls_.set(controls::FrameDurationLimits, { frame_time, frame_time });
	controls_.set(controls::ExposureTime, 15000);
	controls_.set(controls::AnalogueGain, 0);
	controls_.set(controls::draft::NoiseReductionMode, controls::draft::NoiseReductionModeOff);
	controls_.set(controls::Sharpness, 0);
	// controls_.set(controls::AwbMode, 7);
	// controls_.set(controls::ColourGains, { 0.0f, 0.0f });
	cam.set(controls_);

	LibcameraOutData frameData;
	cam.startCamera();
		
	ldiNet* net = networkCreate();

	int64_t timeAtLastFrame = 0;
	
	while (true) {
		if (networkConnect(net, "192.168.3.49", 20000) != 0) {
			std::cout << "Network failed to connect\n";
			sleep(1);
		}

		while (true) {
			// NOTE: Get next frame.
			bool flag = cam.readFrame(&frameData);
			if (!flag) {
				usleep(1000);
				continue;
			}

			int64_t timeAtFrame = platformGetMicrosecond();
			float frameDeltaTime = (timeAtFrame - timeAtLastFrame) / 1000.0;
			float fps = 1000.0 / frameDeltaTime;
			timeAtLastFrame = timeAtFrame;

			int64_t t0 = platformGetMicrosecond();
			processImage(frameData.imageData, frameData.size);
			t0 = platformGetMicrosecond() - t0;

			std::cout << "Frame delta time: " << frameDeltaTime << " ms FPS: " << fps << " Proctime: " << (t0 / 1000.0) << " ms\n";

			cam.returnFrameBuffer(frameData);

			struct ldiProtocolImageHeader {
				int packetSize;
				int opcode;
				int width;
				int height;
			};

			ldiProtocolImageHeader header;
			header.packetSize = sizeof(ldiProtocolImageHeader) - 4 + sizeof(pixelBuffer);
			header.opcode = 1;
			header.width = IMG_WIDTH;
			header.height = IMG_HEIGHT;

			if (networkWrite(net, (uint8_t*)&header, sizeof(header)) != 0) {
				std::cout << "Write failed\n";
				break;
			}

			if (networkWrite(net, pixelBuffer, sizeof(pixelBuffer)) != 0) {
				std::cout << "Write failed\n";
				break;
			}

			// sleep(1);
		}
	}

	cam.stopCamera();
	cam.closeCamera();

	return 0;
}