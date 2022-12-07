#pragma once

struct ldiSamplerTester {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.67f, 0.3f, 0.76f, 1.0f };
	bool						showGrid = true;

	vec3						camOffset = { 0, 0, 1 };
	float						camScale = 0.01f;

	ldiRenderModel				samplePointModel;
};

struct ldiSamplePoint {
	vec2 pos;
	float value;
	float scale;
	float radius;
	int nextSampleId;
};

int samplerTesterInit(ldiApp* AppContext, ldiSamplerTester* SamplerTester) {
	SamplerTester->appContext = AppContext;
	SamplerTester->mainViewWidth = 0;
	SamplerTester->mainViewHeight = 0;

	return 0;
}

//float GammaToLinear(float In) {
//	float linearRGBLo = In / 12.92f;
//	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);
//
//	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
//}
//
//float LinearToGamma(float In) {
//	float sRGBLo = In * 12.92f;
//	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
//	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
//}

// Get intensity value from source texture at x,y, 0.0 to 1.0.
float GetSourceValue(float* Data, int Width, int Height, float X, float Y) {
	int pX = (int)(X * Width);
	int pY = (int)(Y * Height);
	int idx = pY * Width + pX;

	return Data[idx];
}

inline int clamp(int Value, int Min, int Max) {
	return min(max(Value, Min), Max);
}

inline float clampf(float Value, float Min, float Max) {
	return min(max(Value, Min), Max);
}

void samplerTesterInitPointModel(ldiSamplerTester* SamplerTester, ldiRenderModel* Model, std::vector<ldiSamplePoint>* SamplePoints) {
	ldiApp* appContext = SamplerTester->appContext;

	if (Model->vertexBuffer) {
		Model->vertexBuffer->Release();
		Model->vertexBuffer = NULL;
	}

	if (Model->indexBuffer) {
		Model->indexBuffer->Release();
		Model->indexBuffer = NULL;
	}

	// Create vert buffer.
	int sampleCount = SamplePoints->size();
	int quadCount = sampleCount;
	int triCount = quadCount * 2;
	int vertCount = quadCount * 4;

	ldiBasicVertex* verts = new ldiBasicVertex[vertCount];

	vec3 basicColor(0, 0, 0);

	for (int i = 0; i < quadCount; ++i) {
		ldiSamplePoint* sample = &(*SamplePoints)[i];

		float hS = (sample->scale * (1.0 - sample->value));

		verts[i * 4 + 0].position = vec3(-hS + sample->pos.x, -hS + sample->pos.y, 0);
		verts[i * 4 + 0].color = basicColor;
		verts[i * 4 + 0].uv = vec2(0, 0);

		verts[i * 4 + 1].position = vec3(hS + sample->pos.x, -hS + sample->pos.y, 0);
		verts[i * 4 + 1].color = basicColor;
		verts[i * 4 + 1].uv = vec2(1, 0);

		verts[i * 4 + 2].position = vec3(hS + sample->pos.x, hS + sample->pos.y, 0);
		verts[i * 4 + 2].color = basicColor;
		verts[i * 4 + 2].uv = vec2(1, 1);

		verts[i * 4 + 3].position = vec3(-hS + sample->pos.x, hS + sample->pos.y, 0);
		verts[i * 4 + 3].color = basicColor;
		verts[i * 4 + 3].uv = vec2(0, 1);
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiBasicVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	appContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &Model->vertexBuffer);

	delete[] verts;

	// Create index buffer.
	int indexCount = triCount * 3;

	uint32_t* indices = new uint32_t[indexCount];

	for (int i = 0; i < quadCount; ++i) {
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;

		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 2;
		indices[i * 6 + 5] = i * 4 + 3;
	}

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	appContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &Model->indexBuffer);

	delete[] indices;

	Model->vertCount = vertCount;
	Model->indexCount = indexCount;
}

void samplerTesterRunTest(ldiSamplerTester* SamplerTester) {
	//---------------------------------------------------------------------------------------------------------------
	// Load image.
	//---------------------------------------------------------------------------------------------------------------
	double t0 = _getTime(SamplerTester->appContext);

	int sourceWidth, sourceHeight, sourceChannels;
	//uint8_t* sourcePixels = imageLoadRgba("../../assets/images/imM.png", &sourceWidth, &sourceHeight, &sourceChannels);
	uint8_t* sourcePixels = imageLoadRgba("../../assets/images/dergn2.png", &sourceWidth, &sourceHeight, &sourceChannels);

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Image loaded: " << (t0 * 1000.0) << " ms\n";
	std::cout << "Image stats: " << sourceWidth << ", " << sourceHeight << " " << sourceChannels << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Convert image to greyscale
	//---------------------------------------------------------------------------------------------------------------
	t0 = _getTime(SamplerTester->appContext);
	
	int sourceTotalPixels = sourceWidth * sourceHeight;
	float* sourceIntensity = new float[sourceTotalPixels];

	for (int i = 0; i < sourceTotalPixels; ++i) {
		uint8_t r = sourcePixels[i * 4 + 0];
		//uint8_t g = sourcePixels[i * sourceChannels + 1];
		//uint8_t b = sourcePixels[i * sourceChannels + 2];

		//float lum = r / 255.0f;// (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;
		float lum = 0.0f;
		//float lum = (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;

		//sourceIntensity[i] = GammaToLinear(lum);
		sourceIntensity[i] = lum;
	}

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Greyscale: " << (t0 * 1000.0) << " ms\n";

	//---------------------------------------------------------------------------------------------------------------
	// Poisson disk sampler - generate sample pool.
	//---------------------------------------------------------------------------------------------------------------
	t0 = _getTime(SamplerTester->appContext);

	float samplesPerPixel = 8.0f;
	float sampleScale = ((sourceWidth / 10.0f) / sourceWidth) / samplesPerPixel;
	float singlePixelScale = sampleScale * 8;

	int samplesWidth = (int)(sourceWidth * samplesPerPixel);
	int samplesHeight = (int)(sourceHeight * samplesPerPixel);

	ldiSamplePoint* candidateList = new ldiSamplePoint[samplesWidth * samplesHeight];
	int candidateCount = 0;

	for (int iY = 0; iY < samplesHeight; ++iY) {
		for (int iX = 0; iX < samplesWidth; ++iX) {
			float sX = (float)iX / samplesWidth;
			float sY = (float)iY / samplesHeight;
			float value = GetSourceValue(sourceIntensity, sourceWidth, sourceHeight, sX, sY);

			if (value >= 0.999f) {
				continue;
			}

			ldiSamplePoint* s = &candidateList[candidateCount++];
			s->pos.x = iX * sampleScale;
			s->pos.y = iY * sampleScale;
			s->value = value;

			float radiusMul = 2.4f;

			float radius = 1;

			float minDotSize = 0.2f;
			float minDotInv = 1.0 - minDotSize;

			if (value > minDotInv) {
				s->value = minDotInv;

				float area = 1.0f / (1.0f - (value - minDotInv)) * 3.14f;
				radius = sqrt(area / M_PI);
				radius = pow(radius, 4.0f);
			}

			radius *= radiusMul;

			s->scale = singlePixelScale * 0.5f * 1.5f;
			//s->scale = singlePixelScale * 0.5f * 0.1f;
			s->radius = singlePixelScale * 0.5f * radius;
		}
	}

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Generate sample pool: " << (t0 * 1000.0) << " ms\n";
	std::cout << "Sample count: " << candidateCount << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Create random order for sample selection.
	//---------------------------------------------------------------------------------------------------------------
	t0 = _getTime(SamplerTester->appContext);

	int* orderList = new int[candidateCount];
	for (int i = 0; i < candidateCount; ++i) {
		orderList[i] = i;
	}

	// Randomize pairs
	for (int j = 0; j < candidateCount; ++j) {
		int targetSlot = rand() % candidateCount;

		int tmp = orderList[targetSlot];
		orderList[targetSlot] = orderList[j];
		orderList[j] = tmp;
	}

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Generate random sample order: " << (t0 * 1000.0) << " ms\n";

	//---------------------------------------------------------------------------------------------------------------
	// Build spatial table.
	//---------------------------------------------------------------------------------------------------------------
	float gridCellSize = 5 * singlePixelScale;
	float gridPotentialWidth = sampleScale * samplesWidth;
	float gridPotentialHeight = sampleScale * samplesHeight;
	int gridCellsX = (int)ceil(gridPotentialWidth / gridCellSize);
	int gridCellsY = (int)ceil(gridPotentialHeight / gridCellSize);
	int gridTotalCells = gridCellsX * gridCellsY;

	std::cout << "Grid size: " << gridCellsX << ", " << gridCellsY << "(" << gridCellSize << ") Total: " << gridTotalCells << "\n";

	int* gridCells = new int[gridCellsX * gridCellsY];

	for (int i = 0; i < gridCellsX * gridCellsY; ++i) {
		gridCells[i] = -1;
	}

	//---------------------------------------------------------------------------------------------------------------
	// Add samples from pool.
	//---------------------------------------------------------------------------------------------------------------
	t0 = _getTime(SamplerTester->appContext);

	//int* pointList = new int[candidateCount];
	//int pointCount = 0;
	std::vector<ldiSamplePoint> samplePoints;
	
	for (int i = 0; i < candidateCount; ++i) {
		int candidateId = orderList[i];

		ldiSamplePoint* candidate = &candidateList[candidateId];
		bool found = false;

		float halfGcs = candidate->radius;

		int gSx = (int)((candidate->pos.x - halfGcs) / gridCellSize);
		int gEx = (int)((candidate->pos.x + halfGcs) / gridCellSize);
		int gSy = (int)((candidate->pos.y - halfGcs) / gridCellSize);
		int gEy = (int)((candidate->pos.y + halfGcs) / gridCellSize);

		gSx = clamp(gSx, 0, gridCellsX - 1);
		gEx = clamp(gEx, 0, gridCellsX - 1);
		gSy = clamp(gSy, 0, gridCellsY - 1);
		gEy = clamp(gEy, 0, gridCellsY - 1);

		for (int iY = gSy; iY <= gEy; ++iY) {
			for (int iX = gSx; iX <= gEx; ++iX) {

				int targetId = gridCells[iY * gridCellsX + iX];

				while (targetId != -1) {
					ldiSamplePoint* target = &candidateList[targetId];

					float dX = candidate->pos.x - target->pos.x;
					float dY = candidate->pos.y - target->pos.y;
					float sqrDist = dX * dX + dY * dY;

					if (sqrDist <= (candidate->radius * candidate->radius)) {
						found = true;
						break;
					}

					targetId = target->nextSampleId;
				}

				if (found == true) {
					break;
				}
			}

			if (found == true) {
				break;
			}
		}

		if (!found) {
			//pointList[pointCount++] = candidateId;
			samplePoints.push_back(*candidate);

			int gX = (int)((candidate->pos.x) / gridCellSize);
			int gY = (int)((candidate->pos.y) / gridCellSize);

			int tempId = gridCells[gY * gridCellsX + gX];
			candidate->nextSampleId = tempId;
			gridCells[gY * gridCellsX + gX] = candidateId;
		}
	}

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Generate poisson samples: " << (t0 * 1000.0) << " ms\n";
	std::cout << "Poisson sample count: " << samplePoints.size() << "\n";

	samplerTesterInitPointModel(SamplerTester, &SamplerTester->samplePointModel, &samplePoints);
}

void samplerTesterRender(ldiSamplerTester* SamplerTester, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = SamplerTester->appContext;

	if (Width != SamplerTester->mainViewWidth || Height != SamplerTester->mainViewHeight) {
		SamplerTester->mainViewWidth = Width;
		SamplerTester->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &SamplerTester->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);

	// Origin.
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	pushDebugQuad(appContext, vec3(0, 0, -1), vec3(50, 69.6, 0), vec3(1, 1, 1));

	if (SamplerTester->showGrid) {
		int gridCountX = 100;
		int gridCountY = 140;
		float gridCellWidth = 0.5f;
		vec3 gridColor = SamplerTester->gridColor;
		//vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, gridCellWidth * gridCount, 0) * 0.5f;
		vec3 gridHalfOffset(0, 0, 0);
		for (int i = 0; i < gridCountX + 1; ++i) {
			pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, gridCountY * gridCellWidth, 0) - gridHalfOffset, gridColor);
		}

		for (int i = 0; i < gridCountY + 1; ++i) {
			pushDebugLine(appContext, vec3(0, i * gridCellWidth, 0) - gridHalfOffset, vec3(gridCountX * gridCellWidth, i * gridCellWidth, 0) - gridHalfOffset, gridColor);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	vec3 camPos = SamplerTester->camOffset;
	mat4 camViewMat = glm::translate(mat4(1.0f), -camPos);

	float viewWidth = SamplerTester->mainViewWidth * SamplerTester->camScale;
	float viewHeight = SamplerTester->mainViewHeight * SamplerTester->camScale;
	mat4 projMat = glm::orthoRH_ZO(0.0f, viewWidth, viewHeight, 0.0f, 0.01f, 100.0f);
	//mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)SamplerTester->mainViewWidth, (float)SamplerTester->mainViewHeight, 0.01f, 100.0f);

	mat4 projViewModelMat = projMat * camViewMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &SamplerTester->renderViewBuffers.mainViewRtView, SamplerTester->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SamplerTester->mainViewWidth;
	viewport.Height = SamplerTester->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(SamplerTester->renderViewBuffers.mainViewRtView, (float*)&SamplerTester->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(SamplerTester->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext);

	//----------------------------------------------------------------------------------------------------
	// Render point samples.
	//----------------------------------------------------------------------------------------------------
	// TODO: Set texture!
	if (SamplerTester->samplePointModel.vertCount > 0) {
		UINT lgStride = sizeof(ldiMeshVertex);
		UINT lgOffset = 0;

		appContext->d3dDeviceContext->IASetInputLayout(appContext->dotInputLayout);
		appContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &SamplerTester->samplePointModel.vertexBuffer, &lgStride, &lgOffset);
		appContext->d3dDeviceContext->IASetIndexBuffer(SamplerTester->samplePointModel.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		appContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		appContext->d3dDeviceContext->VSSetShader(appContext->dotVertexShader, 0, 0);
		appContext->d3dDeviceContext->VSSetConstantBuffers(0, 2, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->PSSetShader(appContext->dotPixelShader, 0, 0);
		appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		appContext->d3dDeviceContext->OMSetBlendState(appContext->alphaBlendState, NULL, 0xffffffff);
		appContext->d3dDeviceContext->RSSetState(appContext->defaultRasterizerState);
		appContext->d3dDeviceContext->OMSetDepthStencilState(appContext->noDepthState, 0);

		appContext->d3dDeviceContext->PSSetShaderResources(0, 1, &appContext->dotShaderResourceView);
		appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->dotSamplerState);

		appContext->d3dDeviceContext->DrawIndexed(SamplerTester->samplePointModel.indexCount, 0, 0);
	}
}

void samplerTesterShowUi(ldiSamplerTester* tool) {
	ImGui::Begin("Sampler tester controls");
	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Background", (float*)&tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid color", (float*)&tool->gridColor);
		ImGui::Checkbox("Grid", &tool->showGrid);
	}

	if (ImGui::CollapsingHeader("Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Run sample test")) {
			samplerTesterRunTest(tool);
		}
	}
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Sampler tester", 0, ImGuiWindowFlags_NoCollapse);

	ImVec2 viewSize = ImGui::GetContentRegionAvail();
	ImVec2 startPos = ImGui::GetCursorPos();
	ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

	// This will catch our interactions.
	ImGui::InvisibleButton("__sampleTestViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool isHovered = ImGui::IsItemHovered(); // Hovered
	const bool isActive = ImGui::IsItemActive();   // Held
	//ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
	//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
	ImVec2 mousePos = ImGui::GetIO().MousePos;
	const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

	// Convert canvas pos to world pos.
	vec2 worldPos;
	worldPos.x = mouseCanvasPos.x;
	worldPos.y = mouseCanvasPos.y;
	worldPos *= tool->camScale;
	worldPos += vec2(tool->camOffset);
	//std::cout << worldPos.x << ", " << worldPos.y << "\n";

	/*{
		vec3 camMove(0, 0, 0);
		ldiCamera* camera = &modelInspector->camera;
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
			float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime;
			camera->position += camMove * cameraSpeed;
		}
	}

	if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= 0.15f;
		modelInspector->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
	}*/

	if (isHovered) {
		float wheel = ImGui::GetIO().MouseWheel;

		if (wheel) {
			tool->camScale -= wheel * 0.1f * tool->camScale;

			vec2 newWorldPos;
			newWorldPos.x = mouseCanvasPos.x;
			newWorldPos.y = mouseCanvasPos.y;
			newWorldPos *= tool->camScale;
			newWorldPos += vec2(tool->camOffset);

			vec2 deltaWorldPos = newWorldPos - worldPos;

			tool->camOffset.x -= deltaWorldPos.x;
			tool->camOffset.y -= deltaWorldPos.y;
		}
	}

	if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= tool->camScale;

		tool->camOffset -= vec3(mouseDelta.x, mouseDelta.y, 0);
	}

	ImGui::SetCursorPos(startPos);
	std::vector<ldiTextInfo> textBuffer;
	samplerTesterRender(tool, viewSize.x, viewSize.y, &textBuffer);
	ImGui::Image(tool->renderViewBuffers.mainViewResourceView, viewSize);

	//{
	//	// Viewport overlay widgets.
	//	ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
	//	ImGui::BeginChild("_simpleOverlayMainView", ImVec2(300, 0), false, ImGuiWindowFlags_NoScrollbar);

	//	ImGui::Text("Time: (%f)", ImGui::GetTime());
	//	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	//	ImGui::Separator();
	//	ImGui::Text("Primary model");
	//	ImGui::Checkbox("Shaded", &modelInspector->primaryModelShowShaded);
	//	ImGui::Checkbox("Wireframe", &modelInspector->primaryModelShowWireframe);

	//	ImGui::Separator();
	//	ImGui::Text("Point cloud");
	//	ImGui::Checkbox("Show", &modelInspector->showPointCloud);
	//	ImGui::SliderFloat("World size", &modelInspector->pointWorldSize, 0.0f, 1.0f);
	//	ImGui::SliderFloat("Screen size", &modelInspector->pointScreenSize, 0.0f, 32.0f);
	//	ImGui::SliderFloat("Screen blend", &modelInspector->pointScreenSpaceBlend, 0.0f, 1.0f);

	//	ImGui::Separator();
	//	ImGui::Text("Viewport");
	//	ImGui::ColorEdit3("Background", (float*)&modelInspector->viewBackgroundColor);
	//	ImGui::ColorEdit3("Grid", (float*)&modelInspector->gridColor);

	//	ImGui::Separator();
	//	ImGui::Text("Processing");
	//	if (ImGui::Button("Process model")) {
	//		modelInspectorVoxelize(modelInspector);
	//		modelInspectorCreateQuadMesh(modelInspector);
	//	}

	//	if (ImGui::Button("Voxelize")) {
	//		modelInspectorVoxelize(modelInspector);
	//	}

	//	if (ImGui::Button("Quadralize")) {
	//		modelInspectorCreateQuadMesh(modelInspector);
	//	}

	//	ImGui::EndChild();
	//}

	ImGui::End();
	ImGui::PopStyleVar();
}