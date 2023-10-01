#pragma once

#define DEVICE_RECV_TEMP_SIZE (1024 * 1024 * 64)
#define CAMERA_FRAME_BUFFER_SIZE (1024 * 1024 * 12)

enum ldiCameraCaptureMode {
	LDI_CAMERACAPTUREMODE_WAIT = 0,
	LDI_CAMERACAPTUREMODE_CONTINUOUS = 1,
	LDI_CAMERACAPTUREMODE_AVERAGE = 2,
	LDI_CAMERACAPTUREMODE_SINGLE = 3,
};

struct ldiHawk {
	// Only main thread.
	int							imgWidth;
	int							imgHeight;
		
	uint8_t*					frameBuffer;
	int							latestFrameId;

	// NOTE: Capture mode DOES NOT reflect the actual state of the camera at all times.
	ldiCameraCaptureMode		captureMode;
	int							shutterSpeed;
	int							analogGain;

	// Shared by threads.
	std::thread					workerThread;
	std::atomic_bool			workerThreadRunning = true;
	std::atomic_bool			connected = false;
	std::mutex					socketWriteMutex;
	
	ldiThreadSafeQueue			packetQueue;

	std::string					hostname;
	int							port;
	SOCKET						socket;

	// Only worker thread.
	uint8_t*					rxBuffer;
	ldiPacketBuilder			packetBuilder;
};

void hawkDisconnect(ldiHawk* Cam) {
	if (Cam->connected) {
		Cam->connected = false;
		closesocket(Cam->socket);
		Cam->socket = NULL;
	}
}

void hawkSocketWrite(ldiHawk* Cam, uint8_t* Data, int Size) {
	if (!Cam->connected) {
		std::cout << "Tried to write to hawk that was not connected\n";
		return;
	}

	std::unique_lock<std::mutex> lock(Cam->socketWriteMutex);
	int result = send(Cam->socket, (const char*)Data, Size, 0);
	//std::cout << "Send result: " << result << "\n";
}

void hawkWorkerThread(ldiHawk* Cam) {
	std::cout << "Running machine vision cam thread\n";

	// Check incoming data, or wait on jobs to execute.

	int totalRecvBytes = 0;

	int recvCount = 0;

	while (Cam->workerThreadRunning) {
		if (networkConnect(Cam->hostname, Cam->port, &Cam->socket)) {
			Cam->connected = true;
			packetBuilderReset(&Cam->packetBuilder);

			// Send initial data request.
			{
				ldiProtocolHeader packet;
				packet.packetSize = sizeof(ldiProtocolHeader) - 4;
				packet.opcode = 2;

				hawkSocketWrite(Cam, (uint8_t*)&packet, sizeof(ldiProtocolHeader)); 
			}

			while (Cam->connected) {
				//std::cout << "Poll\n";

				WSAPOLLFD fds[1];
				fds[0].fd = Cam->socket;
				fds[0].events = POLLRDNORM;

				int pollResult = WSAPoll(fds, 1, 1000);

				if (pollResult > 0) {
					//std::cout << "Got poll result: " << pollResult << " " << fds[0].revents << "\n";

					if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
						std::cout << "Bad socket\n";
						Cam->connected = false;
						Cam->socket = NULL;
						break;
					} else if (fds[0].revents & POLLRDNORM) {
						int recvBytes = recv(Cam->socket, (char*)Cam->rxBuffer, DEVICE_RECV_TEMP_SIZE, 0);
						//std::cout << "Read " << recvBytes << " bytes.\n";

						if (recvBytes == 0) {
							// TODO: Terminate socket.
						} else if (recvBytes == SOCKET_ERROR) {
							int error = WSAGetLastError();

							if (error == WSAEWOULDBLOCK) {
								std::cout << "Would block\n";
							} else {
								std::cout << "Some other error? " << error << "\n";
								// TODO: Terminate socket.
							}
						} else {
							int bytesProcessed = 0;

							while (bytesProcessed != recvBytes) {
								bytesProcessed += packetBuilderProcessData(&Cam->packetBuilder, Cam->rxBuffer + bytesProcessed, recvBytes - bytesProcessed);

								if (Cam->packetBuilder.state == 2) {
									//std::cout << "Net layer got full packet\n";
									uint8_t* newPacket = new uint8_t[Cam->packetBuilder.len];
									memcpy(newPacket, Cam->packetBuilder.data, Cam->packetBuilder.len);
									packetBuilderReset(&Cam->packetBuilder);
									tsqPush(&Cam->packetQueue, newPacket);
								}
							}
						}
					}
				}
			}
		} else {
			std::cout << "Camera failed to connect\n";
			Sleep(1000);
		}
	}
}

void hawkInit(ldiHawk* Cam, const std::string& Hostname, int Port) {
	std::cout << "Init machine vision cam at " << Hostname << ":" << Port << "\n";

	Cam->imgWidth = 0;
	Cam->imgHeight = 0;
	Cam->frameBuffer = new uint8_t[CAMERA_FRAME_BUFFER_SIZE];
	Cam->hostname = Hostname;
	Cam->port = Port;

	Cam->shutterSpeed = 10000;
	Cam->analogGain = 2.0;

	Cam->rxBuffer = new uint8_t[DEVICE_RECV_TEMP_SIZE];
	packetBuilderInit(&Cam->packetBuilder, 1024 * 1024 * 32);
	
	Cam->workerThread = std::thread(hawkWorkerThread, Cam);
}

bool hawkUpdate(ldiApp* AppContext, ldiHawk* Cam) {
	uint8_t* rawPacket = (uint8_t*)tsqPop(&Cam->packetQueue);
	
	if (rawPacket != nullptr) {
		ldiProtocolHeader* packetHeader = (ldiProtocolHeader*)rawPacket;

		if (packetHeader->opcode == 0) {
			ldiProtocolSettings* packet = (ldiProtocolSettings*)rawPacket;
			std::cout << "Got settings: " << packet->shutterSpeed << " " << packet->analogGain << "\n";

			Cam->shutterSpeed = packet->shutterSpeed;
			Cam->analogGain = packet->analogGain;

		} else if (packetHeader->opcode == 1) {
			ldiProtocolImageHeader* imageHeader = (ldiProtocolImageHeader*)rawPacket;
			//std::cout << "Got image " << imageHeader->width << " " << imageHeader->height << "\n";

			if (imageHeader->width * imageHeader->height > CAMERA_FRAME_BUFFER_SIZE) {
				std::cout << "Received image bigger than frame buffer: " << (imageHeader->width * imageHeader->height) << " bytes, max: " << CAMERA_FRAME_BUFFER_SIZE << " bytes\n";
				return false;
			}

			Cam->imgWidth = imageHeader->width;
			Cam->imgHeight = imageHeader->height;

			uint8_t* frameData = rawPacket + sizeof(ldiProtocolImageHeader);
			memcpy(Cam->frameBuffer, frameData, imageHeader->width * imageHeader->height);
			
			delete[] rawPacket;
			return true;
		} else if (packetHeader->opcode == 4) {
			std::cout << "Hawk average finished gather\n";
		} else {
			std::cout << "Error: Got unknown opcode (" << packetHeader->opcode << ") from packet\n";
		}

		delete[] rawPacket;
	}

	return false;
}

void hawkCommitSettings(ldiHawk* Cam) {
	ldiProtocolSettings packet;
	packet.header.packetSize = sizeof(ldiProtocolSettings) - 4;
	packet.header.opcode = 0;
	packet.shutterSpeed = Cam->shutterSpeed;
	packet.analogGain = Cam->analogGain;

	hawkSocketWrite(Cam, (uint8_t*)&packet, sizeof(ldiProtocolSettings));
}

void hawkTriggerAndGetFrame(ldiHawk* Cam) {

}

void hawkSetMode(ldiHawk* Cam, ldiCameraCaptureMode Mode) {
	Cam->captureMode = Mode;

	ldiProtocolMode packet;
	packet.header.packetSize = sizeof(ldiProtocolMode) - 4;
	packet.header.opcode = 1;
	packet.mode = (int)Mode;

	hawkSocketWrite(Cam, (uint8_t*)&packet, sizeof(ldiProtocolMode));
}