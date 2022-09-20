#pragma once

void _imageInspectorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	// ldiApp* appContext = &_appContext;
	ldiApp* appContext = (ldiApp*)cmd->UserCallbackData;

	//AddDrawCmd ??
	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->camSamplerState);
	appContext->d3dDeviceContext->PSSetShader(appContext->imgCamPixelShader, NULL, 0);
	appContext->d3dDeviceContext->VSSetShader(appContext->imgCamVertexShader, NULL, 0);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->camImagePixelConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCamImagePixelConstants* constantBuffer = (ldiCamImagePixelConstants*)ms.pData;
		constantBuffer->params = vec4(appContext->camImageGainR, appContext->camImageGainG, appContext->camImageGainB, 0);
		appContext->d3dDeviceContext->Unmap(appContext->camImagePixelConstants, 0);
	}

	appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->camImagePixelConstants);
}

void imageInspectorShowUi(ldiApp* appContext) {
	ImGui::Begin("Image inspector controls");
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool newSettings = false;

		if (ImGui::SliderInt("Shutter speed", &appContext->camImageShutterSpeed, 10, 100000)) {
			//std::cout << "Set shutter: " << appContext->camImageShutterSpeed << "\n";
			newSettings = true;
		}

		if (ImGui::SliderInt("Analog gain", &appContext->camImageAnalogGain, 0, 100)) {
			//std::cout << "Set shutter: " << appContext->camImageAnalogGain << "\n";
			newSettings = true;
		}

		if (newSettings) {
			ldiProtocolSettings settings;
			settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
			settings.header.opcode = 0;
			settings.shutterSpeed = appContext->camImageShutterSpeed;
			settings.analogGain = appContext->camImageAnalogGain;

			networkSend(&appContext->server, (uint8_t*)&settings, sizeof(ldiProtocolSettings));
		}
	}

	static float imgScale = 1.0f;
	static vec2 imgOffset(0.0f, 0.0f);

	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Scale", &imgScale, 0.1f, 10.0f);
		ImGui::SliderFloat("Filter factor", &appContext->camImageFilterFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Gain R", &appContext->camImageGainR, 0.0f, 2.0f);
		ImGui::SliderFloat("Gain G", &appContext->camImageGainG, 0.0f, 2.0f);
		ImGui::SliderFloat("Gain B", &appContext->camImageGainB, 0.0f, 2.0f);
		ImGui::Text("Cursor position: %.2f, %.2f", appContext->camImageCursorPos.x, appContext->camImageCursorPos.y);
		ImGui::Text("Cursor value: %d", appContext->camImagePixelValue);
	}

	if (ImGui::CollapsingHeader("Machine vision", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Process", &appContext->camImageProcess);

		if (ImGui::Button("Find Charuco")) {
			findCharuco(_pixelsFinal, appContext);
		}
	}

	//ImGui::Checkbox("Grid", &camInspector->showGrid);
	//if (ImGui::Button("Run sample test")) {
		//camInspectorRunTest(camInspector);
	//}
	ImGui::End();

	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

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
	draw_list->AddCallback(_imageInspectorSetStateCallback, appContext);

	ImVec2 imgMin;
	imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
	imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

	ImVec2 imgMax;
	imgMax.x = imgMin.x + CAM_IMG_WIDTH * imgScale;
	imgMax.y = imgMin.y + CAM_IMG_HEIGHT * imgScale;

	draw_list->AddImage(appContext->camResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));

	draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

	std::vector<vec2> markerCentroids;

	for (int i = 0; i < appContext->camImageMarkerCorners.size() / 4; ++i) {
		/*ImVec2 offset = pos;
		offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
		offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;*/

		ImVec2 points[5];

		vec2 markerCentroid(0.0f, 0.0f);

		for (int j = 0; j < 4; ++j) {
			vec2 o = appContext->camImageMarkerCorners[i * 4 + j];

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

	for (int i = 0; i < appContext->camImageMarkerIds.size(); ++i) {
		char strBuff[32];
		sprintf_s(strBuff, sizeof(strBuff), "%d", appContext->camImageMarkerIds[i]);
		draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
	}

	for (int i = 0; i < appContext->camImageCharucoCorners.size(); ++i) {
		vec2 o = appContext->camImageCharucoCorners[i];

		// TODO: Do we need half pixel offset? Check debug drawing commands.
		ImVec2 offset = pos;
		offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
		offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

		draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
	}

	for (int i = 0; i < appContext->camImageCharucoCorners.size(); ++i) {
		vec2 o = appContext->camImageCharucoCorners[i];
		int cornerId = appContext->camImageCharucoIds[i];

		ImVec2 offset = pos;
		offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale + 5;
		offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale - 15;

		char strBuf[256];
		sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x, o.y);

		draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
	}

	// Cursor.
	if (isHovered) {
		vec2 pixelPos;
		pixelPos.x = (int)worldPos.x;
		pixelPos.y = (int)worldPos.y;

		appContext->camImageCursorPos = worldPos;

		if (pixelPos.x >= 0 && pixelPos.x < CAM_IMG_WIDTH) {
			if (pixelPos.y >= 0 && pixelPos.y < CAM_IMG_HEIGHT) {
				appContext->camImagePixelValue = _pixelsFinal[(int)pixelPos.y * CAM_IMG_WIDTH + (int)pixelPos.x];
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

	ImGui::End();
	ImGui::PopStyleVar();
}