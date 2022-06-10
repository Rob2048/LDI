#pragma once

struct ldiModelInspector {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;

	bool						primaryModelShowWireframe = false;
	bool						primaryModelShowShaded = true;
	bool						showPointCloud = false;

	float						pointWorldSize = 0.01f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	ldiModel					dergnModel;
	ldiRenderModel				dergnRenderModel;
	ID3D11ShaderResourceView*	shaderResourceViewTest;
	ID3D11SamplerState*			texSamplerState;
	ldiRenderLines				quadMeshWire;

	ldiRenderPointCloud			pointCloudRenderModel;
};

void modelInspectorVoxelize(ldiModelInspector* ModelInspector) {
	double t0 = _getTime(ModelInspector->appContext);
	stlSaveModel("./cache/source.stl", &ModelInspector->dergnModel);
	t0 = _getTime(ModelInspector->appContext) - t0;
	std::cout << "Save STL: " << t0 * 1000.0f << " ms\n";

	// Run: PolyMender-clean.exe source.stl 10 0.9 voxel.ply
	{
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		char args[] = "C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\PolyMender-clean.exe C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\source.stl 10 0.9 C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\voxel.ply";

		// C:\Projects\LDI\WyvernDX11\assets\bin

		CreateProcessA(
			"../assets/bin/PolyMender-clean.exe",
			args,
			NULL,
			NULL,
			FALSE,
			0, //CREATE_NEW_CONSOLE,
			NULL,
			"C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\",
			&si,
			&pi
		);

		WaitForSingleObject(pi.hProcess, INFINITE);
	}
}

void modelInspectorCreateQuadMesh(ldiModelInspector* ModelInspector) {
	// Run: "Instant Meshes.exe" voxel.ply -o output.ply --scale 0.02
	{
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		char args[] = "\"C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\Instant Meshes.exe\" C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\voxel.ply -o C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\output.ply --scale 0.02 --smooth 2";
		// C:\Projects\LDI\WyvernDX11\assets\bin

		CreateProcessA(
			"../assets/bin/Instant Meshes.exe",
			args,
			NULL,
			NULL,
			FALSE,
			0, //CREATE_NEW_CONSOLE,
			NULL,
			"C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\",
			&si,
			&pi
		);

		WaitForSingleObject(pi.hProcess, INFINITE);
	}
}

int modelInspectorInit(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->appContext = AppContext;
	ModelInspector->mainViewWidth = 0;
	ModelInspector->mainViewHeight = 0;
	
	ModelInspector->camera = {};
	ModelInspector->camera.position = vec3(0, 0, 10);
	ModelInspector->camera.rotation = vec3(0, 0, 0);

	ModelInspector->dergnModel = objLoadModel("../assets/models/dergn.obj");

	// Scale model.
	float scale = 12.0 / 11.0;
	for (int i = 0; i < ModelInspector->dergnModel.verts.size(); ++i) {
		ldiMeshVertex* vert = &ModelInspector->dergnModel.verts[i];
		vert->pos = vert->pos * scale;
	}

	//ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &ModelInspector->dergnModel);

	// Import PLY quad mesh.
	ldiQuadModel quadModel;
	if (!plyLoadQuadMesh("cache/output.ply", &quadModel)) {
		return 1;
	}

	ldiModel quadMeshTriModel;
	convertQuadToTriModel(&quadModel, &quadMeshTriModel);

	//std::cout << "Quad tri mesh stats: " << quadMeshTriModel.verts.size() << 

	ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &quadMeshTriModel);

	ldiPointCloud pointCloud;
	if (!plyLoadPoints("../assets/models/dergnScan.ply", &pointCloud)) {
		return 1;
	}

	ModelInspector->pointCloudRenderModel = gfxCreateRenderPointCloud(AppContext, &pointCloud);

	ModelInspector->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &quadModel);

	double t0 = _getTime(AppContext);
	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba("../assets/models/dergnTexture.png", &x, &y, &n);
	t0 = _getTime(AppContext) - t0;
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ") " << t0 * 1000.0f << " ms\n";

	D3D11_SUBRESOURCE_DATA texData = {};
	texData.pSysMem = imageRawPixels;
	texData.SysMemPitch = x * 4;

	D3D11_TEXTURE2D_DESC tex2dDesc = {};
	tex2dDesc.Width = x;
	tex2dDesc.Height = y;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.MiscFlags = 0;

	ID3D11Texture2D* texResource;
	if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &texResource) != S_OK) {
		std::cout << "Texture failed to create\n";
	}

	imageFree(imageRawPixels);

	AppContext->d3dDevice->CreateShaderResourceView(texResource, NULL, &ModelInspector->shaderResourceViewTest);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
	AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &ModelInspector->texSamplerState);

	return 0;
}

vec3 projectPoint(ldiCamera* Camera, int ViewWidth, int ViewHeight, vec3 Pos) {
	// TODO: Should be cached in the camera after calculating final position.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)ViewWidth, (float)ViewHeight, 0.01f, 100.0f);
	
	mat4 projViewModelMat = projMat * camViewMat;
	
	vec4 worldPos = vec4(Pos.x, Pos.y, Pos.z, 1);
	vec4 clipPos = projViewModelMat * worldPos;
	clipPos.x /= clipPos.w;
	clipPos.y /= clipPos.w;

	vec3 screenPos;
	screenPos.x = (clipPos.x * 0.5 + 0.5) * ViewWidth;
	screenPos.y = (1.0f - (clipPos.y * 0.5 + 0.5)) * ViewHeight;
	screenPos.z = clipPos.z;

	return screenPos;
}

void modelInspectorRender(ldiModelInspector* ModelInspector, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = ModelInspector->appContext;

	if (Width != ModelInspector->mainViewWidth || Height != ModelInspector->mainViewHeight) {
		ModelInspector->mainViewWidth = Width;
		ModelInspector->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &ModelInspector->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));


	pushDebugLine(appContext, vec3(3, 0, 0), vec3(3, 5, 0), vec3(0.25, 0, 0.25));
	pushDebugLine(appContext, vec3(3, 5, 0), vec3(3, 10, 0), vec3(0.5, 0, 0.5));
	pushDebugLine(appContext, vec3(3, 10, 0), vec3(3, 12, 0), vec3(0.75, 0, 0.75));
	pushDebugLine(appContext, vec3(3, 12, 0), vec3(3, 15, 0), vec3(1, 0, 1));
	
	
	pushDebugBox(appContext, vec3(2.5, 10, 0), vec3(0.05, 0.05, 0.05), vec3(1, 0, 1));
	pushDebugBoxSolid(appContext, vec3(1, 10, 0), vec3(0.005, 0.005, 0.005), vec3(1, 0, 1));

	ldiTextInfo text0 = {};

	vec3 projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 0, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "0 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 5, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "50 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 10, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "100 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 12, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "120 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 15, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "150 mm";
		TextBuffer->push_back(text0);
	}

	// Grid.
	int gridCount = 10;
	float gridCellWidth = 1.0f;
	vec3 gridColor = ModelInspector->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(ModelInspector->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(ModelInspector->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -ModelInspector->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)ModelInspector->mainViewWidth, (float)ModelInspector->mainViewHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &ModelInspector->renderViewBuffers.mainViewRtView, ModelInspector->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = ModelInspector->mainViewWidth;
	viewport.Height = ModelInspector->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(ModelInspector->renderViewBuffers.mainViewRtView, (float*)&ModelInspector->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(ModelInspector->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
	// Render models.
	//----------------------------------------------------------------------------------------------------
	if (ModelInspector->primaryModelShowShaded) {
		gfxRenderModel(appContext, &ModelInspector->dergnRenderModel, false, ModelInspector->shaderResourceViewTest, ModelInspector->texSamplerState);
	}

	if (ModelInspector->primaryModelShowWireframe) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderModel(appContext, &ModelInspector->dergnRenderModel, true);
	}

	if (ModelInspector->showPointCloud) {
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->screenSize = vec4(ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, 0, 0);
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			constantBuffer->view = camViewMat;
			constantBuffer->proj = projMat;
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
		}
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->pointcloudConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiPointCloudConstantBuffer* constantBuffer = (ldiPointCloudConstantBuffer*)ms.pData;
			//constantBuffer->size = vec4(0.1f, 0.1f, 0, 0);
			//constantBuffer->size = vec4(16, 16, 1, 0);
			constantBuffer->size = vec4(ModelInspector->pointWorldSize, ModelInspector->pointScreenSize, ModelInspector->pointScreenSpaceBlend, 0);
			appContext->d3dDeviceContext->Unmap(appContext->pointcloudConstantBuffer, 0);
		}

		gfxRenderPointCloud(appContext, &ModelInspector->pointCloudRenderModel);
	}

	{	
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderWireModel(appContext, &ModelInspector->quadMeshWire);
	}
}
