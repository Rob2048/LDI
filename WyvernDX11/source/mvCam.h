#pragma once

#define SERIAL_DEVICE_RECV_TEMP_SIZE (4096 * 1024)

struct ldiMvCam {
	// Only main thread.
	int							imgWidth;
	int							imgHeight;
		
	uint8_t*					frameBuffer;
	int							latestFrameId;

	// Shared by threads.
	std::thread					workerThread;
	std::atomic_bool			workerThreadRunning = true;
	std::atomic_bool			serialPortConnected = false;
	std::mutex					frameMutex;
	volatile bool				frameAvailable = false;

	std::string					serialPortPath;

	// Only worker thread.
	uint8_t*					captureFrameBuffer;
	ldiSerialPort				serialPort;
	uint8_t*					recvTemp;
	int32_t						recvTempSize = 0;
	int32_t						recvTempPos = 0;
};

void mvCamDisconnect(ldiMvCam* Cam) {
	if (Cam->serialPortConnected) {
		Cam->serialPortConnected = false;
		serialPortDisconnect(&Cam->serialPort);
	}
}

void mvCamWorkerThread(ldiMvCam* Cam) {
	std::cout << "Running machine vision cam thread\n";

	// Check incoming data, or wait on jobs to execute.

	int totalRecvBytes = 0;

	int recvCount = 0;

	while (Cam->workerThreadRunning) {
		if (serialPortConnect(&Cam->serialPort, Cam->serialPortPath.c_str(), 921600)) {
			Cam->serialPortConnected = true;

			while (Cam->serialPortConnected) {
				Cam->recvTempSize = serialPortReadData(&Cam->serialPort, Cam->recvTemp, SERIAL_DEVICE_RECV_TEMP_SIZE);
				Cam->recvTempPos = 0;

				if (Cam->recvTempSize == -1) {
					mvCamDisconnect(Cam);
					break;
				} else if (Cam->recvTempSize == 0) {
					Sleep(1);
				} else {
					totalRecvBytes += Cam->recvTempSize;
					std::cout << "mvCam read: " << Cam->recvTempSize << " " << totalRecvBytes << "\n";

					for (int i = 0; i < Cam->recvTempSize; ++i) {
						uint8_t v = Cam->recvTemp[i];

						int newGoal = (recvCount / 256) % 256;

						if (v != newGoal) {
							std::cout << "Seq failure (" << recvCount << ") expected: " << newGoal << " got: " << (int)v << "\n";
							exit(0);
						}

						recvCount += 1;
					}

					/*while (Panther->recvTempPos < Panther->recvTempSize) {
						uint8_t d = Panther->recvTemp[Panther->recvTempPos++];
					}*/
				}
			}
		} else {
			std::cout << "Serial port failed to connect\n";
			Sleep(1000);
		}
	}
}

void mvCamInit(ldiMvCam* Cam, const std::string& PortPath) {
	std::cout << "Init machine vision cam\n";

	Cam->imgWidth = 1920;
	Cam->imgHeight = 1080;
	Cam->frameBuffer = new uint8_t[Cam->imgWidth * Cam->imgHeight];
	Cam->captureFrameBuffer = new uint8_t[Cam->imgWidth * Cam->imgHeight];
	Cam->serialPortPath = PortPath;

	Cam->recvTemp = new uint8_t[SERIAL_DEVICE_RECV_TEMP_SIZE];

	Cam->workerThread = std::thread(mvCamWorkerThread, Cam);
}

void mvCamUpdate(ldiMvCam* Cam) {

}

void mvCamTriggerAndGetFrame(ldiMvCam* Cam) {

}