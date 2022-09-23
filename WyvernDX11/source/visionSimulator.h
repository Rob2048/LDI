#pragma once

struct ldiTransform {
	vec3 localPos;
	vec3 localRot;
	mat4 local;
	mat4 world;
};

struct ldiHorse {
	ldiTransform origin;
	ldiTransform axisX;
	ldiTransform axisY;
	ldiTransform axisZ;
	ldiTransform axisA;
	ldiTransform axisB;
	float x;
	float y;
	float z;
	float a;
	float b;
};

struct ldiVisionSimulator {
	ldiApp*						appContext;

	// Need this??
	ldiRenderViewBuffers		renderViewBuffers;
	ldiCamera					camera;
	ID3D11Texture2D*			renderViewCopyStaging;
	uint8_t*					renderViewCopy;

	ldiRenderViewBuffers		mainViewBuffers;
	int							mainViewWidth;
	int							mainViewHeight;
	ldiCamera					mainCamera;
	float						mainCameraSpeed = 1.0f;
	vec4						mainBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	vec2						imageCursorPos;
	uint8_t						imagePixelValue;

	int							imageWidth;
	int							imageHeight;
	float						imageScale;
	vec2						imageOffset;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };

	vec3						targetPosition;
	vec3						targetRotation;
	vec3						targetScale;

	ID3D11Texture2D*			targetTexFace[6];
	ID3D11ShaderResourceView*	targetShaderViewFace[6];
	ID3D11SamplerState*			targetTexSampler;

	ldiRenderModel				targetModel;
	mat4						targetFaceMat[6];

	ldiCharucoResults			charucoResults = {};

	ldiHorse					horse;
};

void transformUpdateLocal(ldiTransform* Transform) {
	Transform->local = glm::rotate(mat4(1.0f), glm::radians(Transform->localRot.x), vec3(1, 0, 0));
	Transform->local = glm::rotate(Transform->local, glm::radians(Transform->localRot.y), vec3(0, 1, 0));
	Transform->local = glm::rotate(Transform->local, glm::radians(Transform->localRot.z), vec3(0, 0, 1));
	// TODO: Check if can put local instead of ident mat.
	Transform->local = glm::translate(mat4(1.0f), Transform->localPos) * Transform->local;
}

void transformUpdateWorld(ldiTransform* Transform, ldiTransform* Parent) {
	Transform->world = Transform->local;

	if (Parent) {
		Transform->world = Parent->world * Transform->world;
	}
}

void transformInit(ldiTransform* Transform, vec3 Pos, vec3 Rot) {
	Transform->localPos = Pos;
	Transform->localRot = Rot;
	transformUpdateLocal(Transform);
}

vec3 transformGetWorldPoint(ldiTransform* Transform, vec3 LocalPos) {
	return Transform->world * vec4(LocalPos.x, LocalPos.y, LocalPos.z, 1.0f);
}

void horseUpdateMats(ldiHorse* Horse) {
	transformUpdateWorld(&Horse->origin, 0);
	transformUpdateWorld(&Horse->axisX, &Horse->origin);
	transformUpdateWorld(&Horse->axisY, &Horse->axisX);
	transformUpdateWorld(&Horse->axisZ, &Horse->axisY);
	transformUpdateWorld(&Horse->axisA, &Horse->axisZ);
	transformUpdateWorld(&Horse->axisB, &Horse->axisA);
}

void horseInit(ldiHorse* Horse) {
	transformInit(&Horse->origin, vec3(0, 0, 0), vec3(-90, 0, 0));
	transformInit(&Horse->axisX, vec3(0, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisY, vec3(0, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisZ, vec3(0, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisA, vec3(0, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisB, vec3(0, 0, 0), vec3(0, 0, 0));

	horseUpdateMats(Horse);
}

void horseUpdate(ldiHorse* Horse) {
	Horse->axisX.localPos.x = Horse->x;
	Horse->axisY.localPos.y = Horse->y - 10.0f;
	Horse->axisZ.localPos.z = Horse->z;
	Horse->axisA.localRot.x = Horse->a;
	Horse->axisB.localRot.z = Horse->b;

	transformUpdateLocal(&Horse->axisX);
	transformUpdateLocal(&Horse->axisY);
	transformUpdateLocal(&Horse->axisZ);
	transformUpdateLocal(&Horse->axisA);
	transformUpdateLocal(&Horse->axisB);

	horseUpdateMats(Horse);
}

void _visionSimulatorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiApp* appContext = (ldiApp*)cmd->UserCallbackData;

	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
	/*appContext->d3dDeviceContext->PSSetShader(appContext->imgCamPixelShader, NULL, 0);
	appContext->d3dDeviceContext->VSSetShader(appContext->imgCamVertexShader, NULL, 0);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->camImagePixelConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCamImagePixelConstants* constantBuffer = (ldiCamImagePixelConstants*)ms.pData;
		constantBuffer->params = vec4(appContext->camImageGainR, appContext->camImageGainG, appContext->camImageGainB, 0);
		appContext->d3dDeviceContext->Unmap(appContext->camImagePixelConstants, 0);
	}

	appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->camImagePixelConstants);*/
}

int _visionSimulatorLoadCubeFaceTexture(ldiVisionSimulator* Tool, uint8_t* ImageData, int ImageWidth, int ImageHeight, int FaceId) {
	D3D11_SUBRESOURCE_DATA texData = {};
	texData.pSysMem = ImageData;
	texData.SysMemPitch = ImageWidth * 4;

	D3D11_TEXTURE2D_DESC tex2dDesc = {};
	tex2dDesc.Width = ImageWidth;
	tex2dDesc.Height = ImageHeight;
	tex2dDesc.MipLevels = 0;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	if (Tool->appContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->targetTexFace[FaceId]) != S_OK) {
		std::cout << "CreateTexture2D failed\n";
		return 1;
	}

	Tool->appContext->d3dDeviceContext->UpdateSubresource(Tool->targetTexFace[FaceId], 0, NULL, ImageData, ImageWidth * 4, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = tex2dDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MostDetailedMip = 0;
	viewDesc.Texture2D.MipLevels = -1;

	if (Tool->appContext->d3dDevice->CreateShaderResourceView(Tool->targetTexFace[FaceId], &viewDesc, &Tool->targetShaderViewFace[FaceId]) != S_OK) {
		std::cout << "CreateShaderResourceView failed\n";
		return 1;
	}

	Tool->appContext->d3dDeviceContext->GenerateMips(Tool->targetShaderViewFace[FaceId]);

	return 0;
}

void _visionSimulatorCopyRenderToBuffer(ldiVisionSimulator* Tool) {
	Tool->appContext->d3dDeviceContext->CopyResource(Tool->renderViewCopyStaging, Tool->renderViewBuffers.mainViewTexture);
	
	D3D11_MAPPED_SUBRESOURCE ResourceDesc = {};
	Tool->appContext->d3dDeviceContext->Map(Tool->renderViewCopyStaging, 0, D3D11_MAP_READ, 0, &ResourceDesc);

	if (ResourceDesc.pData) {
		int const BytesPerPixel = 4;
		for (int i = 0; i < Tool->imageHeight; ++i) {
			memcpy(Tool->renderViewCopy + Tool->imageWidth * BytesPerPixel * i, (byte*)ResourceDesc.pData + ResourceDesc.RowPitch * i, Tool->imageWidth * BytesPerPixel);
		}
	}

	Tool->appContext->d3dDeviceContext->Unmap(Tool->renderViewCopyStaging, 0);

	// Convert to greyscale.
	for (int i = 0; i < Tool->imageHeight * Tool->imageWidth; ++i) {
		Tool->renderViewCopy[i] = Tool->renderViewCopy[i * 4];
	}

	//imageWrite("Copy test.png", Tool->imageWidth, Tool->imageHeight, 1, Tool->imageWidth, Tool->renderViewCopy);
}

int visionSimulatorInit(ldiApp* AppContext, ldiVisionSimulator* Tool) {
	Tool->appContext = AppContext;

	Tool->camera = {};
	Tool->camera.position = vec3(10, 10, 10);
	Tool->camera.rotation = vec3(-45, 45, 0);

	Tool->imageScale = 1.0f;
	Tool->imageOffset = vec2(0.0f, 0.0f);
	Tool->imageWidth = 1600;
	Tool->imageHeight = 1300;

	Tool->renderViewCopy = new uint8_t[Tool->imageWidth * Tool->imageHeight * 4];

	horseInit(&Tool->horse);

	gfxCreateRenderView(AppContext, &Tool->renderViewBuffers, Tool->imageWidth, Tool->imageHeight);

	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = Tool->imageWidth;
		texDesc.Height = Tool->imageHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.MiscFlags = 0;
		AppContext->d3dDevice->CreateTexture2D(&texDesc, NULL, &Tool->renderViewCopyStaging);
	}

	const int boardSize = 512;
	static uint8_t boardBuffer[boardSize * boardSize * 4];

	// NOTE: Load charucos as textures from cv engine.
	for (int i = 0; i < 6; ++i) {
		cv::Mat markerImage;
		_charucoBoards[i]->draw(cv::Size(boardSize, boardSize), markerImage, 0, 1);

		for (int p = 0; p < boardSize * boardSize; ++p) {
			uint8_t v = markerImage.data[p];

			boardBuffer[p * 4 + 0] = v;
			boardBuffer[p * 4 + 1] = v;
			boardBuffer[p * 4 + 2] = v;
			boardBuffer[p * 4 + 3] = 255;
		}
	
		if (_visionSimulatorLoadCubeFaceTexture(Tool, boardBuffer, boardSize, boardSize, i) != 0) {
			return 1;
		}
	}

	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; //D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &Tool->targetTexSampler);
	}

	// NOTE: Create cube model.
	ldiModel targetModel;

	float cubeSize = 5.0f;
	float chs = cubeSize * 0.5f;

	targetModel.verts.push_back({{-chs, +chs, -chs}, {0, 1, 0}, {0, 1}});
	targetModel.verts.push_back({{+chs, +chs, -chs}, {0, 1, 0}, {1, 1}});
	targetModel.verts.push_back({{+chs, +chs, +chs}, {0, 1, 0}, {1, 0}});
	targetModel.verts.push_back({{-chs, +chs, +chs}, {0, 1, 0}, {0, 0}});
	targetModel.indices.push_back(0);
	targetModel.indices.push_back(1);
	targetModel.indices.push_back(2);
	targetModel.indices.push_back(0);
	targetModel.indices.push_back(2);
	targetModel.indices.push_back(3);

	Tool->targetModel = gfxCreateRenderModel(AppContext, &targetModel);

	// Y+
	Tool->targetFaceMat[0] = mat4(1.0f);
	// X+
	Tool->targetFaceMat[1] = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
	// X-
	Tool->targetFaceMat[2] = glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
	// Z+
	Tool->targetFaceMat[3] = glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
	// Z-
	Tool->targetFaceMat[4] = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
	// Y-
	Tool->targetFaceMat[5] = glm::rotate(mat4(1.0f), glm::radians(180.0f), vec3(1.0f, 0.0f, 0.0f));

	return 0;
}

void _renderTransformOrigin(ldiVisionSimulator* Tool, ldiTransform* Transform, std::string Text, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;
	vec3 root = transformGetWorldPoint(Transform, vec3(0, 0, 0));

	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(1, 0, 0)), vec3(1, 0, 0));
	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(0, 1, 0)), vec3(0, 1, 0));
	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(0, 0, 1)), vec3(0, 0, 1));

	vec3 projPos = projectPoint(&Tool->mainCamera, Tool->mainViewWidth, Tool->mainViewHeight, root);
	if (projPos.z > 0) {
		TextBuffer->push_back({ vec2(projPos.x, projPos.y), Text });
	}
}

void visionSimulatorMainRender(ldiVisionSimulator* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->mainViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	//----------------------------------------------------------------------------------------------------
	// Grid.
	//----------------------------------------------------------------------------------------------------
	int gridCount = 10;
	float gridCellWidth = 1.0f;
	vec3 gridColor = Tool->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->mainCamera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->mainCamera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->mainCamera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)Tool->mainViewWidth, (float)Tool->mainViewHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

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
		_renderTransformOrigin(Tool, &Tool->horse.origin, "Origin", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisX, "X", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisY, "Y", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisZ, "Z", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisA, "A", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisB, "B", TextBuffer);

		// NOTE: camera rotation doesn't really translate to an ldiTransfrom style rotation.
		ldiTransform mvCameraTransform = {};
		transformInit(&mvCameraTransform, Tool->camera.position, Tool->camera.rotation);
		transformUpdateWorld(&mvCameraTransform, 0);
		_renderTransformOrigin(Tool, &mvCameraTransform, "Camera", TextBuffer);
	}

	//----------------------------------------------------------------------------------------------------
	// Render cube model.
	//----------------------------------------------------------------------------------------------------
	for (int i = 0; i < 6; ++i) {
		ldiBasicConstantBuffer constantBuffer;
		constantBuffer.mvp = projMat * camViewMat * Tool->horse.axisB.world * glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(1, 0, 0)) * Tool->targetFaceMat[i];
		constantBuffer.color = vec4(1, 1, 1, 1);

		if (i == 5) {
			constantBuffer.color = vec4(0, 0, 0, 1);
		}

		gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
		gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], Tool->targetTexSampler);
	}

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
}

void visionSimulatorRender(ldiVisionSimulator* Tool) {
	ldiApp* appContext = Tool->appContext;

	beginDebugPrimitives(appContext);
	/*pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(0.5f, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 0.5f, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 0.5f));*/

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)Tool->imageWidth, (float)Tool->imageHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->renderViewBuffers.mainViewRtView, Tool->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->imageWidth;
	viewport.Height = Tool->imageHeight;
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
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext);

	//----------------------------------------------------------------------------------------------------
	// Render cube model.
	//----------------------------------------------------------------------------------------------------
	for (int i = 0; i < 6; ++i) {
		ldiBasicConstantBuffer constantBuffer;
		constantBuffer.mvp = projMat * camViewMat * Tool->horse.axisB.world * glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(1, 0, 0)) * Tool->targetFaceMat[i];
		constantBuffer.color = vec4(1, 1, 1, 1);

		if (i == 5) {
			constantBuffer.color = vec4(0, 0, 0, 1);
		}

		gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
		gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], Tool->targetTexSampler);
	}
}

void visionSimulatorShowUi(ldiVisionSimulator* Tool) {
	//----------------------------------------------------------------------------------------------------
	// Vision simulator controls.
	//----------------------------------------------------------------------------------------------------
	ImGui::Begin("Vision simulator controls");
	
	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Scale", &Tool->imageScale, 0.1f, 10.0f);
		ImGui::Text("Cursor position: %.2f, %.2f", Tool->imageCursorPos.x, Tool->imageCursorPos.y);
		ImGui::Text("Cursor value: %d", Tool->imagePixelValue);

		ImGui::SliderFloat("Cam X", &Tool->camera.position.x, 0.0f, 0.01f, "%.4f");
	}

	if (ImGui::CollapsingHeader("Machine vision", ImGuiTreeNodeFlags_DefaultOpen)) {
		//ImGui::Checkbox("Process", &appContext->camImageProcess);

		if (ImGui::Button("Find Charuco")) {
			// Get pixels from VRAM.
			_visionSimulatorCopyRenderToBuffer(Tool);
			ldiImage img = {};
			img.data = Tool->renderViewCopy;
			img.width = Tool->imageWidth;
			img.height = Tool->imageHeight;
			findCharuco(img, Tool->appContext, &Tool->charucoResults);
		}
	}

	if (ImGui::CollapsingHeader("Machine", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("X", &Tool->horse.x, 0.0f, 20.0f);
		ImGui::SliderFloat("Y", &Tool->horse.y, 0.0f, 20.0f);
		ImGui::SliderFloat("Z", &Tool->horse.z, 0.0f, 20.0f);
		ImGui::SliderFloat("A", &Tool->horse.a, 0.0f, 360.0f);
		ImGui::SliderFloat("B", &Tool->horse.b, 0.0f, 360.0f);
	}

	ImGui::End();

	//----------------------------------------------------------------------------------------------------
	// Sim updates.
	//----------------------------------------------------------------------------------------------------
	horseUpdate(&Tool->horse);

	//----------------------------------------------------------------------------------------------------
	// Vision simulator image view.
	//----------------------------------------------------------------------------------------------------
	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	ImGui::Begin("Vision simulator", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

	ImVec2 viewSize = ImGui::GetContentRegionAvail();
	ImVec2 startPos = ImGui::GetCursorPos();
	ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

	// This will catch our interactions.
	ImGui::InvisibleButton("__visSimViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool isHovered = ImGui::IsItemHovered(); // Hovered
	const bool isActive = ImGui::IsItemActive();   // Held
	ImVec2 mousePos = ImGui::GetIO().MousePos;
	const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

	// Convert canvas pos to world pos.
	vec2 worldPos;
	worldPos.x = mouseCanvasPos.x;
	worldPos.y = mouseCanvasPos.y;
	worldPos *= (1.0 / Tool->imageScale);
	worldPos -= Tool->imageOffset;

	if (isHovered) {
		float wheel = ImGui::GetIO().MouseWheel;

		if (wheel) {
			Tool->imageScale += wheel * 0.2f * Tool->imageScale;

			vec2 newWorldPos;
			newWorldPos.x = mouseCanvasPos.x;
			newWorldPos.y = mouseCanvasPos.y;
			newWorldPos *= (1.0 / Tool->imageScale);
			newWorldPos -= Tool->imageOffset;

			vec2 deltaWorldPos = newWorldPos - worldPos;

			Tool->imageOffset.x += deltaWorldPos.x;
			Tool->imageOffset.y += deltaWorldPos.y;
		}
	}

	if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= (1.0 / Tool->imageScale);

		Tool->imageOffset += vec2(mouseDelta.x, mouseDelta.y);
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
	ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
	ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
	ImVec4 border_col = ImVec4(0.5f, 0.5f, 0.5f, 0.0f); // 50% opaque white

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImVec2 imgMin;
	imgMin.x = screenStartPos.x + Tool->imageOffset.x * Tool->imageScale;
	imgMin.y = screenStartPos.y + Tool->imageOffset.y * Tool->imageScale;

	ImVec2 imgMax;
	imgMax.x = imgMin.x + Tool->imageWidth * Tool->imageScale;
	imgMax.y = imgMin.y + Tool->imageHeight * Tool->imageScale;

	visionSimulatorRender(Tool);
	draw_list->AddCallback(_visionSimulatorSetStateCallback, Tool->appContext);
	draw_list->AddImage(Tool->renderViewBuffers.mainViewResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
	draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

	//----------------------------------------------------------------------------------------------------
	// Draw charuco results.
	//----------------------------------------------------------------------------------------------------
	std::vector<vec2> markerCentroids;

	const ldiCharucoResults* charucos = &Tool->charucoResults;

	for (int i = 0; i < charucos->markerCorners.size() / 4; ++i) {
		/*ImVec2 offset = pos;
		offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
		offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;*/

		ImVec2 points[5];

		vec2 markerCentroid(0.0f, 0.0f);

		for (int j = 0; j < 4; ++j) {
			vec2 o = charucos->markerCorners[i * 4 + j];

			points[j] = pos;
			points[j].x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
			points[j].y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

			markerCentroid.x += points[j].x;
			markerCentroid.y += points[j].y;
		}
		points[4] = points[0];

		markerCentroid /= 4.0f;
		markerCentroids.push_back(markerCentroid);

		draw_list->AddPolyline(points, 5, ImColor(0, 0, 255, 200), 0, 6.0f);
		//draw_list->AddCircle(offset, 4.0f, ImColor(0, 0, 255));
	}

	//std::cout << appContext->camImageMarkerIds.size() << "\n";

	for (int i = 0; i < charucos->markerIds.size(); ++i) {
		char strBuff[32];
		sprintf_s(strBuff, sizeof(strBuff), "%d", charucos->markerIds[i]);
		draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
	}

	for (int i = 0; i < charucos->charucoCorners.size(); ++i) {
		vec2 o = charucos->charucoCorners[i];

		// TODO: Do we need half pixel offset? Check debug drawing commands.
		ImVec2 offset = pos;
		offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
		offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

		draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0, 200), 0, 3.0f);
	}

	for (int i = 0; i < charucos->charucoCorners.size(); ++i) {
		vec2 o = charucos->charucoCorners[i];
		int cornerId = charucos->charucoIds[i];

		ImVec2 offset = pos;
		offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale + 5;
		offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale - 15;

		char strBuf[256];
		// NOTE: Half pixel offsets added to position values.
		sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x + 0.5f, o.y + 0.5f);

		draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
	}

	//----------------------------------------------------------------------------------------------------
	// Draw cursor.
	//----------------------------------------------------------------------------------------------------
	if (isHovered) {
		vec2 pixelPos;
		pixelPos.x = (int)worldPos.x;
		pixelPos.y = (int)worldPos.y;

		Tool->imageCursorPos = worldPos;

		if (pixelPos.x >= 0 && pixelPos.x < Tool->imageWidth) {
			if (pixelPos.y >= 0 && pixelPos.y < Tool->imageHeight) {
				Tool->imagePixelValue = 69;
				//tool->imagePixelValue = _pixelsFinal[(int)pixelPos.y * tool->imageWidth + (int)pixelPos.x];
			}
		}

		ImVec2 rMin;
		rMin.x = screenStartPos.x + (Tool->imageOffset.x + pixelPos.x) * Tool->imageScale;
		rMin.y = screenStartPos.y + (Tool->imageOffset.y + pixelPos.y) * Tool->imageScale;

		ImVec2 rMax = rMin;
		rMax.x += Tool->imageScale;
		rMax.y += Tool->imageScale;

		draw_list->AddRect(rMin, rMax, ImColor(255, 0, 255));
	}

	ImGui::End();
	ImGui::PopStyleVar();

	//----------------------------------------------------------------------------------------------------
	// Vision simulator 3D view.
	//----------------------------------------------------------------------------------------------------
	{
		ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Visual simulator 3D view", 0, ImGuiWindowFlags_NoCollapse);

		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// NOTE: ImDrawList API uses screen coordinates!
		/*ImVec2 tempStartPos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(tempStartPos, ImVec2(tempStartPos.x + 500, tempStartPos.y + 500), IM_COL32(200, 200, 200, 255));*/

		// This will catch our interactions.
		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held
		//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
		//const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

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
		visionSimulatorMainRender(Tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(Tool->mainViewBuffers.mainViewResourceView, viewSize);

		// NOTE: ImDrawList API uses screen coordinates!
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int i = 0; i < textBuffer.size(); ++i) {
			ldiTextInfo* info = &textBuffer[i];
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
		}

		// Viewport overlay widgets.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 25), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::EndChild();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}