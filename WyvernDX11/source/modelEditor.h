#pragma once

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

struct ldiModelEditor {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;
	float						cameraSpeed = 0.5f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	ID3D11VertexShader*			raymarchVertexShader;
	ID3D11PixelShader*			raymarchPixelShader;
	ID3D11InputLayout*			raymarchInputLayout;

	ldiRenderModel				fullScreenQuad;

	ID3D11Texture3D*			sdfVolumeTexture;
	ID3D11ShaderResourceView*	sdfVolumeResourceView;
	ID3D11UnorderedAccessView*	sdfVolumeUav;
	ID3D11SamplerState*			sdfVolumeSamplerState;
	ID3D11ComputeShader*		sdfVolumeCompute;
};

int modelEditorInit(ldiApp* AppContext, ldiModelEditor* Tool) {
	Tool->appContext = AppContext;
	Tool->mainViewWidth = 0;
	Tool->mainViewHeight = 0;
	
	Tool->camera = {};
	Tool->camera.position = vec3(6, 6, 6);
	Tool->camera.rotation = vec3(-45, 30, 0);

	return 0;
}

int modelEditorLoad(ldiApp* AppContext, ldiModelEditor* Tool) {

	std::cout << "Start shader compile\n";

	//----------------------------------------------------------------------------------------------------
	// Raymarch shader.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC basicLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/raymarch.hlsl", "mainVs", &Tool->raymarchVertexShader, basicLayout, 3, &Tool->raymarchInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/raymarch.hlsl", "mainPs", &Tool->raymarchPixelShader)) {
		return 1;
	}

	std::cout << "Completed shader compile\n";

	//----------------------------------------------------------------------------------------------------
	// SDF generate compute shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/generateSdf.hlsl", "mainCs", &Tool->sdfVolumeCompute)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Fullscreen quad for raymarcher.
	//----------------------------------------------------------------------------------------------------
	{
		std::vector<ldiBasicVertex> verts;
		std::vector<uint32_t> indices;

		verts.push_back({ {-1, -1, 1}, {0, 0, 0}, {0, 0} });
		verts.push_back({ {1, -1, 1}, {0, 0, 0}, {1, 0} });
		verts.push_back({ {1, 1, 1}, {0, 0, 0}, {1, 1} });
		verts.push_back({ {-1, 1, 1}, {0, 0, 0}, {0, 1} });

		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(0);
		indices.push_back(3);
		indices.push_back(2);
		indices.push_back(0);

		Tool->fullScreenQuad = gfxCreateRenderModelBasic(Tool->appContext, &verts, &indices);
	}

	//----------------------------------------------------------------------------------------------------
	// SDF volume.
	//----------------------------------------------------------------------------------------------------
	{
		int sdfWidth = 128;
		int sdfHeight = 128;
		int sdfDepth = 128;

		D3D11_TEXTURE3D_DESC desc = {};
		desc.Width = sdfWidth;
		desc.Height = sdfHeight;
		desc.Depth = sdfDepth;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		float* sdfVolume = new float[sdfWidth * sdfHeight * sdfDepth];

		vec3 sphereOrigin(sdfWidth / 2.0f, sdfHeight / 2.0f, sdfDepth / 2.0f);
		float sphereRadius = 32.0f;

		for (int iZ = 0; iZ < sdfDepth; ++iZ) {
			for (int iY = 0; iY < sdfHeight; ++iY) {
				for (int iX = 0; iX < sdfWidth; ++iX) {
					int idx = iX + iY * sdfWidth + iZ * sdfWidth * sdfHeight;

					vec3 pos(iX + 0.5f, iY + 0.5f, iZ + 0.5f);
					float dist = glm::length(pos - sphereOrigin) - sphereRadius;

					sdfVolume[idx] = dist;
				}
			}
		}

		D3D11_SUBRESOURCE_DATA sub = {};
		sub.pSysMem = sdfVolume;
		sub.SysMemPitch = sdfWidth * 4;
		sub.SysMemSlicePitch = sdfWidth * sdfHeight * 4;

		if (AppContext->d3dDevice->CreateTexture3D(&desc, &sub, &Tool->sdfVolumeTexture) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}

		delete[] sdfVolume;

		if (AppContext->d3dDevice->CreateShaderResourceView(Tool->sdfVolumeTexture, NULL, &Tool->sdfVolumeResourceView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &Tool->sdfVolumeSamplerState);
	}

	//----------------------------------------------------------------------------------------------------
	// Create SDF of triangle mesh.
	//----------------------------------------------------------------------------------------------------
	{
		if (!gfxCreateTexture3DUav(AppContext, Tool->sdfVolumeTexture, &Tool->sdfVolumeUav)) {
			std::cout << "Could not create UAV\n";
			return 1;
		}

		//ldiModel dergnModel = objLoadModel("../../assets/models/dergn.obj");
		ldiModel dergnModel = objLoadModel("../../assets/models/materialball.obj");

		float scale = 1;
		for (int i = 0; i < dergnModel.verts.size(); ++i) {
			ldiMeshVertex* vert = &dergnModel.verts[i];
			vert->pos = vert->pos * scale;
		}

		// Create tri list.
		std::vector<vec3> tris;
		tris.reserve(dergnModel.indices.size());

		for (size_t i = 0; i < dergnModel.indices.size(); ++i) {
			tris.push_back(dergnModel.verts[dergnModel.indices[i]].pos);
		}

		ID3D11Buffer* triListBuffer;

		if (!gfxCreateStructuredBuffer(AppContext, 4 * 3, tris.size(), tris.data(), &triListBuffer)) {
			std::cout << "Could not create structured buffer\n";
			return 1;
		}

		ID3D11ShaderResourceView* triListResourceView;

		if (!gfxCreateBufferShaderResourceView(AppContext, triListBuffer, &triListResourceView)) {
			std::cout << "Could not create shader resource view\n";
			return 1;
		}

		ID3D11Buffer* constantBuffer;

		struct ldiSdfComputeConstantBuffer {
			int triCount;
			float testVal;
			float __p0;
			float __p1;
		};

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(ldiSdfComputeConstantBuffer);
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			ldiSdfComputeConstantBuffer cbTemp;
			cbTemp.triCount = 10000;//tris.size() / 3;
			cbTemp.testVal = 0.0f;

			D3D11_SUBRESOURCE_DATA constantBufferData = {};
			constantBufferData.pSysMem = &cbTemp;
			
			if (AppContext->d3dDevice->CreateBuffer(&desc, &constantBufferData, &constantBuffer) != S_OK) {
				std::cout << "Failed to create constant buffer\n";
				return 1;
			}
		}
		
		AppContext->d3dDeviceContext->CSSetShader(Tool->sdfVolumeCompute, 0, 0);
		AppContext->d3dDeviceContext->CSSetShaderResources(0, 1, &triListResourceView);
		AppContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &Tool->sdfVolumeUav, 0);
		AppContext->d3dDeviceContext->CSSetConstantBuffers(0, 1, &constantBuffer);
		AppContext->d3dDeviceContext->Dispatch(128, 128, 128);
		AppContext->d3dDeviceContext->CSSetShader(0, 0, 0);
		ID3D11UnorderedAccessView* ppUAViewnullptr[1] = { 0 };
		AppContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewnullptr, 0);
	}

	return 0;
}

void modelEditorRender(ldiModelEditor* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	/*pushDebugLine(appContext, vec3(3, 0, 0), vec3(3, 5, 0), vec3(0.25, 0, 0.25));
	pushDebugLine(appContext, vec3(3, 5, 0), vec3(3, 10, 0), vec3(0.5, 0, 0.5));
	pushDebugLine(appContext, vec3(3, 10, 0), vec3(3, 12, 0), vec3(0.75, 0, 0.75));
	pushDebugLine(appContext, vec3(3, 12, 0), vec3(3, 15, 0), vec3(1, 0, 1));*/
	
	//pushDebugBox(appContext, vec3(0, 0, 1), vec3(2, 2, 2), vec3(1, 0, 1));
	//pushDebugBoxSolid(appContext, vec3(0, 0, 1), vec3(2, 2, 2), vec3(0.5, 0, 0.5));

	// Grid.
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
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)Tool->mainViewWidth, (float)Tool->mainViewHeight, 0.01f, 100.0f);
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
	viewport.Width = Tool->mainViewWidth;
	viewport.Height = Tool->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->renderViewBuffers.mainViewRtView, (float*)&Tool->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------------------------
	// Render models.
	//----------------------------------------------------------------------------------------------------
	{
		mat4 projViewMat = projMat * camViewMat;
		mat4 invProjViewMat = glm::inverse(projViewMat);

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = invProjViewMat;
		constantBuffer->proj = projViewMat;
		constantBuffer->color = vec4(0, 0, 0, _getTime(appContext));
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		appContext->d3dDeviceContext->PSSetShaderResources(0, 1, &Tool->sdfVolumeResourceView);
		appContext->d3dDeviceContext->PSSetSamplers(0, 1, &Tool->sdfVolumeSamplerState);

		gfxRenderBasicModel(appContext, &Tool->fullScreenQuad, Tool->raymarchInputLayout, Tool->raymarchVertexShader, Tool->raymarchPixelShader, appContext->nowriteDepthStencilState);
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

void modelEditorShowUi(ldiModelEditor* Tool) {
	ImGui::Begin("Model editor controls");
	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Background", (float*)&Tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid", (float*)&Tool->gridColor);
		ImGui::SliderFloat("Camera speed", &Tool->cameraSpeed, 0.0f, 1.0f);
	}
		
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Model editor", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// This will catch our interactions.
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
		modelEditorRender(Tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(Tool->renderViewBuffers.mainViewResourceView, viewSize);

		// NOTE: ImDrawList API uses screen coordinates!
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int i = 0; i < textBuffer.size(); ++i) {
			ldiTextInfo* info = &textBuffer[i];
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
		}

		{
			// Viewport overlay widgets.
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 25), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::EndChild();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}