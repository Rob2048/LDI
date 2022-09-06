#pragma once

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

struct ldiModelInspector {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;

	bool						primaryModelShowWireframe = false;
	bool						primaryModelShowShaded = false;
	bool						showPointCloud = false;
	bool						quadMeshShowWireframe = false;
	bool						quadMeshShowDebug = true;

	float						pointWorldSize = 0.01f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	float						cameraSpeed = 1.0f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	ldiModel					dergnModel;
	ldiRenderModel				dergnRenderModel;
	ldiRenderModel				dergnDebugModel;
	ID3D11ShaderResourceView*	shaderResourceViewTest;
	ID3D11SamplerState*			texSamplerState;
	ldiRenderLines				quadMeshWire;

	ldiRenderPointCloud			pointCloudRenderModel;
};

void modelInspectorVoxelize(ldiModelInspector* ModelInspector) {
	double t0 = _getTime(ModelInspector->appContext);
	stlSaveModel("../cache/source.stl", &ModelInspector->dergnModel);
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
			"../../assets/bin/PolyMender-clean.exe",
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

		// 100um sides.
		//char args[] = "\"C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\Instant Meshes.exe\" C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\voxel.ply -o C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\output.ply --scale 0.02 --smooth 2";
		// 75um sides.
		char args[] = "\"C:\\Projects\\LDI\\WyvernDX11\\assets\\bin\\Instant Meshes.exe\" C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\voxel.ply -o C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\output.ply --scale 0.015";
		// C:\Projects\LDI\WyvernDX11\assets\bin

		CreateProcessA(
			"../../assets/bin/Instant Meshes.exe",
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

	ModelInspector->dergnModel = objLoadModel("../../assets/models/dergn.obj");
	//ModelInspector->dergnModel = objLoadModel("../../assets/models/materialball.obj");

	// Scale model.
	//float scale = 12.0 / 11.0;
	float scale = 5.0 / 11.0;
	for (int i = 0; i < ModelInspector->dergnModel.verts.size(); ++i) {
		ldiMeshVertex* vert = &ModelInspector->dergnModel.verts[i];
		vert->pos = vert->pos * scale;
	}

	ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &ModelInspector->dergnModel);

	// Import PLY quad mesh.
	ldiQuadModel quadModel;
	if (!plyLoadQuadMesh("../cache/output.ply", &quadModel)) {
		return 1;
	}

	int quadCount = quadModel.indices.size() / 4;

	double totalSideLengths = 0.0;
	int totalSideCount = 0;
	float maxSide = FLT_MIN;
	float minSide = FLT_MAX;

	// Side lengths.
	for (int i = 0; i < quadCount; ++i) {
		vec3 v0 = quadModel.verts[quadModel.indices[i * 4 + 0]];
		vec3 v1 = quadModel.verts[quadModel.indices[i * 4 + 1]];
		vec3 v2 = quadModel.verts[quadModel.indices[i * 4 + 2]];
		vec3 v3 = quadModel.verts[quadModel.indices[i * 4 + 3]];

		float s0 = glm::length(v0 - v1);
		float s1 = glm::length(v1 - v2);
		float s2 = glm::length(v2 - v3);
		float s3 = glm::length(v3 - v0);

		maxSide = max(maxSide, s0);
		maxSide = max(maxSide, s1);
		maxSide = max(maxSide, s2);
		maxSide = max(maxSide, s3);

		minSide = min(minSide, s0);
		minSide = min(minSide, s1);
		minSide = min(minSide, s2);
		minSide = min(minSide, s3);

		totalSideLengths += s0 + s1 + s2 + s3;
		totalSideCount += 4;
	}

	totalSideLengths /= double(totalSideCount);

	std::cout << "Quad model - quads: " << quadCount << " sideAvg: " << (totalSideLengths * 10000.0) << " um \n";
	std::cout << "Min: " << (minSide * 10000.0) << " um Max: " << (maxSide * 10000.0) << " um \n";

	ModelInspector->dergnDebugModel = gfxCreateRenderQuadModelDebug(AppContext, &quadModel);

	ldiModel quadMeshTriModel;
	convertQuadToTriModel(&quadModel, &quadMeshTriModel);

	//std::cout << "Quad tri mesh stats: " << quadMeshTriModel.verts.size() << 

	//ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &quadMeshTriModel);

	ldiPointCloud pointCloud;
	if (!plyLoadPoints("../../assets/models/dergnScan.ply", &pointCloud)) {
		return 1;
	}

	ModelInspector->pointCloudRenderModel = gfxCreateRenderPointCloud(AppContext, &pointCloud);

	ModelInspector->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &quadModel);

	double t0 = _getTime(AppContext);
	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/dergnTexture.png", &x, &y, &n);
	//uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/materialballTextureGrid.png", &x, &y, &n);
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

	// TODO: Move this to app context.
	ID3D11Texture2D* texResource;
	if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &texResource) != S_OK) {
		std::cout << "Texture failed to create\n";
	}

	imageFree(imageRawPixels);

	AppContext->d3dDevice->CreateShaderResourceView(texResource, NULL, &ModelInspector->shaderResourceViewTest);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // D3D11_FILTER_MIN_MAG_MIP_LINEAR
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

	if (ModelInspector->quadMeshShowDebug) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderDebugModel(appContext, &ModelInspector->dergnDebugModel);
	}

	if (ModelInspector->quadMeshShowWireframe) {	
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderWireModel(appContext, &ModelInspector->quadMeshWire);
	}
}

void modelInspectorShowUi(ldiModelInspector* tool) {
	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Model inspector", 0, ImGuiWindowFlags_NoCollapse);

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
		ldiCamera* camera = &tool->camera;
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
			float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime * tool->cameraSpeed;
			camera->position += camMove * cameraSpeed;
		}
	}

	if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= 0.15f;
		tool->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
	}

	if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
		mouseDelta *= 0.025f;

		ldiCamera* camera = &tool->camera;
		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

		camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
		camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
	}

	ImGui::SetCursorPos(startPos);
	std::vector<ldiTextInfo> textBuffer;
	modelInspectorRender(tool, viewSize.x, viewSize.y, &textBuffer);
	ImGui::Image(tool->renderViewBuffers.mainViewResourceView, viewSize);

	// NOTE: ImDrawList API uses screen coordinates!
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	for (int i = 0; i < textBuffer.size(); ++i) {
		ldiTextInfo* info = &textBuffer[i];
		drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
	}

	// Viewport overlay.
	// TODO: Use command buffer of text locations/strings.
	/*{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::Begin("__viewport", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (clipPos.z > 0) {
			drawList->AddText(ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 255, 255), "X");
		}

		ImGui::End();
	}*/

	{
		// Viewport overlay widgets.
		ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
		ImGui::BeginChild("_simpleOverlayMainView", ImVec2(300, 0), false, ImGuiWindowFlags_NoScrollbar);

		ImGui::Text("Time: (%f)", ImGui::GetTime());
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Separator();
		ImGui::Text("Primary model");
		ImGui::Checkbox("Shaded", &tool->primaryModelShowShaded);
		ImGui::Checkbox("Wireframe", &tool->primaryModelShowWireframe);

		ImGui::Separator();
		ImGui::Text("Quad model");
		ImGui::Checkbox("Area debug", &tool->quadMeshShowDebug);
		ImGui::Checkbox("Quad wireframe", &tool->quadMeshShowWireframe);

		ImGui::Separator();
		ImGui::Text("Point cloud");
		ImGui::Checkbox("Show", &tool->showPointCloud);
		ImGui::SliderFloat("World size", &tool->pointWorldSize, 0.0f, 1.0f);
		ImGui::SliderFloat("Screen size", &tool->pointScreenSize, 0.0f, 32.0f);
		ImGui::SliderFloat("Screen blend", &tool->pointScreenSpaceBlend, 0.0f, 1.0f);

		ImGui::Separator();
		ImGui::Text("Viewport");
		ImGui::ColorEdit3("Background", (float*)&tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid", (float*)&tool->gridColor);
		ImGui::SliderFloat("Camera speed", &tool->cameraSpeed, 0.0f, 1.0f);

		ImGui::Separator();
		ImGui::Text("Processing");
		if (ImGui::Button("Process model")) {
			modelInspectorVoxelize(tool);
			modelInspectorCreateQuadMesh(tool);
		}

		if (ImGui::Button("Voxelize")) {
			modelInspectorVoxelize(tool);
		}

		if (ImGui::Button("Quadralize")) {
			modelInspectorCreateQuadMesh(tool);
		}

		ImGui::EndChild();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}