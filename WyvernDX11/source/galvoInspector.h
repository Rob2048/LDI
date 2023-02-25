#pragma once

struct ldiGalvoInspector {
	ldiApp*						appContext;

	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	float						cameraSpeed = 0.5f;
};

int galvoInspectorInit(ldiApp* AppContext, ldiGalvoInspector* Tool) {
	Tool->appContext = AppContext; 
	Tool->mainViewWidth = 0;
	Tool->mainViewHeight = 0;

	Tool->camera = {};
	Tool->camera.position = vec3(0, 0, 10);
	Tool->camera.rotation = vec3(0, 0, 0);
	Tool->camera.fov = 60.0f;

	return 0;
}

void galvoInspectorRender(ldiGalvoInspector* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	updateCamera3dBasicFps(&Tool->camera, (float)Tool->mainViewWidth, (float)Tool->mainViewHeight);

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
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->renderViewBuffers.mainViewRtView, Tool->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->mainViewWidth;
	viewport.Height = Tool->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->renderViewBuffers.mainViewRtView, (float*)&Tool->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = Tool->camera.projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
}

void galvoInspectorShowUi(ldiGalvoInspector* Tool) {
	ldiApp* appContext = Tool->appContext;
	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Galvo inspector controls", 0, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Toolhead position");

	ImGui::Text("Laser source position");

	ImGui::Text("Galvo mirrors");

	ImGui::End();
	
	//----------------------------------------------------------------------------------------------------
	// Platform 3D view.
	//----------------------------------------------------------------------------------------------------
	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Galvo inspector 3D view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held

		{
			vec3 camMove(0, 0, 0);
			ldiCamera* camera = &Tool->camera;
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
				float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime * Tool->cameraSpeed;
				camera->position += camMove * cameraSpeed;
			}
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.15f;
			Tool->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.025f;

			ldiCamera* camera = &Tool->camera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
			camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
		}

		ImGui::SetCursorPos(startPos);
		std::vector<ldiTextInfo> textBuffer;
		galvoInspectorRender(Tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(Tool->renderViewBuffers.mainViewResourceView, viewSize);

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