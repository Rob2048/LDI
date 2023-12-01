#pragma once

#define ANALOG_SCOPE_PACKET_RECV_SIZE (1024 * 1024)

struct ldiAnalogScope {
	ldiApp* appContext;

	std::thread				workerThread;
	std::atomic_bool		workerThreadRunning = true;

	std::atomic_bool		serialPortConnected = false;
	ldiSerialPort			serialPort;

	std::mutex				serialPortConnectMutex;
	std::condition_variable	serialPortConnectCondVar;

	uint8_t					recvTemp[ANALOG_SCOPE_PACKET_RECV_SIZE];
	int32_t					recvTempSize = 0;
	int32_t					recvTempPos = 0;

	// Shared data.
	std::mutex				samplesLockMutex;
	uint8_t*				samples;
	std::atomic_int			sampleCount;

	// Main thread only.
	float					samplesF[1024 * 1024];
};

void analogScopeDisconnect(ldiAnalogScope* Scope) {
	if (Scope->serialPortConnected) {
		Scope->serialPortConnected = false;
		serialPortDisconnect(&Scope->serialPort);
	}
}

void analogScopeWorkerThread(ldiAnalogScope* Scope) {
	std::cout << "Running analog scope thread\n";

	while (Scope->workerThreadRunning) {
		std::cout << "Analogs scope thread wating for serial connection\n";
		std::unique_lock<std::mutex> lock(Scope->serialPortConnectMutex);
		// TODO: Need this to wake on error too.
		Scope->serialPortConnectCondVar.wait(lock);

		std::cout << "Got connection\n";

		while (Scope->serialPortConnected) {
			Scope->recvTempSize = serialPortReadData(&Scope->serialPort, Scope->recvTemp, ANALOG_SCOPE_PACKET_RECV_SIZE);
			Scope->recvTempPos = 0;

			if (Scope->recvTempSize == -1) {
				analogScopeDisconnect(Scope);
				break;
			} else if (Scope->recvTempSize == 0) {
				// TODO: I hate this :(. Find a better way.
				Sleep(1);
			} else {

				//std::unique_lock<std::mutex> lock(Scope->samplesLockMutex);
				memcpy(Scope->samples + Scope->sampleCount, Scope->recvTemp, Scope->recvTempSize);
				Scope->sampleCount += Scope->recvTempSize;
				//std::cout << "Got data: " << Scope->sampleCount << " " << Scope->recvTempSize << "\n";
			}
		}
	}

	std::cout << "Panther thread completed\n";
}

int analogScopeInit(ldiApp* AppContext, ldiAnalogScope* Scope) {
	Scope->appContext = AppContext;
	Scope->workerThread = std::thread(analogScopeWorkerThread, Scope);

	Scope->samples = new uint8_t[1024 * 1024 * 1024];
	Scope->sampleCount = 0;

	return 1;
}

bool analogScopeConnect(ldiAnalogScope* Scope, const char* Path) {
	analogScopeDisconnect(Scope);

	if (serialPortConnect(&Scope->serialPort, Path, 921600)) {
		Scope->serialPortConnected = true;
		Scope->serialPortConnectCondVar.notify_all();
		return true;
	}

	return false;
}

void analogScopeDestroy(ldiAnalogScope* Scope) {
	analogScopeDisconnect(Scope);

	Scope->workerThreadRunning = false;
	Scope->workerThread.join();
}

void analogScopeShowUi(ldiAnalogScope* Scope) {
	if (ImGui::Begin("Analog scope", 0, ImGuiWindowFlags_NoCollapse)) {
		int sampleTotal = Scope->sampleCount;
		ImGui::Text("Samples: %d", sampleTotal);

		static int lastTriggerIndex = 0;

		// ~500000 samples per second.

		bool useTrigger = true;
		int triggerLevel = 70;
		int triggerOffset = -400;
		int nextSearchOffset = 4000;
		int minAfterTrigger = 4000;

		if (useTrigger) {
			// Search X samples ahead only.
			for (int i = lastTriggerIndex + nextSearchOffset; i < sampleTotal - minAfterTrigger; ++i) {
				if (Scope->samples[i] > triggerLevel) {
					lastTriggerIndex = i + triggerOffset;
					break;
				}
			}
		} else {
			lastTriggerIndex = sampleTotal - minAfterTrigger;
		}

		int sampleCount = sampleTotal - lastTriggerIndex;

		if (sampleCount > 4000) {
			sampleCount = 4000;
		} else if (sampleCount < 0) {
			sampleCount = 0;
		}

		for (int i = 0; i < sampleCount; ++i) {
			Scope->samplesF[i] = Scope->samples[lastTriggerIndex + i];
		}

		//ImGui::PlotLines("Samples", Scope->samplesF, 256, 0, 0, 0, 75.0f, ImVec2(0, 120.0f));
		ImGui::PlotLines("Samples", Scope->samplesF, 4000, 0, 0, 0, 256, ImVec2(1700, 800));
	}
	ImGui::End();
}