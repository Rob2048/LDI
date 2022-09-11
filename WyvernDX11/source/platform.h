#pragma once

struct ldiPlatform {
	ldiApp*			appContext;

	ldiSerialPort	serialPort;

	std::thread			workerThread;
	std::atomic_bool	workerThreadRunning = true;

	float positionX;
	float positionY;
	float positionZ;
};

void platformWorkerThread(ldiPlatform* Platform) {
	std::cout << "Running platform thread\n";

	while (Platform->workerThreadRunning) {
		// Dispatch next job to platform.
		// Wait for response from last dispatch.

		// Update main thread details.

		Sleep(100);
	}

	std::cout << "Platform thread completed\n";
}

int platformInit(ldiApp* AppContext, ldiPlatform* Platform) {
	Platform->appContext = AppContext;

	std::thread workerThread(platformWorkerThread, Platform);
	Platform->workerThread = std::move(workerThread);

	return 0;
}

void platformDestroy(ldiPlatform* Platform) {
	Platform->workerThreadRunning = false;
	Platform->workerThread.join();
}

void platformShowUi(ldiPlatform* Tool) {
	static float f = 0.0f;
	static int counter = 0;

	//ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->WorkSize.y));
	//ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x, ImGui::GetMainViewport()->WorkPos.y), 0, ImVec2(1, 0));

	//ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
	ImGui::Begin("Platform controls", 0, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Connection");

	ImGui::BeginDisabled(Tool->serialPort.connected);
	char ipBuff[] = "192.168.0.50";
	int port = 5000;
	ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
	ImGui::InputInt("Port", &port);
	ImGui::EndDisabled();

	if (Tool->serialPort.connected) {
		if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
			serialPortDisconnect(&Tool->serialPort);
		};
		ImGui::Text("Status: Connected");
	}
	else {
		if (ImGui::Button("Connect", ImVec2(-1, 0))) {
			serialPortConnect(&Tool->serialPort, "\\\\.\\COM39", 921600);
		};
		ImGui::Text("Status: Disconnected");
	}

	ImGui::Separator();

	ImGui::BeginDisabled(!Tool->serialPort.connected);
	ImGui::Text("Position");
	ImGui::PushFont(Tool->appContext->fontBig);

	float startX = ImGui::GetCursorPosX();
	float availX = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(startX);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: %.2f", Tool->positionX);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3);
	ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: %.2f", Tool->positionY);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 2);
	ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: %.2f", Tool->positionZ);
	ImGui::PopFont();

	ImGui::Button("Find home", ImVec2(-1, 0));
	ImGui::Button("Go home", ImVec2(-1, 0));

	float distance = 1;
	ImGui::InputFloat("Distance", &distance);

	ImGui::Button("-X", ImVec2(32, 32));
	ImGui::SameLine();
	ImGui::Button("-Y", ImVec2(32, 32));
	ImGui::SameLine();
	ImGui::Button("-Z", ImVec2(32, 32));

	ImGui::Button("+X", ImVec2(32, 32));
	ImGui::SameLine();
	ImGui::Button("+Y", ImVec2(32, 32));
	ImGui::SameLine();
	ImGui::Button("+Z", ImVec2(32, 32));

	ImGui::Separator();
	ImGui::Text("Laser");
	ImGui::Button("Start laser preview", ImVec2(-1, 0));
	ImGui::EndDisabled();

	ImGui::End();
}