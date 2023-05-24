#pragma once

// NOTE: OV9281
//#define CAM_IMG_WIDTH 1280
//#define CAM_IMG_HEIGHT 800

// NOTE: OV2311
#define CAM_IMG_WIDTH 1600
#define CAM_IMG_HEIGHT 1300

// NOTE: IMX477
//#define CAM_IMG_WIDTH 4056
//#define CAM_IMG_HEIGHT 3040

enum ldiImageInspectorMode {
	IIM_LIVE_CAMERA,
	IIM_CALIBRATION_JOB
};

struct ldiCalibrationJob {
	std::vector<std::string> fileSamples;
};

struct ldiImageInspector {
	ldiApp*						appContext;

	ldiImageInspectorMode		imageMode = IIM_LIVE_CAMERA;

	float						camImageFilterFactor = 0.0f;

	ldiCharucoResults			camImageCharucoResults = {};

	vec2						camImageCursorPos;
	uint8_t						camImagePixelValue;
	int							camImageShutterSpeed = 8000;
	int							camImageAnalogGain = 1;
	float						camImageGainR = 1.0f;
	float						camImageGainG = 1.0f;
	float						camImageGainB = 1.0f;
	bool						camImageProcess = false;

	ID3D11Buffer*				camImagePixelConstants;
	ID3D11ShaderResourceView*	camResourceView;
	ID3D11Texture2D*			camTex;

	std::mutex					camNewFrameMutex;
	std::condition_variable		camNewFrameCondVar;

	int							camLastNewFrameId = 0;
	std::atomic_int				camNewFrameId = 0;
	float						camPixels[CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};
	uint8_t						camPixelsFinal[CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};

	ldiCalibrationJob			calibJob;
	int							calibJobSelectedSampleId;
	uint8_t						calibJobImage[CAM_IMG_WIDTH * CAM_IMG_HEIGHT];
};

bool imageInspectorWaitForCameraFrame(ldiImageInspector* Inspector) {
	std::unique_lock<std::mutex> lock(Inspector->camNewFrameMutex);
	Inspector->camNewFrameCondVar.wait(lock);

	return true;
}

void _imageInspectorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;
	ldiApp* appContext = tool->appContext;

	//AddDrawCmd ??
	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
	appContext->d3dDeviceContext->PSSetShader(appContext->imgCamPixelShader, NULL, 0);
	appContext->d3dDeviceContext->VSSetShader(appContext->imgCamVertexShader, NULL, 0);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(tool->camImagePixelConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCamImagePixelConstants* constantBuffer = (ldiCamImagePixelConstants*)ms.pData;
		constantBuffer->params = vec4(tool->camImageGainR, tool->camImageGainG, tool->camImageGainB, 0);
		appContext->d3dDeviceContext->Unmap(tool->camImagePixelConstants, 0);
	}

	appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &tool->camImagePixelConstants);
}

void imageInspectorHandlePacket(ldiImageInspector* Tool, ldiPacketView* PacketView) {
	ldiProtocolHeader* packetHeader = (ldiProtocolHeader*)PacketView->data;

	if (packetHeader->opcode == 0) {
		ldiProtocolSettings* packet = (ldiProtocolSettings*)PacketView->data;
		std::cout << "Got settings: " << packet->shutterSpeed << " " << packet->analogGain << "\n";

		Tool->camImageShutterSpeed = packet->shutterSpeed;
		Tool->camImageAnalogGain = packet->analogGain;

	} else if (packetHeader->opcode == 1) {
		ldiProtocolImageHeader* imageHeader = (ldiProtocolImageHeader*)PacketView->data;
		//std::cout << "Got image " << imageHeader->width << " " << imageHeader->height << "\n";

		uint8_t* frameData = PacketView->data + sizeof(ldiProtocolImageHeader);

		for (int i = 0; i < CAM_IMG_WIDTH * CAM_IMG_HEIGHT; ++i) {
			Tool->camPixels[i] = Tool->camPixels[i] * Tool->camImageFilterFactor + frameData[i] * (1.0f - Tool->camImageFilterFactor);
			Tool->camPixelsFinal[i] = Tool->camPixels[i];
		}

		Tool->camNewFrameId++;
		Tool->camNewFrameCondVar.notify_all();
	} else {
		std::cout << "Error: Got unknown opcode (" << packetHeader->opcode << ") from packet\n";
	}
}

int imageInspectorInit(ldiApp* AppContext, ldiImageInspector* Tool) {
	Tool->appContext = AppContext;

	//----------------------------------------------------------------------------------------------------
	// Cam image constant buffer.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(ldiCamImagePixelConstants);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		AppContext->d3dDevice->CreateBuffer(&desc, NULL, &Tool->camImagePixelConstants);
	}

	//----------------------------------------------------------------------------------------------------
	// Prime camera texture.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = CAM_IMG_WIDTH;
		tex2dDesc.Height = CAM_IMG_HEIGHT;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DYNAMIC;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		tex2dDesc.MiscFlags = 0;

		if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->camTex) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}

		if (AppContext->d3dDevice->CreateShaderResourceView(Tool->camTex, NULL, &Tool->camResourceView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}
	}

	return 0;
}

void _imageInspectorCopyFrameDataToTexture(ldiImageInspector* Tool, uint8_t* FrameData) {
	D3D11_MAPPED_SUBRESOURCE ms;
	Tool->appContext->d3dDeviceContext->Map(Tool->camTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	uint8_t* pixelData = (uint8_t*)ms.pData;

	for (int i = 0; i < CAM_IMG_HEIGHT; ++i) {
		//memcpy(pixelData + i * ms.RowPitch, Tool->camPixelsFinal + i * 4064, 4064);
		memcpy(pixelData + i * ms.RowPitch, FrameData + i * CAM_IMG_WIDTH, CAM_IMG_WIDTH);
	}

	Tool->appContext->d3dDeviceContext->Unmap(Tool->camTex, 0);
}

void _imageInspectorSelectCalibJob(ldiImageInspector* Tool, int SelectionId) {
	if (Tool->calibJobSelectedSampleId == SelectionId) {
		return;
	}

	Tool->calibJobSelectedSampleId = SelectionId;

	if (SelectionId == -1) {
		return;
	}

	std::cout << "Sel " << Tool->calibJobSelectedSampleId << "\n";

	FILE* file;
	fopen_s(&file, Tool->calibJob.fileSamples[Tool->calibJobSelectedSampleId].c_str(), "rb");

	int width;
	int height;
	fread(&width, 4, 1, file);
	fread(&height, 4, 1, file);
	fread(&Tool->calibJobImage, width * height, 1, file);
	fclose(file);

	// Copy image to fullpixel input.
	memcpy(Tool->camPixelsFinal, Tool->calibJobImage, width * height);

	_imageInspectorCopyFrameDataToTexture(Tool, Tool->calibJobImage);
}

void _imageInspectorProcessCalibJob(ldiImageInspector* Tool) {
	if (Tool->calibJob.fileSamples.size() == 0) {
		std::cout << "No calibration job samples to process\n";
		return;
	}
}

void imageInspectorShowUi(ldiImageInspector* Tool) {
	if (Tool->imageMode == IIM_LIVE_CAMERA) {
		if (Tool->camNewFrameId != Tool->camLastNewFrameId) {
			Tool->camLastNewFrameId = Tool->camNewFrameId;

			_imageInspectorCopyFrameDataToTexture(Tool, Tool->camPixelsFinal);

			if (Tool->camImageProcess) {
				ldiImage camImg = {};
				camImg.data = Tool->camPixelsFinal;
				camImg.width = CAM_IMG_WIDTH;
				camImg.height = CAM_IMG_HEIGHT;

				findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
			}
		}
	}

	ImGui::Begin("Image inspector controls");

	static float imgScale = 1.0f;
	static vec2 imgOffset(0.0f, 0.0f);

	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Scale", &imgScale, 0.1f, 10.0f);
		//ImGui::SliderFloat("Gain R", &Tool->camImageGainR, 0.0f, 2.0f);
		//ImGui::SliderFloat("Gain G", &Tool->camImageGainG, 0.0f, 2.0f);
		//ImGui::SliderFloat("Gain B", &Tool->camImageGainB, 0.0f, 2.0f);
		ImGui::Text("Cursor position: %.2f, %.2f", Tool->camImageCursorPos.x, Tool->camImageCursorPos.y);
		ImGui::Text("Cursor value: %d", Tool->camImagePixelValue);
	}

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool newSettings = false;

		if (ImGui::SliderInt("Shutter speed", &Tool->camImageShutterSpeed, 1, 30000)) {
			//std::cout << "Set shutter: " << appContext->camImageShutterSpeed << "\n";
			newSettings = true;
		}

		if (ImGui::SliderInt("Analog gain", &Tool->camImageAnalogGain, 0, 100)) {
			//std::cout << "Set shutter: " << appContext->camImageAnalogGain << "\n";
			newSettings = true;
		}

		ImGui::SliderFloat("Filter factor", &Tool->camImageFilterFactor, 0.0f, 1.0f);

		if (newSettings) {
			ldiProtocolSettings settings;
			settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
			settings.header.opcode = 0;
			settings.shutterSpeed = Tool->camImageShutterSpeed;
			settings.analogGain = Tool->camImageAnalogGain;

			networkSend(&Tool->appContext->server, (uint8_t*)&settings, sizeof(ldiProtocolSettings));
		}

		if (ImGui::Button("Start continuous mode")) {
			Tool->imageMode = IIM_LIVE_CAMERA;

			ldiProtocolMode settings;
			settings.header.packetSize = sizeof(ldiProtocolMode) - 4;
			settings.header.opcode = 1;
			settings.mode = 1;

			networkSend(&Tool->appContext->server, (uint8_t*)&settings, sizeof(ldiProtocolMode));
		}

		if (ImGui::Button("Stop continuous mode")) {
			Tool->imageMode = IIM_LIVE_CAMERA;

			ldiProtocolMode settings;
			settings.header.packetSize = sizeof(ldiProtocolMode) - 4;
			settings.header.opcode = 1;
			settings.mode = 0;

			networkSend(&Tool->appContext->server, (uint8_t*)&settings, sizeof(ldiProtocolMode));
		}

		if (ImGui::Button("Get single image")) {
			Tool->imageMode = IIM_LIVE_CAMERA;

			ldiProtocolMode settings;
			settings.header.packetSize = sizeof(ldiProtocolMode) - 4;
			settings.header.opcode = 1;
			settings.mode = 2;

			networkSend(&Tool->appContext->server, (uint8_t*)&settings, sizeof(ldiProtocolMode));
		}
	}

	if (ImGui::CollapsingHeader("Machine vision", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Process", &Tool->camImageProcess);

		if (ImGui::Button("Find Charuco")) {
			ldiImage camImg = {};
			camImg.data = Tool->camPixelsFinal;
			camImg.width = CAM_IMG_WIDTH;
			camImg.height = CAM_IMG_HEIGHT;

			findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
		}
	}

	if (ImGui::CollapsingHeader("Platform calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Load job")) {
			Tool->imageMode = IIM_CALIBRATION_JOB;
			Tool->calibJob.fileSamples.clear();
			_imageInspectorSelectCalibJob(Tool, -1);

			std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/calib/");

			for (int i = 0; i < filePaths.size(); ++i) {
				std::cout << "file " << i << ": " << filePaths[i] << "\n";

				if (endsWith(filePaths[i], ".dat")) {
					Tool->calibJob.fileSamples.push_back(filePaths[i]);
				}
			}
		}

		if (ImGui::Button("Process job")) {
			_imageInspectorProcessCalibJob(Tool);
		}

		ImGui::Text("Samples");
		//if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, -FLT_MIN))) {
			for (int n = 0; n < Tool->calibJob.fileSamples.size(); ++n) {
				bool isSelected = (Tool->calibJobSelectedSampleId == n);

				if (ImGui::Selectable(Tool->calibJob.fileSamples[n].c_str(), isSelected)) {
					// NOTE: Always switch back to calibration job mode if clicking on a job sample.
					Tool->imageMode == IIM_CALIBRATION_JOB;

					_imageInspectorSelectCalibJob(Tool, n);
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	//ImGui::Checkbox("Grid", &camInspector->showGrid);
	//if (ImGui::Button("Run sample test")) {
		//camInspectorRunTest(camInspector);
	//}
	ImGui::End();

	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// This will catch our interactions.
		ImGui::InvisibleButton("__imgInspecViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

		// Convert canvas pos to world pos.
		vec2 worldPos;
		worldPos.x = mouseCanvasPos.x;
		worldPos.y = mouseCanvasPos.y;
		worldPos *= (1.0 / imgScale);
		worldPos -= imgOffset;

		if (isHovered) {
			float wheel = ImGui::GetIO().MouseWheel;

			if (wheel) {
				imgScale += wheel * 0.2f * imgScale;

				vec2 newWorldPos;
				newWorldPos.x = mouseCanvasPos.x;
				newWorldPos.y = mouseCanvasPos.y;
				newWorldPos *= (1.0 / imgScale);
				newWorldPos -= imgOffset;

				vec2 deltaWorldPos = newWorldPos - worldPos;

				imgOffset.x += deltaWorldPos.x;
				imgOffset.y += deltaWorldPos.y;
			}
		}

		if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= (1.0 / imgScale);

			imgOffset += vec2(mouseDelta.x, mouseDelta.y);
		}

		//ImGui::BeginChild("ImageChild", ImVec2(530, 0));
			//ImGui::Text("Image");
			//ID3D11ShaderResourceView*
			//ImTextureID my_tex_id = io.Fonts->TexID;
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(0.5f, 0.5f, 0.5f, 0.0f); // 50% opaque white

		//ImGui::Image(shaderResourceViewTest, ImVec2(512, 512), uv_min, uv_max, tint_col, border_col);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 imgMin;
		imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
		imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

		ImVec2 imgMax;
		imgMax.x = imgMin.x + CAM_IMG_WIDTH * imgScale;
		imgMax.y = imgMin.y + CAM_IMG_HEIGHT * imgScale;

		draw_list->AddCallback(_imageInspectorSetStateCallback, Tool);
		draw_list->AddImage(Tool->camResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

		//----------------------------------------------------------------------------------------------------
		// Draw charuco results.
		//----------------------------------------------------------------------------------------------------
		std::vector<vec2> markerCentroids;

		const ldiCharucoResults* charucos = &Tool->camImageCharucoResults;

		for (int i = 0; i < charucos->markers.size(); ++i) {
			/*ImVec2 offset = pos;
			offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
			offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;*/

			ImVec2 points[5];

			vec2 markerCentroid(0.0f, 0.0f);

			for (int j = 0; j < 4; ++j) {
				vec2 o = charucos->markers[i].corners[j];

				points[j] = pos;
				points[j].x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
				points[j].y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

				markerCentroid.x += points[j].x;
				markerCentroid.y += points[j].y;
			}
			points[4] = points[0];

			markerCentroid /= 4.0f;
			markerCentroids.push_back(markerCentroid);

			draw_list->AddPolyline(points, 5, ImColor(0, 0, 255), 0, 1.0f);
			//draw_list->AddCircle(offset, 4.0f, ImColor(0, 0, 255));
		}

		//std::cout << appContext->camImageMarkerIds.size() << "\n";

		for (int i = 0; i < charucos->markers.size(); ++i) {
			char strBuff[32];
			sprintf_s(strBuff, sizeof(strBuff), "%d", charucos->markers[i].id);
			draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;

				// NOTE: Half pixel offset required.
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
				offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

				draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
			}
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;
				int cornerId = charucos->boards[b].corners[i].id;

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale + 5;
				offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale - 15;

				char strBuf[256];
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x + 0.5f, o.y + 0.5f);

				draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw cursor.
		//----------------------------------------------------------------------------------------------------
		if (isHovered) {
			vec2 pixelPos;
			pixelPos.x = (int)worldPos.x;
			pixelPos.y = (int)worldPos.y;

			Tool->camImageCursorPos = worldPos;

			if (pixelPos.x >= 0 && pixelPos.x < CAM_IMG_WIDTH) {
				if (pixelPos.y >= 0 && pixelPos.y < CAM_IMG_HEIGHT) {
					Tool->camImagePixelValue = Tool->camPixelsFinal[(int)pixelPos.y * CAM_IMG_WIDTH + (int)pixelPos.x];
				}
			}

			ImVec2 rMin;
			rMin.x = screenStartPos.x + (imgOffset.x + pixelPos.x) * imgScale;
			rMin.y = screenStartPos.y + (imgOffset.y + pixelPos.y) * imgScale;

			ImVec2 rMax = rMin;
			rMax.x += imgScale;
			rMax.y += imgScale;

			draw_list->AddRect(rMin, rMax, ImColor(255, 0, 255));
		}

		// Viewport overlay.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_imageInspectorOverlay", ImVec2(200, 70), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			//ImGui::Text("%.3f %.3f - %.3f", tool->surfelViewImgOffset.x, tool->surfelViewImgOffset.y, tool->surfelViewScale);
			ImGui::Text("%.3f %.3f", worldPos.x, worldPos.y);
			//ImGui::Text("Surfels: %d", laserViewSurfelCount);

			ImGui::EndChild();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}