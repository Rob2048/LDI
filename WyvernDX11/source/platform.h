#pragma once

enum ldiPlatformJobType {
	PJT_MOVE_AXIS,
	PJT_MOTOR_STATUS,
	PJT_HOMING_TEST,
	PJT_CAPTURE_CALIBRATION,
};

struct ldiPlatformJobHeader {
	int type;
};

struct ldiPlatformJobMoveAxis {
	ldiPlatformJobHeader header;
	int axisId;
	int steps;
	float velocity;
};

struct ldiPlatform {
	ldiApp*						appContext;

	std::thread					workerThread;
	std::atomic_bool			workerThreadRunning = true;
	std::atomic_bool			jobCancel = false;
	std::atomic_bool			jobExecuting = false;
	int							jobState = 0;
	ldiPlatformJobHeader*		job;
	std::mutex					jobAvailableMutex;
	std::condition_variable		jobAvailableCondVar;

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
	float						positionX;
	float						positionY;
	float						positionZ;
	float						positionA;
	float						positionB;
};

void platformWorkerThreadJobComplete(ldiPlatform* Platform) {
	delete Platform->job;
	Platform->jobState = 0;
	Platform->jobExecuting = false;
}

bool _platformCaptureCalibration(ldiPlatform* Platform) {
	ldiApp* appContext = Platform->appContext;
	ldiPanther* panther = appContext->panther;

	// TODO: Make sure directories exist for saving results.

	// TODO: Set cam to mode 0. Prepare other hardware.
	// TODO: Set starting point.

	for (int i = 0; i < 3; ++i) {
		if (Platform->jobCancel) {
			break;
		}

		std::cout << "Start move " << i << "\n";
		double t0 = _getTime(appContext);
		pantherSendMoveRelativeCommand(panther, 0, 8000, 30.0f);
		pantherWaitForExecutionComplete(panther);
		t0 = _getTime(appContext) - t0;
		std::cout << "Move: " << (t0 * 1000.0) << " ms\n";

		t0 = _getTime(appContext);
		Sleep(200);
		t0 = _getTime(appContext) - t0;
		std::cout << "Sleep: " << (t0 * 1000.0) << " ms\n";

		// Trigger camera start.
		t0 = _getTime(appContext);

		ldiProtocolMode setMode;
		setMode.header.packetSize = sizeof(ldiProtocolMode) - 4;
		setMode.header.opcode = 1;
		setMode.mode = 2;

		networkSend(&Platform->appContext->server, (uint8_t*)&setMode, sizeof(ldiProtocolMode));

		// Wait until next frame recvd.
		imageInspectorWaitForCameraFrame(Platform->appContext->imageInspector);
		t0 = _getTime(appContext) - t0;
		std::cout << "Frame: " << (t0 * 1000.0) << " ms\n";

		// NOTE: the frame data here is not yet thread protected. We just assume that because we are not callling for a new frame that it won't change.
		t0 = _getTime(appContext);

		char fileName[256];
		sprintf_s(fileName, sizeof(fileName), "../cache/calib/data_%d.dat", i);

		int imageWidth = CAM_IMG_WIDTH;
		int imageHeight = CAM_IMG_HEIGHT;

		FILE* file;
		fopen_s(&file, fileName, "wb");
		fwrite(&imageWidth, 4, 1, file);
		fwrite(&imageHeight, 4, 1, file);
		fwrite(Platform->appContext->imageInspector->camPixelsFinal, CAM_IMG_WIDTH * CAM_IMG_HEIGHT, 1, file);
		fclose(file);

		t0 = _getTime(appContext) - t0;
		std::cout << "Save: " << (t0 * 1000.0) << " ms\n";
	}

	platformWorkerThreadJobComplete(Platform);
}

void platformWorkerThread(ldiPlatform* Platform) {
	std::cout << "Running platform thread\n";

	while (Platform->workerThreadRunning) {
		Platform->jobCancel = false;

		std::unique_lock<std::mutex> lock(Platform->jobAvailableMutex);
		Platform->jobAvailableCondVar.wait(lock);

		while (Platform->jobExecuting) {
			ldiPanther* panther = Platform->appContext->panther;

			switch (Platform->job->type) {
				case PJT_MOVE_AXIS: {
					ldiPlatformJobMoveAxis* job = (ldiPlatformJobMoveAxis*)Platform->job;
					pantherSendMoveRelativeCommand(panther, job->axisId, job->steps, job->velocity);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_MOTOR_STATUS: {
					pantherSendReadMotorStatusCommand(panther);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_HOMING_TEST: {
					pantherSendHomingTestCommand(panther);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_CAPTURE_CALIBRATION: {
					_platformCaptureCalibration(Platform);
				} break;
			};
		}
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

	Tool->workerThread = std::thread(platformWorkerThread, Tool);

	return 0;
}

void platformDestroy(ldiPlatform* Platform) {
	Platform->workerThreadRunning = false;
	Platform->workerThread.join();
}

bool platformPrepareForNewJob(ldiPlatform* Platform) {
	if (Platform->jobExecuting) {
		std::cout << "Platform is already executing a job\n";
		return false;
	}

	return true;
}

void platformCancelJob(ldiPlatform* Platform) {
	Platform->jobCancel = true;

	// TODO: Cancel remaining queued jobs.
}

void platformQueueJob(ldiPlatform* Platform, ldiPlatformJobHeader* Job) {
	Platform->job = Job;
	Platform->jobState = 1;
	Platform->jobCancel = false;
	Platform->jobExecuting = true;
	Platform->jobAvailableCondVar.notify_all();
}

bool platformQueueJobMoveAxis(ldiPlatform* Platform, int AxisId, int Steps, float Velocity) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobMoveAxis* job = new ldiPlatformJobMoveAxis();
	job->header.type = PJT_MOVE_AXIS;
	job->axisId = AxisId;
	job->steps = Steps;
	job->velocity = Velocity;

	platformQueueJob(Platform, &job->header);

	return true;
}

bool platformQueueJobMotorStatus(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_MOTOR_STATUS;
	
	platformQueueJob(Platform, job);

	return true;
}

bool platformQueueJobHomingTest(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_HOMING_TEST;

	platformQueueJob(Platform, job);

	return true;
}

bool platformQueueJobCaptureCalibration(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_CAPTURE_CALIBRATION;

	platformQueueJob(Platform, job);

	return true;
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
	ldiApp* appContext = Tool->appContext;
	static float f = 0.0f;
	static int counter = 0;

	// TODO: Move update somewhere sensible.
	// Copy values from panther.
	appContext->panther->dataLockMutex.lock();
	Tool->positionX = appContext->panther->positionX;
	Tool->positionY = appContext->panther->positionY;
	Tool->positionZ = appContext->panther->positionZ;
	appContext->panther->dataLockMutex.unlock();

	Tool->horse.x = Tool->positionX / 10.0f - 7.5f;
	Tool->horse.y = Tool->positionY / 10.0f - 0.0f;
	Tool->horse.z = Tool->positionZ / 10.0f - 0.0f;
	horseUpdate(&Tool->horse);

	//ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->WorkSize.y));
	//ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x, ImGui::GetMainViewport()->WorkPos.y), 0, ImVec2(1, 0));

	//ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
	ImGui::Begin("Platform controls", 0, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Connection");
	ImGui::BeginDisabled(appContext->panther->serialPortConnected);
	/*char ipBuff[] = "192.168.0.50";
	int port = 5000;
	ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
	ImGui::InputInt("Port", &port);*/

	char comPortPath[7] = "COM4";
	ImGui::InputText("COM path", comPortPath, sizeof(comPortPath));

	ImGui::EndDisabled();

	if (appContext->panther->serialPortConnected) {
		if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
			pantherDisconnect(appContext->panther);
		};
	} else {
		if (ImGui::Button("Connect", ImVec2(-1, 0))) {
			if (pantherConnect(appContext->panther, "\\\\.\\COM4")) {
				pantherSendTestCommand(appContext->panther);
			}
		}
	}

	ImGui::Separator();

	ImGui::BeginDisabled(!appContext->panther->serialPortConnected);
	ImGui::Text("Platform status");
	ImGui::PushFont(Tool->appContext->fontBig);

	const char* labelStrs[] = {
		"Disconnected",
		"Connected",
		"Idle",
		"Cancelling job",
		"Running job",

	};

	ImColor labelColors[] = {
		ImColor(0, 0, 0, 50),
		ImColor(184, 179, 55, 255),
		ImColor(61, 184, 55, 255),
		ImColor(186, 28, 28, 255),
		ImColor(179, 72, 27, 255),
	};

	int labelIndex = 0;

	if (appContext->panther->serialPortConnected) {
		labelIndex = 1;

		if (Tool->jobExecuting) {
			if (Tool->jobCancel) {
				labelIndex = 3;
			} else {
				labelIndex = 4;
			}
		} else {
			labelIndex = 2;
		}
	}

	float startX = ImGui::GetCursorPosX();
	float availX = ImGui::GetContentRegionAvail().x;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImGuiStyle style = ImGui::GetStyle();
	ImVec2 bannerCursorStart = ImGui::GetCursorPos();
	ImVec2 bannerMin = ImGui::GetCursorScreenPos() + ImVec2(0.0f, style.FramePadding.y);
	ImVec2 bannerMax = bannerMin + ImVec2(availX, 32);
	int bannerHeight = 32;

	drawList->AddRectFilled(bannerMin, bannerMax, labelColors[labelIndex]);

	ImVec2 bannerCursorEnd = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + bannerHeight + style.FramePadding.y * 2);

	ImVec2 labelSize = ImGui::CalcTextSize(labelStrs[labelIndex]);
	ImGui::SetCursorPos(bannerCursorStart + ImVec2(availX / 2.0f - labelSize.x / 2.0f, bannerHeight / 2.0f - labelSize.y / 2.0f + style.FramePadding.y));
	ImGui::Text(labelStrs[labelIndex]);
	ImGui::SetCursorPos(bannerCursorEnd);
	ImGui::PopFont();

	ImGui::BeginDisabled(!Tool->jobExecuting || Tool->jobCancel);

	if (ImGui::Button("Cancel job", ImVec2(-1, 0))) {
		platformCancelJob(Tool);
	}
	
	ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Text("Position");

	ImGui::PushFont(Tool->appContext->fontBig);
	ImGui::SetCursorPosX(startX);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: %.2f", Tool->positionX);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3);
	ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: %.2f", Tool->positionY);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 2);
	ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: %.2f", Tool->positionZ);
	
	ImGui::SetCursorPosX(startX + availX / 3 * 0);
	ImGui::TextColored(ImVec4(0.921f, 0.921f, 0.125f, 1.0f), "A: %.2f", Tool->positionA);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 1);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.921f, 1.0f), "B: %.2f", Tool->positionB);
	ImGui::PopFont();

	ImGui::BeginDisabled(Tool->jobExecuting);
	
	ImGui::Separator();

	float distance = 1;
	ImGui::InputFloat("Distance", &distance);

	static int steps = 10000;
	ImGui::InputInt("Steps", &steps);
	static float accel = 30.0f;
	ImGui::InputFloat("Velocity", &accel);

	if (ImGui::Button("-X", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 0, -steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("-Y", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 1, -steps, accel);
	}
	ImGui::SameLine();		
	if (ImGui::Button("-Z", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 2, -steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("-A", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 3, -steps, accel);
	}

	if (ImGui::Button("+X", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 0, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Y", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 1, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Z", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 2, steps, accel);
	}
	ImGui::SameLine();
	if (ImGui::Button("+A", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, 3, steps, accel);
	}

	ImGui::Separator();
	ImGui::Text("Tasks");

	ImGui::Button("Find home", ImVec2(-1, 0));
	ImGui::Button("Go home", ImVec2(-1, 0));

	if (ImGui::Button("Read motor status", ImVec2(-1, 0))) {
		platformQueueJobMotorStatus(Tool);
	}

	if (ImGui::Button("Perform homing test", ImVec2(-1, 0))) {
		platformQueueJobHomingTest(Tool);
	}

	/*ImGui::Separator();
	static int stepDelay = 300;
	ImGui::InputInt("Step delay", &stepDelay);

	if (ImGui::Button("-X Direct")) {
		pantherSendMoveRelativeDirectCommand(appContext->panther, 0, -steps, stepDelay);
	}
	if (ImGui::Button("+X Direct")) {
		pantherSendMoveRelativeDirectCommand(appContext->panther, 0, steps, stepDelay);
	}*/

	if (ImGui::Button("Capture calibration", ImVec2(-1, 0))) {
		platformQueueJobCaptureCalibration(Tool);
	}

	ImGui::Button("Start laser preview", ImVec2(-1, 0));

	ImGui::EndDisabled();

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