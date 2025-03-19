#pragma once

#define DEVICE_RECV_TEMP_SIZE (1024 * 1024 * 64)
#define CAMERA_FRAME_BUFFER_SIZE (1024 * 1024 * 12)

enum ldiCameraCaptureMode {
	CCM_WAIT = 0,
	CCM_CONTINUOUS = 1,
	CCM_AVERAGE = 2,
	CCM_SINGLE = 3,
	CCM_AVERAGE_NO_FLASH = 4,
};

enum ldiHawkOpcode {
	HO_NONE = -1,
	HO_SETTINGS = 0,
	HO_FRAME = 1,
	HO_AVERAGE_GATHERED = 2,
	
	HO_SETTINGS_REQUEST = 10,
	HO_SET_VALUES = 11,
	HO_SET_CAPTURE_MODE = 12,
};

struct ldiHawk {
	// Only main thread.
	
	// NOTE: Capture mode DOES NOT reflect the actual state of the camera at all times.
	// NOTE: Precommited values for UI.
	ldiCameraCaptureMode		uiCaptureMode;
	int							uiShutterSpeed;
	int							uiAnalogGain;

	// Shared by threads.
	std::thread					workerThread;
	std::atomic_bool			workerThreadRunning = true;
	std::atomic_bool			connected = false;
	std::mutex					socketWriteMutex;
	std::mutex					packetRecvdMutex;
	std::condition_variable		packetRecvdCondVar;
	ldiHawkOpcode				packetRecvdOpcode = HO_NONE;
	
	//ldiThreadSafeQueue			packetQueue;

	std::string					hostname;
	int							port;
	SOCKET						socket;

	std::mutex					valuesMutex;
	int							imgWidth;
	int							imgHeight;
	uint8_t*					frameBuffer;
	int							latestFrameId;
	int							shutterSpeed;
	int							analogGain;

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

void hawkProcessPacket(ldiHawk* Cam, ldiPacketBuilder* Packet) {
	uint8_t* rawPacket = Packet->data;

	if (rawPacket != nullptr) {
		ldiProtocolHeader* packetHeader = (ldiProtocolHeader*)rawPacket;

		if (packetHeader->opcode == HO_SETTINGS) {
			ldiProtocolSettings* packet = (ldiProtocolSettings*)rawPacket;
			std::cout << "Got settings: " << packet->shutterSpeed << " " << packet->analogGain << "\n";

			{
				std::unique_lock<std::mutex> lock(Cam->valuesMutex);
				Cam->shutterSpeed = packet->shutterSpeed;
				Cam->analogGain = packet->analogGain;
			}
		} else if (packetHeader->opcode == HO_FRAME) {
			ldiProtocolImageHeader* imageHeader = (ldiProtocolImageHeader*)rawPacket;
			//std::cout << "Got image " << imageHeader->width << " " << imageHeader->height << "\n";

			if (imageHeader->width * imageHeader->height > CAMERA_FRAME_BUFFER_SIZE) {
				std::cout << "Received image bigger than frame buffer: " << (imageHeader->width * imageHeader->height) << " bytes, max: " << CAMERA_FRAME_BUFFER_SIZE << " bytes\n";
			} else {
				std::unique_lock<std::mutex> lock(Cam->valuesMutex);
				Cam->imgWidth = imageHeader->width;
				Cam->imgHeight = imageHeader->height;

				uint8_t* frameData = rawPacket + sizeof(ldiProtocolImageHeader);

				// Direct replace:
				memcpy(Cam->frameBuffer, frameData, imageHeader->width * imageHeader->height);

				// Low pass filter:
				//for (int i = 0; i < imageHeader->width * imageHeader->height; ++i) {
					//Cam->frameBuffer[i] = (uint8_t)((double)Cam->frameBuffer[i] * 0.5 + (double)frameData[i] * 0.5);
				//}

				++Cam->latestFrameId;
			}
		} else if (packetHeader->opcode == HO_AVERAGE_GATHERED) {
			std::cout << "Hawk average finished gather\n";
		} else {
			std::cout << "Error: Got unknown opcode (" << packetHeader->opcode << ") from packet\n";
		}

		{
			std::unique_lock<std::mutex> lock(Cam->packetRecvdMutex);
			Cam->packetRecvdOpcode = (ldiHawkOpcode)packetHeader->opcode;
		}

		Cam->packetRecvdCondVar.notify_all();
	}
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
				packet.opcode = HO_SETTINGS_REQUEST;

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
									hawkProcessPacket(Cam, &Cam->packetBuilder);
									packetBuilderReset(&Cam->packetBuilder);
								}
							}
						}
					}
				}
			}
		} else {
			//std::cout << "Camera failed to connect\n";
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

	Cam->latestFrameId = 0;
	
	Cam->workerThread = std::thread(hawkWorkerThread, Cam);
}

void hawkTriggerAndGetFrame(ldiHawk* Cam) {

}

// Copies values from worker thread to global access.
void hawkUpdateValues(ldiHawk* Cam) {
	std::unique_lock<std::mutex> lock(Cam->valuesMutex);

	Cam->uiAnalogGain = Cam->analogGain;
	Cam->uiShutterSpeed = Cam->shutterSpeed;
}

void hawkCommitValues(ldiHawk* Cam) {
	{
		std::unique_lock<std::mutex> lock(Cam->valuesMutex);
		Cam->analogGain = Cam->uiAnalogGain;
		Cam->shutterSpeed = Cam->uiShutterSpeed;
	}

	ldiProtocolSettings packet;
	packet.header.packetSize = sizeof(ldiProtocolSettings) - 4;
	packet.header.opcode = HO_SET_VALUES;
	packet.shutterSpeed = Cam->uiShutterSpeed;
	packet.analogGain = Cam->uiAnalogGain;

	hawkSocketWrite(Cam, (uint8_t*)&packet, sizeof(ldiProtocolSettings));
}

void hawkSetMode(ldiHawk* Cam, ldiCameraCaptureMode Mode) {
	Cam->uiCaptureMode = Mode;

	ldiProtocolMode packet;
	packet.header.packetSize = sizeof(ldiProtocolMode) - 4;
	packet.header.opcode = HO_SET_CAPTURE_MODE;
	packet.mode = (int)Mode;

	hawkSocketWrite(Cam, (uint8_t*)&packet, sizeof(ldiProtocolMode));
}

void hawkClearWaitPacket(ldiHawk* Cam) {
	std::unique_lock<std::mutex> lock(Cam->packetRecvdMutex);
	Cam->packetRecvdOpcode == HO_NONE;
}

bool hawkWaitForPacket(ldiHawk* Cam, ldiHawkOpcode Opcode) {
	while (true) {
		{
			std::unique_lock<std::mutex> lock(Cam->packetRecvdMutex);
			if (Cam->packetRecvdOpcode == Opcode) {
				return true;
			}
		}

		{
			//std::cout << "Wait\n";
			std::unique_lock<std::mutex> lock(Cam->packetRecvdMutex);
			Cam->packetRecvdCondVar.wait(lock);
			//std::cout << "Awake " << Cam->packetRecvdOpcode << "\n";
		}
	}

	return false;
}