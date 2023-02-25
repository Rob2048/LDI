#pragma once

#define PANTHER_PACKET_RECV_MAX (1024 * 1024)
#define PANTHER_PACKET_START 255
#define PANTHER_PACKET_END 254
#define PANTHER_RECV_TEMP_SIZE 4096

struct ldiPanther {
	ldiApp* appContext;

	std::thread				workerThread;
	std::atomic_bool		workerThreadRunning = true;

	std::atomic_bool		serialPortConnected = false;
	ldiSerialPort			serialPort;

	std::mutex				serialPortConnectMutex;
	std::condition_variable	serialPortConnectCondVar;

	uint8_t					recvTemp[PANTHER_RECV_TEMP_SIZE];
	int32_t					recvTempSize = 0;
	int32_t					recvTempPos = 0;

	uint8_t					recvPacketBuffer[PANTHER_PACKET_RECV_MAX];
	int32_t					recvPacketSize = 0;
	uint8_t					recvPacketState = 0;
	int32_t					recvPacketPayloadSize = 0;

	std::atomic_bool		operationExecuting = false;
	std::mutex				operationCompleteMutex;
	std::condition_variable operationCompleteCondVar;

	std::mutex				dataLockMutex;
	// TODO: These should be in steps.
	float positionX;
	float positionY;
	float positionZ;
	float positionA;
	float positionB;
};

void pantherProcessPacket(ldiPanther* Panther) {
	if (Panther->recvPacketPayloadSize == 0) {
		return;
	}

	uint8_t opcode = Panther->recvPacketBuffer[0];

	switch (opcode) {
		case 13: {
			std::cout << "Panther message: " << (Panther->recvPacketBuffer + 1) << "\n";
		} break;

		case 10: {
			float x = *(float*)(Panther->recvPacketBuffer + 1);
			float y = *(float*)(Panther->recvPacketBuffer + 5);
			float z = *(float*)(Panther->recvPacketBuffer + 9);
			unsigned long time = *(unsigned long*)(Panther->recvPacketBuffer + 13);

			Panther->dataLockMutex.lock();
			Panther->positionX = x;
			Panther->positionY = y;
			Panther->positionZ = z;
			Panther->dataLockMutex.unlock();

			std::cout << "Panther command success " << x << ", " << y << ", " << z << " " << time << "\n";

			std::unique_lock<std::mutex> lock(Panther->operationCompleteMutex);
			Panther->operationExecuting = false;
			Panther->operationCompleteCondVar.notify_all();
		} break;

		case 69: {
			float x = *(float*)(Panther->recvPacketBuffer + 1);
			float y = *(float*)(Panther->recvPacketBuffer + 5);
			float z = *(float*)(Panther->recvPacketBuffer + 9);

			Panther->dataLockMutex.lock();
			Panther->positionX = x;
			Panther->positionY = y;
			Panther->positionZ = z;
			Panther->dataLockMutex.unlock();

		} break;

		case 70: {
			/*float x = *(float*)(Panther->recvPacketBuffer + 1);
			float y = *(float*)(Panther->recvPacketBuffer + 5);
			float z = *(float*)(Panther->recvPacketBuffer + 9);*/
			unsigned long time = *(unsigned long*)(Panther->recvPacketBuffer + 1);

			std::cout << "Log " << time << "\n";
		} break;

		default: {
			std::cout << "Panther opcode " << (int)opcode << " not handled.\n";
		}
	}
}

void pantherDisconnect(ldiPanther* Panther) {
	if (Panther->serialPortConnected) {
		serialPortDisconnect(&Panther->serialPort);
		Panther->serialPortConnected = false;
	}
}

void pantherWorkerThread(ldiPanther* Panther) {
	std::cout << "Running panther thread\n";

	while (Panther->workerThreadRunning) {
		std::cout << "Panther thread wating for serial connection\n";
		std::unique_lock<std::mutex> lock(Panther->serialPortConnectMutex);
		// TODO: Need this to wake on error too.
		Panther->serialPortConnectCondVar.wait(lock);

		while (Panther->serialPortConnected) {
			Panther->recvTempSize = serialPortReadData(&Panther->serialPort, Panther->recvTemp, PANTHER_PACKET_RECV_MAX);
			Panther->recvTempPos = 0;

			if (Panther->recvTempSize == -1) {
				pantherDisconnect(Panther);
				break;
			} else if (Panther->recvTempSize == 0) {
				Sleep(1);
			} else {
				while (Panther->recvTempPos < Panther->recvTempSize) {
					uint8_t d = Panther->recvTemp[Panther->recvTempPos++];

					if (Panther->recvPacketState == 0) {
						if (d == PANTHER_PACKET_START) {
							Panther->recvPacketState = 1;
						} else {
							// NOTE: Bad data.
						}
					} else if (Panther->recvPacketState == 1) {
						Panther->recvPacketPayloadSize = d;
						Panther->recvPacketSize = 0;
						Panther->recvPacketState = 2;
						// NOTE: Assumes payload is at least 1 in size.
					} else if (Panther->recvPacketState == 2) {
						Panther->recvPacketBuffer[Panther->recvPacketSize++] = d;

						if (Panther->recvPacketSize == Panther->recvPacketPayloadSize) {
							Panther->recvPacketState = 3;
						}
					} else if (Panther->recvPacketState == 3) {
						Panther->recvPacketState = 0;

						if (d == PANTHER_PACKET_END) {
							//std::cout << "Process packet\n";
							pantherProcessPacket(Panther);
						}
					}
				}
			}
		}
	}

	std::cout << "Panther thread completed\n";
}

int pantherInit(ldiApp* AppContext, ldiPanther* Panther) {
	Panther->appContext = AppContext;

	Panther->workerThread = std::thread(pantherWorkerThread, Panther);

	return 1;
}

bool pantherConnect(ldiPanther* Panther, const char* Path) {
	pantherDisconnect(Panther);

	Panther->operationExecuting = false;
	// TODO: Reset packet recvs?

	if (serialPortConnect(&Panther->serialPort, Path, 921600)) {
		Panther->serialPortConnected = true;
		Panther->serialPortConnectCondVar.notify_all();
		return true;
	}

	return false;
}

void pantherDestroy(ldiPanther* Panther) {
	pantherDisconnect(Panther);

	Panther->workerThreadRunning = false;
	Panther->workerThread.join();
}

bool pantherIsExecuting(ldiPanther* Panther) {
	return Panther->operationExecuting;
}

void pantherWaitForExecutionComplete(ldiPanther* Panther) {
	if (!Panther->operationExecuting) {
		return;
	}

	std::unique_lock<std::mutex> lock(Panther->operationCompleteMutex);
	Panther->operationCompleteCondVar.wait(lock);
}

void pantherIssueCommand(ldiPanther* Panther, uint8_t* Data, int Size) {
	if (Panther->operationExecuting) {
		std::cout << "Panther can't issue new command while already executing!\n";
		return;
	}

	std::unique_lock<std::mutex> lock(Panther->operationCompleteMutex);
	Panther->operationExecuting = true;

	serialPortWriteData(&Panther->serialPort, Data, Size);
}

void pantherSendTestCommand(ldiPanther* Panther) {
	uint8_t cmd[5];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 1;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

void pantherSendPingCommand(ldiPanther* Panther) {
	uint8_t cmd[5];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 0;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

void pantherSendMoveRelativeCommand(ldiPanther* Panther, uint8_t AxisId, int32_t Steps, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 2;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 14);
}

void pantherSendMoveRelativeLogCommand(ldiPanther* Panther, uint8_t AxisId, int32_t Steps, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 22;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 14);
}

void pantherSendMoveRelativeDirectCommand(ldiPanther* Panther, uint8_t AxisId, int32_t Steps, int32_t Delay) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 21;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &Delay, 4);
	cmd[13] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 14);
}

void pantherSendReadMotorStatusCommand(ldiPanther* Panther) {
	uint8_t cmd[5];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 20;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

void pantherSendHomingTestCommand(ldiPanther* Panther) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 23;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}