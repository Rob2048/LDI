#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include "network.h"
#include <string.h>
#include "camera.h"
#include <vector>
// #inlcude <pigpio.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

// NOTE: OV9281
// #define IMG_WIDTH 1280
// #define IMG_HEIGHT 800

// NOTE: OV2311
// #define IMG_WIDTH 1600
// #define IMG_HEIGHT 1300

// NOTE: IMX477
// TODO: NB: The width rounds to nearest 16.
// #define IMG_WIDTH 4056
// #define IMG_HEIGHT 3040

// NOTE: IMX219
// #define IMG_WIDTH 1920
// #define IMG_HEIGHT 1080
#define IMG_WIDTH 3280
#define IMG_HEIGHT 2464
// NOTE: Stride = 3328

// uint8_t tempBuffer[IMG_WIDTH * IMG_HEIGHT];
uint8_t tempBuffer[3328 * IMG_HEIGHT];
uint8_t pixelBuffer[IMG_WIDTH * IMG_HEIGHT];
float floatBuffer[IMG_WIDTH * IMG_HEIGHT];
float intBuffer[IMG_WIDTH * IMG_HEIGHT];

uint8_t avgBuffer[5][IMG_WIDTH * IMG_HEIGHT * 5];
int64_t averageModeStartTimeMs = 0;
int averageModeCapturedFrames = 0;
int averageModeCaptureTarget = 0;

int _shutterSpeed = 66000;
int _analogGain = 1;

int contModeLastSendTimeUs = 0;

// NOTE: IMX219
int frameWidth = 3280;
int frameHeight = 2464;
// int frameWidth = 1640;
// int frameHeight = 1232;
int frameStride = 0;

FILE* ledProc = NULL;

enum ldiCameraCaptureMode {
	CCM_WAIT = 0,
	CCM_CONTINUOUS = 1,
	CCM_AVERAGE = 2,
	CCM_SINGLE = 3,
};

enum ldiHawkOpcode {
	HO_SETTINGS = 0,
	HO_FRAME = 1,
	HO_AVERAGE_GATHERED = 2,
	
	HO_SETTINGS_REQUEST = 10,
	HO_SET_VALUES = 11,
	HO_SET_CAPTURE_MODE = 12,
};

ldiCameraCaptureMode _mode = CCM_WAIT;

ldiCamContext*		camContext		= nullptr;
ldiNet*				net				= nullptr;

//-------------------------------------------------------------------------
// Platform implementation.
//-------------------------------------------------------------------------
int64_t platformGetMicrosecond() {
	timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);

	return ((int64_t)time.tv_sec * 1000000 + (int64_t)time.tv_nsec / 1000);
}

//-------------------------------------------------------------------------
// LED ring.
//-------------------------------------------------------------------------
int ledsStartProcess() {
	ledProc = popen("python /home/pi/projects/leds.py", "w");

	if (ledProc == NULL) {
		std::cout << "Failed to open leds\n";
		return 1;
	}

	std::cout << "Opened leds" << "\n";

	// pclose(ledProc);
	return 0;
}

void startFlash() {
	fprintf(ledProc, "6\r\n");
	fflush(ledProc);
}

void stopLeds() {
	fprintf(ledProc, "0\r\n");
	fflush(ledProc);
}

//-------------------------------------------------------------------------
// Image processing.
//-------------------------------------------------------------------------
bool restartCamera(int Width, int Height, int Fps) {
	cameraStop(camContext);

	int frameSize = 0;

	if (cameraStart(camContext, Width, Height, Fps, _shutterSpeed, _analogGain, &frameSize, &frameStride) != 0) {
		std::cout << "Starting camera failed\n";
		return false;
	}

	frameSize = frameStride * Height;
	std::cout << "Frame size: " << frameSize << " Frame stride: " << frameStride << "\n";

	if (frameSize > sizeof(tempBuffer)) {
		std::cout << "Frame data size mismatch\n";
		return false;
	}

	return true;
}

void setCameraMode(ldiCameraCaptureMode Mode) {
	_mode = Mode;

	if (_mode == CCM_CONTINUOUS) {
		cameraSetFps(camContext, 3);
	}

	if (_mode == CCM_AVERAGE) {
		cameraSetFps(camContext, 15);
		// if (!restartCamera(3280, 2464, 15)) {
		// 	exit(1);
		// }

		averageModeStartTimeMs = platformGetMicrosecond() / 1000;
		averageModeCapturedFrames = 0;
		averageModeCaptureTarget = 5;

		startFlash();
	}
}

//-------------------------------------------------------------------------
// Networking.
//-------------------------------------------------------------------------
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

struct ldiProtocolMode {
	ldiProtocolHeader header;
	int mode;
};

struct ldiProtocolCapture {
	ldiProtocolHeader header;
	int avgCount;
};

struct ldiPacket {
	uint8_t buffer[1024 * 1024];
	int len = 0;
	int state = 0;
	int payloadLen = 0;
};

void packetInit(ldiPacket* Packet) {
	Packet->len = 0;
	Packet->state = 0;
	Packet->payloadLen = 0;
}

int packetProcessByte(ldiPacket* Packet, uint8_t Data) {
	if (Packet->state == 0) {
		Packet->buffer[Packet->len++] = Data;

		if (Packet->len == 4) {
			Packet->payloadLen = *(int*)Packet->buffer;
			Packet->state = 1;
		}
	} else if (Packet->state == 1) {
		Packet->buffer[Packet->len++] = Data;

		if (Packet->len - 4 == Packet->payloadLen) {
			return 1;
		}
	}

	return 0;
}

struct ldiControlAppClient {
	int id;
	ldiPacket packet;
};

std::vector<ldiControlAppClient> clients;

void handlePacket(ldiNet* Net, ldiControlAppClient* Client) {
	ldiPacket* clientPacket = &Client->packet;
	ldiProtocolHeader* header = (ldiProtocolHeader*)clientPacket->buffer;

	if (header->opcode == HO_SET_VALUES) {
		// NOTE: Set camera settings.
		ldiProtocolSettings* packet = (ldiProtocolSettings*)clientPacket->buffer;
		_shutterSpeed = packet->shutterSpeed;
		_analogGain = packet->analogGain;
		
		cameraSetControls(camContext, _shutterSpeed, _analogGain);

	} else if (header->opcode == HO_SET_CAPTURE_MODE) {
		// NOTE: Set camera mode.
		ldiProtocolMode* packet = (ldiProtocolMode*)clientPacket->buffer;
		setCameraMode((ldiCameraCaptureMode)packet->mode);
	} else if (header->opcode == HO_SETTINGS_REQUEST) {
		// NOTE: Get camera settings.
		ldiProtocolSettings settings;
		settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
		settings.header.opcode = HO_SETTINGS;
		settings.shutterSpeed = _shutterSpeed;
		settings.analogGain = _analogGain;

		std::cout << "Send camera settings to network client\n";

		if (networkClientWrite(Net, Client->id, (uint8_t*)&settings, sizeof(settings)) != 0) {
			std::cout << "Settings write failed\n";
			return;
		}
	} else {
		std::cout << "Unknown opcode\n";
		return;
	}

	packetInit(clientPacket);
}

ldiControlAppClient* addClient(int Id) {
	ldiControlAppClient client;
	client.id = Id;
	packetInit(&client.packet);
	clients.push_back(client);

	return &clients[clients.size() - 1];
}

void removeClient(int Id) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i].id == Id) {
			clients.erase(clients.begin() + i);
			return;
		}
	}
}

void readClient(ldiNet* Net, int Id, uint8_t* Data, int Size) {
	ldiControlAppClient* client = nullptr;

	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i].id == Id) {
			client = &clients[i];
			break;
		}
	}

	if (client == nullptr) {
		std::cout << "Client not found\n";
		return;
	}

	for (int i = 0; i < Size; ++i) {
		int packetResult = packetProcessByte(&client->packet, Data[i]);

		if (packetResult == 1) {
			handlePacket(Net, client);
		}
	}
}

void writeAllClients(ldiNet* Net, uint8_t* Data, int Size) {
	std::cout << "Client size: " << clients.size() << "\n";

	for (size_t i = 0; i < clients.size(); ++i) {
		if (networkClientWrite(Net, clients[i].id, Data, Size) != 0) {
			std::cout << "Write failed\n";
			return;
		}
	}
}

void sendAverageGatherDoneToClients() {
	ldiProtocolHeader packet;
	packet.packetSize = sizeof(ldiProtocolHeader) - 4;
	packet.opcode = HO_AVERAGE_GATHERED;

	for (size_t i = 0; i < clients.size(); ++i) {
		if (networkClientWrite(net, clients[i].id, (uint8_t*)&packet, sizeof(packet)) != 0) {
			std::cout << "Write failed\n";
			continue;
		}
	}
}

void sendPixelBufferToClients(int Width, int Height) {
	ldiProtocolImageHeader header;
	header.header.packetSize = sizeof(ldiProtocolImageHeader) - 4 + (Width * Height);
	header.header.opcode = HO_FRAME;
	header.width = Width;
	header.height = Height;

	int64_t t0 = platformGetMicrosecond();
	for (size_t i = 0; i < clients.size(); ++i) {
		if (networkClientWrite(net, clients[i].id, (uint8_t*)&header, sizeof(header)) != 0) {
			std::cout << "Write failed\n";
			continue;
		}

		if (networkClientWrite(net, clients[i].id, pixelBuffer, Width * Height) != 0) {
			std::cout << "Write failed\n";
			continue;
		}
	}
	t0 = platformGetMicrosecond() - t0;
	// std::cout << "Wrote to " << clients.size() << " clients in " << (t0 / 1000.0) << " ms\n";
}

//-------------------------------------------------------------------------
// Main application.
//-------------------------------------------------------------------------
int main() {
	ledsStartProcess();

	net = networkInit();
	
	if (networkListen(net, 6969) != 0) {
		std::cout << "Network listen failed\n";
		return 1;
	}

	camContext = cameraCreateContext();
	cameraInit(camContext);

	int frameSize = 0;
	
	if (cameraStart(camContext, frameWidth, frameHeight, 15, _shutterSpeed, _analogGain, &frameSize, &frameStride) != 0) {
		std::cout << "Failed to start camera\n";
		return 1;
	}

	// if (!restartCamera(frameWidth, frameHeight, 15)) {
	// 	return 1;
	// }

	setCameraMode(_mode);
	
	int64_t timeAtLastFrame = 0;
	int frameId = 0;

	int64_t lastSync = 0;

	std::cout << "Starting main loop...\n";
	while (true) {
		// Update server tasks.
		bool waitForNone = true;
		while (waitForNone) {
			ldiNetworkLoopResult serverResult = networkServerLoop(net);

			switch (serverResult.type) {
				case ldiNetworkTypeNone:
					waitForNone = false;
					break;

				case ldiNetworkTypeFailure:
					std::cout << "Network server failed\n";
					return 1;

				case ldiNetworkTypeClientNew:
					std::cout << "Got new client" << serverResult.id << "\n";
					addClient(serverResult.id);
					break;

				case ldiNetworkTypeClientRead:
					std::cout << "Got client data" << serverResult.id << "\n";
					readClient(net, serverResult.id, serverResult.data, serverResult.size);
					networkClearClientRead(net, serverResult.id);
					break;

				case ldiNetworkTypeClientLost:
					std::cout << "Lost client" << serverResult.id << "\n";
					removeClient(serverResult.id);
					break;
			}
		}
		
		// NOTE: Get next frame.
		if (!cameraReadFrame(camContext)) {
			usleep(1000);
			continue;
		}

		int64_t timeFrameRead = platformGetMicrosecond();
		LibcameraOutData* frameData = cameraGetFrameData(camContext);

		float frameDeltaTime = (timeFrameRead - timeAtLastFrame) / 1000.0;
		float fps = 1000.0 / frameDeltaTime;
		timeAtLastFrame = timeFrameRead;

		int64_t frameTimestampNs = frameData->timestamp;
		int64_t frameTimestampMs = frameTimestampNs / 1000000;

		int64_t timestampVsLastSync = frameTimestampMs - (lastSync / 1000);
		int64_t timestampVsRecv = (timeFrameRead / 1000) - frameTimestampMs;
		lastSync = timeFrameRead;

		// NOTE: Takes ~93ms from first pixel read (frame timestamp) to callback read.
		// Assuming first pixel read must happen after required exposure. How long before first pixel read does exposure occur?
		// Camera takes ~50ms to read out entire sensor. (Very close to 66ms max FPS). 

		// NOTE: ~300us to respond.
		// fprintf(ledProc, "5\r\n");
		// fflush(ledProc);
		// std::cout << "Sent sync at " << timeFrameRead << "\n";

		if (_mode == CCM_WAIT) {
			// ...
		} else if (_mode == CCM_AVERAGE) {
			// NOTE: Discard frames before start time.
			if (frameTimestampMs < averageModeStartTimeMs) {
				std::cout << "Skipped early frame\n";
			} else {
				int64_t t0 = platformGetMicrosecond();
				cameraCopyFrame(camContext, tempBuffer);

				for (int iY = 0; iY < frameHeight; ++iY) {
					memcpy(&avgBuffer[averageModeCapturedFrames][iY * frameWidth], &tempBuffer[iY * frameStride], frameWidth);
				}

				t0 = platformGetMicrosecond() - t0;

				++averageModeCapturedFrames;

				std::cout << "Average: " << averageModeCapturedFrames << "/" << averageModeCaptureTarget << " Proctime: " << (t0 / 1000.0) << " ms\n";

				if (averageModeCapturedFrames == averageModeCaptureTarget) {
					sendAverageGatherDoneToClients();
					stopLeds();

					int64_t averageTime = (platformGetMicrosecond() / 1000) - averageModeStartTimeMs;
					std::cout << "Gather time: " << averageTime << " ms\n";

					t0 = platformGetMicrosecond();
					for (int i = 0; i < frameWidth * frameHeight; ++i) {
						int finalValue = avgBuffer[0][i];
						finalValue += avgBuffer[1][i];
						finalValue += avgBuffer[2][i];
						finalValue += avgBuffer[3][i];
						finalValue += avgBuffer[4][i];

						pixelBuffer[i] = (uint8_t)(finalValue / 5);
					}

					t0 = platformGetMicrosecond() - t0;
					std::cout << "Merge time: " << (t0 / 1000) << " ms\n";

					sendPixelBufferToClients(frameWidth, frameHeight);
					setCameraMode(CCM_WAIT);
				}
			}
		} else if (_mode == CCM_CONTINUOUS || _mode == CCM_SINGLE) {
			int64_t t0 = platformGetMicrosecond();
			cameraCopyFrame(camContext, tempBuffer);
			
			for (int iY = 0; iY < frameHeight; ++iY) {
				memcpy(&pixelBuffer[iY * frameWidth], &tempBuffer[iY * frameStride], frameWidth);
			}

			t0 = platformGetMicrosecond() - t0;

			std::cout << "Frame delta time: " << frameDeltaTime << " ms FPS: " << fps << " Proctime: " << (t0 / 1000.0) << " ms Size: " << frameData->size << " " << timestampVsLastSync << "\n";
			std::cout << "Timestamp: " << frameTimestampMs << " Recv delay: " << timestampVsRecv << " ms Exposure: " << frameData->exposureTime << " Gain: " << frameData->analogGain << " Lux: " << frameData->lux << "\n";

			if (_mode == CCM_SINGLE) {
				sendPixelBufferToClients(frameWidth, frameHeight);
				setCameraMode(CCM_WAIT);
			} else {
				sendPixelBufferToClients(frameWidth, frameHeight);
			}
		}
			
		cameraReturnFrameBuffer(camContext);
	}

	cameraStop(camContext);

	return 0;
}