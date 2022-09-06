#pragma once

struct ldiPlatform {
	ldiApp*			appContext;
	bool			connected = false;
};

int platformInit(ldiApp* AppContext, ldiPlatform* Platform) {
	Platform->appContext = AppContext;

	return 0;
}

void platformShowUi(ldiPlatform* tool) {
	static float f = 0.0f;
	static int counter = 0;

	//ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->WorkSize.y));
	//ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x, ImGui::GetMainViewport()->WorkPos.y), 0, ImVec2(1, 0));

	//ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
	ImGui::Begin("Platform controls", 0, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Connection");

	ImGui::BeginDisabled(tool->connected);
	char ipBuff[] = "192.168.0.50";
	int port = 5000;
	ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
	ImGui::InputInt("Port", &port);
	ImGui::EndDisabled();

	if (tool->connected) {
		if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
			tool->connected = false;
		};
		ImGui::Text("Status: Connected");
	}
	else {
		if (ImGui::Button("Connect", ImVec2(-1, 0))) {
			tool->connected = true;
		};
		ImGui::Text("Status: Disconnected");
	}

	ImGui::Separator();

	ImGui::BeginDisabled(!tool->connected);
	ImGui::Text("Position");
	ImGui::PushFont(tool->appContext->fontBig);

	float startX = ImGui::GetCursorPosX();
	float availX = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(startX);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: 0.00");
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3);
	ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: 0.00");
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 2);
	ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: 0.00");
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