#pragma once

#define PANTHER_PACKET_RECV_MAX (1024 * 1024)
#define PANTHER_PACKET_START 255
#define PANTHER_PACKET_END 254
#define PANTHER_RECV_TEMP_SIZE 4096

enum ldiPantherOpcode {
	PO_PING = 0,
	PO_DIAG = 1,
	
	PO_HOME = 10,
	PO_MOVE_RELATIVE = 11,
	PO_MOVE = 12,
	
	PO_LASER_BURST = 20,
	PO_LASER_PULSE = 21,
	PO_LASER_STOP = 22,
	PO_MODULATE_LASER = 23,

	PO_GALVO_PREVIEW_SQUARE = 30,
	PO_GALVO_MOVE = 31,
	PO_GALVO_BURN = 32,

	PO_CMD_RESPONSE = 100,
	PO_POSITION = 101,
	PO_MESSAGE = 102,
	PO_SETUP = 103,
};

enum ldiPantherStatus {
	PS_OK = 0,
	PS_ERROR = 1,
	PS_NOT_HOMED = 2,
	PS_INVALID_PARAM = 3,
};

enum ldiPantherAxis {
	PA_X = 0,
	PA_Y = 1,
	PA_Z = 2,
	PA_C = 3,
	PA_A = 4,
};

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

	// Shared data.
	std::mutex				dataLockMutex;
	ldiPantherStatus		lastSuccessState;
	bool					homed;
	int						positionX;
	int						positionY;
	int						positionZ;
	int						positionC;
	int						positionA;
};

void pantherProcessPacket(ldiPanther* Panther) {
	if (Panther->recvPacketPayloadSize == 0) {
		return;
	}

	uint8_t opcode = Panther->recvPacketBuffer[0];

	switch (opcode) {
		case PO_MESSAGE: {
			std::cout << "Panther message: " << (Panther->recvPacketBuffer + 1) << "\n";
		} break;

		case PO_CMD_RESPONSE: {
			int x = *(int*)(Panther->recvPacketBuffer + 1);
			int y = *(int*)(Panther->recvPacketBuffer + 5);
			int z = *(int*)(Panther->recvPacketBuffer + 9);
			int c = *(int*)(Panther->recvPacketBuffer + 13);
			int a = *(int*)(Panther->recvPacketBuffer + 17);
			int status = *(int*)(Panther->recvPacketBuffer + 21);
			int homed = *(int*)(Panther->recvPacketBuffer + 25);
			
			Panther->dataLockMutex.lock();
			Panther->positionX = x;
			Panther->positionY = y;
			Panther->positionZ = z;
			Panther->positionC = c;
			Panther->positionA = a;
			Panther->homed = homed;
			Panther->lastSuccessState = (ldiPantherStatus)status;
			Panther->dataLockMutex.unlock();

			std::cout << "Panther command response: " << status << "\n";

			std::unique_lock<std::mutex> lock(Panther->operationCompleteMutex);
			Panther->operationExecuting = false;
			Panther->operationCompleteCondVar.notify_all();
		} break;

		case PO_POSITION: {
			int x = *(int*)(Panther->recvPacketBuffer + 1);
			int y = *(int*)(Panther->recvPacketBuffer + 5);
			int z = *(int*)(Panther->recvPacketBuffer + 9);
			int c = *(int*)(Panther->recvPacketBuffer + 13);
			int a = *(int*)(Panther->recvPacketBuffer + 17);

			Panther->dataLockMutex.lock();
			Panther->positionX = x;
			Panther->positionY = y;
			Panther->positionZ = z;
			Panther->positionC = c;
			Panther->positionA = a;
			Panther->dataLockMutex.unlock();

		} break;

		default: {
			std::cout << "Panther opcode " << (int)opcode << " not handled.\n";
		}
	}
}

void pantherDisconnect(ldiPanther* Panther) {
	if (Panther->serialPortConnected) {
		Panther->serialPortConnected = false;
		serialPortDisconnect(&Panther->serialPort);
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
				// TODO: I hate this :(. Find a better way.
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

bool pantherWaitForExecutionComplete(ldiPanther* Panther) {
	if (!Panther->operationExecuting) {
		return true;
	}

	std::unique_lock<std::mutex> lock(Panther->operationCompleteMutex);
	Panther->operationCompleteCondVar.wait(lock);

	return Panther->lastSuccessState == PS_OK;
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


void pantherSendPingCommand(ldiPanther* Panther) {
	uint8_t cmd[5];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = PO_PING;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

void pantherSendDiagCommand(ldiPanther* Panther) {
	uint8_t cmd[5];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = PO_DIAG;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

void pantherSendMoveRelativeCommand(ldiPanther* Panther, ldiPantherAxis AxisId, int32_t Steps, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = PO_MOVE_RELATIVE;
	cmd[4] = (uint8_t)AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 14);
}

void pantherSendMoveCommand(ldiPanther* Panther, ldiPantherAxis AxisId, int32_t Step, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = PO_MOVE;
	cmd[4] = (uint8_t)AxisId;
	memcpy(cmd + 5, &Step, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 14);
}

void pantherSendHomingCommand(ldiPanther* Panther) {
	uint8_t cmd[64];

	cmd[0] = PANTHER_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = PO_HOME;
	cmd[4] = PANTHER_PACKET_END;

	pantherIssueCommand(Panther, cmd, 5);
}

bool pantherMoveAndWait(ldiPanther* Panther, ldiPantherAxis AxisId, int32_t Step, float MaxVelocity) {
	pantherSendMoveCommand(Panther, AxisId, Step, MaxVelocity);
	return pantherWaitForExecutionComplete(Panther);
}