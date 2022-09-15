#include "LibCamera.h"
#include <iostream>
#include <stdint.h>
// #include "lz4.h"
#include "network.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define IMG_WIDTH 1600
#define IMG_HEIGHT 1300
// #define IMG_WIDTH 1920
// #define IMG_HEIGHT 1080
// #define IMG_WIDTH 3280
// #define IMG_HEIGHT 2464

uint8_t pixelBuffer[IMG_WIDTH * IMG_HEIGHT];

bool _settingsUpdate = false;
// int _shutterSpeed = 8000;
// int _analogGain = 1;
int _shutterSpeed = 40000;
int _analogGain = 2;

// uint8_t* compressedBuffer;
// int compressedBufferSize = 0;

int64_t platformGetMicrosecond() {
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

void processImageSRGGB8(uint8_t* Data, int Size) {
	int pixelCount = IMG_WIDTH * IMG_HEIGHT;

	memcpy(pixelBuffer, Data, pixelCount);
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

struct ldiProtocolHeader {
	int packetSize;
	int opcode;
};

struct ldiProtocolImageHeader {
	ldiProtocolHeader header;
	int width;
	int height;
};

struct ldiProtocolSettings {
	ldiProtocolHeader header;
	int shutterSpeed;
	int analogGain;
};

struct ldiPacket {
	uint8_t buffer[1024 * 1024];
	int len = 0;
	int state = 0;
	int payloadLen = 0;
};

void handlePacket(ldiPacket* Packet) {
	ldiProtocolHeader* header = (ldiProtocolHeader*)Packet->buffer;

	if (header->opcode == 0) {
		ldiProtocolSettings* packet = (ldiProtocolSettings*)Packet->buffer;
		_shutterSpeed = packet->shutterSpeed;
		_analogGain = packet->analogGain;
		_settingsUpdate = true;

	} else {
		std::cout << "Unknown opcode\n";
	}
}

ldiPacket _packet;

void packetInit(ldiPacket* Packet) {
	Packet->len = 0;
	Packet->state = 0;
	Packet->payloadLen = 0;
}

int packetProcessData(ldiPacket* Packet, uint8_t* Data, int Size) {
	for (int i = 0; i < Size; ++i) {
		if (Packet->state == 0) {
			Packet->buffer[Packet->len++] = Data[i];

			if (Packet->len == 4) {
				Packet->payloadLen = *(int*)Packet->buffer;
				Packet->state = 1;
			}
		} else if (Packet->state == 1) {
			Packet->buffer[Packet->len++] = Data[i];

			if (Packet->len - 4 == Packet->payloadLen) {
				handlePacket(Packet);
				packetInit(Packet);
			}
		}
	}

	return 0;
}

int runIMX219() {
	// int decompressedSize = IMG_WIDTH * IMG_HEIGHT * 2;
	// compressedBufferSize = LZ4_compressBound(decompressedSize);
	// std::cout << "Decompressed size: " << decompressedSize << " Compressed size: " << compressedBufferSize << "\n";
	// compressedBuffer = new uint8_t[compressedBufferSize];

	LibCamera cam;

	// formats::RGB888
	int ret = cam.initCamera(IMG_WIDTH, IMG_HEIGHT, formats::SRGGB8, 4, 0);

	if (ret) {
		std::cout << "Cam init failure\n";
		cam.closeCamera();
		return 1;
	}

	// NOTE: https://github.com/raspberrypi/libcamera-apps/blob/main/core/libcamera_app.cpp
	ControlList controls_;
	int64_t frame_time = 1000000 / 10;
	controls_.set(controls::FrameDurationLimits, { frame_time, frame_time });
	controls_.set(controls::ExposureTime, _shutterSpeed);
	// NOTE: Analog gain at 0 is some kind of automatic?
	controls_.set(controls::AnalogueGain, _analogGain);
	controls_.set(controls::draft::NoiseReductionMode, controls::draft::NoiseReductionModeOff);
	controls_.set(controls::Sharpness, 0);
	controls_.set(controls::AwbMode, libcamera::controls::AwbCustom);
	controls_.set(controls::ColourGains, { 0.0f, 0.0f });
	cam.set(controls_);

	LibcameraOutData frameData;
	cam.startCamera();

	ldiNet* net = networkCreate();
		
	int64_t timeAtLastFrame = 0;
	
	while (true) {
		packetInit(&_packet);

		if (networkConnect(net, "192.168.3.49", 20000) != 0) {
			std::cout << "Network failed to connect\n";
			// sleep(1);
			continue;
		}

		// NOTE: Just connected, send initial data.
		ldiProtocolSettings settings;
		settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
		settings.header.opcode = 0;
		settings.shutterSpeed = _shutterSpeed;
		settings.analogGain = _analogGain;

		if (networkWrite(net, (uint8_t*)&settings, sizeof(settings)) != 0) {
			std::cout << "Write failed\n";
			break;
		}

		while (true) {
			uint8_t buff[256];

			int readBytes = networkRead(net, buff, 256);

			if (readBytes == -1) {
				std::cout << "Read failed\n";
				break;
			} else if (readBytes > 0) {
				// Process packet bytes.
				std::cout << "Read: " << readBytes << "\n";

				packetProcessData(&_packet, buff, readBytes);
			}

			// NOTE: Get next frame.
			bool flag = cam.readFrame(&frameData);
			if (!flag) {
				usleep(1000);
				continue;
			}

			if (_settingsUpdate) {
				std::cout << "Update settings\n";
				_settingsUpdate = false;
				controls_.set(controls::ExposureTime, _shutterSpeed);
				controls_.set(controls::AnalogueGain, _analogGain);
				cam.set(controls_);
			}

			int64_t timeAtFrame = platformGetMicrosecond();
			float frameDeltaTime = (timeAtFrame - timeAtLastFrame) / 1000.0;
			float fps = 1000.0 / frameDeltaTime;
			timeAtLastFrame = timeAtFrame;

			int64_t t0 = platformGetMicrosecond();
			processImageSRGGB8(frameData.imageData, frameData.size);
			t0 = platformGetMicrosecond() - t0;

			std::cout << "Frame delta time: " << frameDeltaTime << " ms FPS: " << fps << " Proctime: " << (t0 / 1000.0) << " ms\n";

			cam.returnFrameBuffer(frameData);

			ldiProtocolImageHeader header;
			header.header.packetSize = sizeof(ldiProtocolImageHeader) - 4 + sizeof(pixelBuffer);
			header.header.opcode = 1;
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

int runOV2311() {
	std::cout << "runOV2311\n";
	// int decompressedSize = IMG_WIDTH * IMG_HEIGHT * 2;
	// compressedBufferSize = LZ4_compressBound(decompressedSize);
	// std::cout << "Decompressed size: " << decompressedSize << " Compressed size: " << compressedBufferSize << "\n";
	// compressedBuffer = new uint8_t[compressedBufferSize];

	LibCamera cam;

	// formats::RGB888
	int ret = cam.initCamera(IMG_WIDTH, IMG_HEIGHT, formats::SRGGB8, 4, 0);
	// int ret = cam.initCamera(IMG_WIDTH, IMG_HEIGHT, formats::SRGGB10, 4, 0);

	if (ret) {
		std::cout << "Cam init failure\n";
		cam.closeCamera();
		return 1;
	}

	// NOTE: https://github.com/raspberrypi/libcamera-apps/blob/main/core/libcamera_app.cpp
	ControlList controls_;
	int64_t frame_time = 1000000 / 60;
	controls_.set(controls::FrameDurationLimits, { frame_time, frame_time });
	controls_.set(controls::ExposureTime, _shutterSpeed);
	// NOTE: Analog gain at 0 is some kind of automatic?
	controls_.set(controls::AnalogueGain, _analogGain);
	controls_.set(controls::draft::NoiseReductionMode, controls::draft::NoiseReductionModeOff);
	controls_.set(controls::Sharpness, 0);
	// controls_.set(controls::AwbMode, libcamera::controls::AwbCustom);
	// controls_.set(controls::ColourGains, { 0.0f, 0.0f });
	cam.set(controls_);

	LibcameraOutData frameData;
	cam.startCamera();
		
	ldiNet* net = networkCreate();

	int64_t timeAtLastFrame = 0;
	
	while (true) {
		packetInit(&_packet);

		if (networkConnect(net, "192.168.3.49", 20000) != 0) {
			std::cout << "Network failed to connect\n";
			// sleep(1);
			continue;
		}

		// NOTE: Just connected, send initial data.
		ldiProtocolSettings settings;
		settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
		settings.header.opcode = 0;
		settings.shutterSpeed = _shutterSpeed;
		settings.analogGain = _analogGain;

		if (networkWrite(net, (uint8_t*)&settings, sizeof(settings)) != 0) {
			std::cout << "Write failed\n";
			break;
		}

		while (true) {
			uint8_t buff[256];

			int readBytes = networkRead(net, buff, 256);

			if (readBytes == -1) {
				std::cout << "Read failed\n";
				break;
			} else if (readBytes > 0) {
				// Process packet bytes.
				std::cout << "Read: " << readBytes << "\n";

				packetProcessData(&_packet, buff, readBytes);
			}

			// NOTE: Get next frame.
			bool flag = cam.readFrame(&frameData);
			if (!flag) {
				usleep(1000);
				continue;
			}

			if (_settingsUpdate) {
				std::cout << "Update settings\n";
				_settingsUpdate = false;
				controls_.set(controls::ExposureTime, _shutterSpeed);
				controls_.set(controls::AnalogueGain, _analogGain);
				cam.set(controls_);
			}

			int64_t timeAtFrame = platformGetMicrosecond();
			float frameDeltaTime = (timeAtFrame - timeAtLastFrame) / 1000.0;
			float fps = 1000.0 / frameDeltaTime;
			timeAtLastFrame = timeAtFrame;

			int64_t t0 = platformGetMicrosecond();
			// processImage(frameData.imageData, frameData.size);
			processImageSRGGB8(frameData.imageData, frameData.size);
			t0 = platformGetMicrosecond() - t0;

			std::cout << "Frame delta time: " << frameDeltaTime << " ms FPS: " << fps << " Proctime: " << (t0 / 1000.0) << " ms\n";

			cam.returnFrameBuffer(frameData);

			ldiProtocolImageHeader header;
			header.header.packetSize = sizeof(ldiProtocolImageHeader) - 4 + sizeof(pixelBuffer);
			header.header.opcode = 1;
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

int main() {
	return runOV2311();
	// return runIMX219();
}