#pragma once

#define PLATFORM_PACKET_RECV_MAX (1024 * 1024)
#define PLATFORM_PACKET_START 255
#define PLATFORM_PACKET_END 254
#define PLATFORM_RECV_TEMP_SIZE 4096

struct ldiPlatform {
	ldiApp*			appContext;

	ldiSerialPort	serialPort;

	std::thread			workerThread;
	std::atomic_bool	workerThreadRunning = true;

	uint8_t recvTemp[PLATFORM_RECV_TEMP_SIZE];
	int32_t recvTempSize = 0;
	int32_t recvTempPos = 0;

	uint8_t recvPacketBuffer[PLATFORM_PACKET_RECV_MAX];
	int32_t recvPacketSize = 0;
	uint8_t recvPacketState = 0;
	int32_t recvPacketPayloadSize = 0;

	float positionX;
	float positionY;
	float positionZ;

	ldiCamera					camera;
	int							imageWidth;
	int							imageHeight;
	float						imageScale;
	vec2						imageOffset;

	ldiRenderViewBuffers		mainViewBuffers;
	int							mainViewWidth;
	int							mainViewHeight;
	ldiCamera					mainCamera;
	float						mainCameraSpeed = 1.0f;
	vec4						mainBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	ldiHorse					horse;
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


int platformInit(ldiApp* AppContext, ldiPlatform* Tool) {
	Tool->appContext = AppContext;

	Tool->imageScale = 1.0f;
	Tool->imageOffset = vec2(0.0f, 0.0f);
	Tool->imageWidth = 1600;
	Tool->imageHeight = 1300;

	Tool->camera = {};
	// NOTE: Camera location relative to machine origin: 56.0, 0, 51.0.
	Tool->camera.position = vec3(26.0f, 51.0f, 0);
	Tool->camera.rotation = vec3(-90, 45, 0);
	Tool->camera.projMat = glm::perspectiveFovRH_ZO(glm::radians(42.0f), (float)Tool->imageWidth, (float)Tool->imageHeight, 0.01f, 50.0f);
	Tool->camera.fov = 42.0f;

	Tool->mainCamera.position = vec3(-40, 60, 20);
	Tool->mainCamera.rotation = vec3(55, 40, 0);
	Tool->mainCamera.fov = 60.0f;

	horseInit(&Tool->horse);

	std::thread workerThread(platformWorkerThread, Tool);
	Tool->workerThread = std::move(workerThread);

	return 0;
}

void platformDestroy(ldiPlatform* Platform) {
	Platform->workerThreadRunning = false;
	Platform->workerThread.join();

	serialPortDisconnect(&Platform->serialPort);
}

void platformSendPingCommand(ldiPlatform* Platform) {
	uint8_t cmd[5];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 0;
	cmd[4] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 5);
}

void platformSendTestCommand(ldiPlatform* Platform) {
	uint8_t cmd[5];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 1;
	cmd[4] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 5);
}

void platformSendMoveRelativeCommand(ldiPlatform* Platform, uint8_t AxisId, int32_t Steps, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 2;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 14);
}

void platformSendMoveRelativeLogCommand(ldiPlatform* Platform, uint8_t AxisId, int32_t Steps, float MaxVelocity) {
	uint8_t cmd[64];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 22;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &MaxVelocity, 4);
	cmd[13] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 14);
}

void platformSendMoveRelativeDirectCommand(ldiPlatform* Platform, uint8_t AxisId, int32_t Steps, int32_t Delay) {
	uint8_t cmd[64];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 10;
	cmd[3] = 21;
	cmd[4] = AxisId;
	memcpy(cmd + 5, &Steps, 4);
	memcpy(cmd + 9, &Delay, 4);
	cmd[13] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 14);
}

void platformSendReadMotorStatusCommand(ldiPlatform* Platform) {
	uint8_t cmd[64];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 20;
	cmd[4] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 5);
}

void platformSendHomingTestCommand(ldiPlatform* Platform) {
	uint8_t cmd[64];

	cmd[0] = PLATFORM_PACKET_START;
	*(uint16_t*)(cmd + 1) = 1;
	cmd[3] = 23;
	cmd[4] = PLATFORM_PACKET_END;

	serialPortWriteData(&Platform->serialPort, cmd, 5);
}

bool platformGetNextPacket(ldiPlatform* Platform) {
	while (true) {
		while (Platform->recvTempPos < Platform->recvTempSize) {
			uint8_t d = Platform->recvTemp[Platform->recvTempPos++];

			if (Platform->recvPacketState == 0) {
				if (d == PLATFORM_PACKET_START) {
					Platform->recvPacketState = 1;
				} else {
					// NOTE: Bad data.
				}
			} else if (Platform->recvPacketState == 1) {
				Platform->recvPacketPayloadSize = d;
				Platform->recvPacketSize = 0;
				Platform->recvPacketState = 2;
				// NOTE: Assumes payload is at least 1 in size.
			} else if (Platform->recvPacketState == 2) {
				Platform->recvPacketBuffer[Platform->recvPacketSize++] = d;

				if (Platform->recvPacketSize == Platform->recvPacketPayloadSize) {
					Platform->recvPacketState = 3;
				}
			} else if (Platform->recvPacketState == 3) {
				Platform->recvPacketState = 0;

				if (d == PLATFORM_PACKET_END) {
					return true;
				}
			}
		}

		Platform->recvTempSize = serialPortReadData(&Platform->serialPort, Platform->recvTemp, PLATFORM_RECV_TEMP_SIZE);
		Platform->recvTempPos = 0;

		if (Platform->recvTempSize == 0) {
			return false;
		}
	}
}

void platformProcessPacket(ldiPlatform* Platform) {
	if (Platform->recvPacketPayloadSize == 0) {
		return;
	}

	uint8_t opcode = Platform->recvPacketBuffer[0];

	switch (opcode) {
		case 13: {
			std::cout << "Platform message: " << (Platform->recvPacketBuffer + 1) << "\n";
		} break;

		case 10: {
			float x = *(float*)(Platform->recvPacketBuffer + 1);
			float y = *(float*)(Platform->recvPacketBuffer + 5);
			float z = *(float*)(Platform->recvPacketBuffer + 9);
			unsigned long time = *(unsigned long*)(Platform->recvPacketBuffer + 13);
			Platform->positionX = x;
			Platform->positionY = y;
			Platform->positionZ = z;

			std::cout << "Platform command success " << x << ", " << y << ", " << z << " " << time << "\n";
		} break;


		case 69: {
			float x = *(float*)(Platform->recvPacketBuffer + 1);
			float y = *(float*)(Platform->recvPacketBuffer + 5);
			float z = *(float*)(Platform->recvPacketBuffer + 9);
			Platform->positionX = x;
			Platform->positionY = y;
			Platform->positionZ = z;

		} break;

		case 70: {
			/*float x = *(float*)(Platform->recvPacketBuffer + 1);
			float y = *(float*)(Platform->recvPacketBuffer + 5);
			float z = *(float*)(Platform->recvPacketBuffer + 9);*/
			unsigned long time = *(unsigned long*)(Platform->recvPacketBuffer + 1);
			
			std::cout << "Log " << time << "\n";
		} break;

		default: {
			std::cout << "Platform opcode " << (int)opcode << " not handled.\n";
		}
	}
}

void platformMainRender(ldiPlatform* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->mainViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	updateCamera3dBasicFps(&Tool->mainCamera, (float)Tool->mainViewWidth, (float)Tool->mainViewHeight);

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(&appContext->defaultDebug);
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	//----------------------------------------------------------------------------------------------------
	// Grid.
	//----------------------------------------------------------------------------------------------------
	int gridCount = 30;
	float gridCellWidth = 1.0f;
	vec3 gridColor = Tool->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(&appContext->defaultDebug, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(&appContext->defaultDebug, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->mainViewBuffers.mainViewRtView, Tool->mainViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->mainViewWidth;
	viewport.Height = Tool->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->mainViewBuffers.mainViewRtView, (float*)&Tool->mainBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->mainViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------------------------
	// Horse.
	//----------------------------------------------------------------------------------------------------
	{
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.origin, "Origin", TextBuffer);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisX, "X", TextBuffer);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisY, "Y", TextBuffer);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisZ, "Z", TextBuffer);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisA, "A", TextBuffer);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisB, "B", TextBuffer);

		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		ldiTransform mvCameraTransform = {};
		mvCameraTransform.world = glm::inverse(camViewMat);
		_renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &mvCameraTransform, "Camera", TextBuffer);

		// Render camera frustum.
		mat4 projViewMat = Tool->camera.projMat * camViewMat;
		mat4 invProjViewMat = glm::inverse(projViewMat);

		vec4 f0 = invProjViewMat * vec4(-1, -1, 0, 1); f0 /= f0.w;
		vec4 f1 = invProjViewMat * vec4(1, -1, 0, 1); f1 /= f1.w;
		vec4 f2 = invProjViewMat * vec4(1, 1, 0, 1); f2 /= f2.w;
		vec4 f3 = invProjViewMat * vec4(-1, 1, 0, 1); f3 /= f3.w;

		vec4 f4 = invProjViewMat * vec4(-1, -1, 1, 1); f4 /= f4.w;
		vec4 f5 = invProjViewMat * vec4(1, -1, 1, 1); f5 /= f5.w;
		vec4 f6 = invProjViewMat * vec4(1, 1, 1, 1); f6 /= f6.w;
		vec4 f7 = invProjViewMat * vec4(-1, 1, 1, 1); f7 /= f7.w;

		pushDebugLine(&appContext->defaultDebug, f0, f4, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f1, f5, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f2, f6, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f3, f7, vec3(1, 0, 1));

		pushDebugLine(&appContext->defaultDebug, f0, f1, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f1, f2, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f2, f3, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f3, f0, vec3(1, 0, 1));

		pushDebugLine(&appContext->defaultDebug, f4, f5, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f5, f6, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f6, f7, vec3(1, 0, 1));
		pushDebugLine(&appContext->defaultDebug, f7, f4, vec3(1, 0, 1));
	}

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = Tool->mainCamera.projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
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
	/*char ipBuff[] = "192.168.0.50";
	int port = 5000;
	ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
	ImGui::InputInt("Port", &port);*/

	char comPortPath[7] = "COM4";
	ImGui::InputText("COM path", comPortPath, sizeof(comPortPath));

	ImGui::EndDisabled();

	if (Tool->serialPort.connected) {
		if (platformGetNextPacket(Tool)) {
			platformProcessPacket(Tool);
		}
		
		if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
			serialPortDisconnect(&Tool->serialPort);
		};
		ImGui::Text("Status: Connected");
	} else {
		//ImGui::Text("Checking for serial ports...");

		// NOTE: Update serial port list at interval;
		//static float updateListTime = 0.0f;
		//static int updateCount = 0;

		//updateListTime += ImGui::GetIO().DeltaTime;

		//if (updateListTime >= 1.0f) {
		//	++updateCount;
		//	updateListTime -= 1.0f;

		//	// https://github.com/serialport/bindings-cpp/blob/main/src/serialport_win.cpp
		//}

		//ImGui::Text("%d", updateCount);

		if (ImGui::Button("Connect", ImVec2(-1, 0))) {
			if (serialPortConnect(&Tool->serialPort, "\\\\.\\COM4", 921600)) {
				//platformSendPingCommand(Tool);
				platformSendTestCommand(Tool);
			}
		};

		ImGui::Text("Status: Disconnected");
	}

	// TODO: Move update somewhere sensible.
	Tool->horse.x = Tool->positionX / 10.0f - 7.5f;
	Tool->horse.y = Tool->positionY / 10.0f - 0.0f;
	Tool->horse.z = Tool->positionZ / 10.0f - 0.0f;
	horseUpdate(&Tool->horse);

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

	static int steps = 10000;
	ImGui::InputInt("Steps", &steps);
	static float accel = 30.0f;
	ImGui::InputFloat("Velocity", &accel);

	if (ImGui::Button("-X", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 0, -steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("-Y", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 1, -steps, accel);
	}
	ImGui::SameLine();		
	if (ImGui::Button("-Z", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 2, -steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("-A", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 3, -steps, accel);
	}

	if (ImGui::Button("+X", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 0, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Y", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 1, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Z", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 2, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+A", ImVec2(32, 32))) {
		platformSendMoveRelativeCommand(Tool, 3, steps, accel);
	}

	ImGui::Separator();
	if (ImGui::Button("Read motor status", ImVec2(-1, 0))) {
		platformSendReadMotorStatusCommand(Tool);
	}

	if (ImGui::Button("Perform homing test", ImVec2(-1, 0))) {
		platformSendHomingTestCommand(Tool);
		// 564 - 612
		// 300 - 308
		// 276 - 316
	}

	ImGui::Separator();
	static int stepDelay = 300;
	ImGui::InputInt("Step delay", &stepDelay);

	if (ImGui::Button("-X Direct")) {
		platformSendMoveRelativeDirectCommand(Tool, 0, -steps, stepDelay);
	}
	if (ImGui::Button("+X Direct")) {
		platformSendMoveRelativeDirectCommand(Tool, 0, steps, stepDelay);
	}

	ImGui::Separator();
	ImGui::Text("Laser");
	ImGui::Button("Start laser preview", ImVec2(-1, 0));
	ImGui::EndDisabled();

	ImGui::End();

	//----------------------------------------------------------------------------------------------------
	// Platform 3D view.
	//----------------------------------------------------------------------------------------------------
	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Platform 3D view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held

		{
			vec3 camMove(0, 0, 0);
			ldiCamera* camera = &Tool->mainCamera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			if (isActive && (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				if (ImGui::IsKeyDown(ImGuiKey_W)) {
					camMove += vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_S)) {
					camMove -= vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_A)) {
					camMove -= vec3(vec4(vec3Right, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_D)) {
					camMove += vec3(vec4(vec3Right, 0.0f) * viewRotMat);
				}
			}

			if (glm::length(camMove) > 0.0f) {
				camMove = glm::normalize(camMove);
				float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime * Tool->mainCameraSpeed;
				camera->position += camMove * cameraSpeed;
			}
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.15f;
			Tool->mainCamera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.025f;

			ldiCamera* camera = &Tool->mainCamera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
			camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
		}

		ImGui::SetCursorPos(startPos);
		std::vector<ldiTextInfo> textBuffer;
		platformMainRender(Tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(Tool->mainViewBuffers.mainViewResourceView, viewSize);

		// NOTE: ImDrawList API uses screen coordinates!
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int i = 0; i < textBuffer.size(); ++i) {
			ldiTextInfo* info = &textBuffer[i];
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), ImColor(info->color.r, info->color.g, info->color.b, info->color.a), info->text.c_str());
		}

		// Viewport overlay widgets.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 25), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::EndChild();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}