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

	ldiRenderViewBuffers		renderViewBuffers;
	ldiCamera					camera;
	ID3D11Texture2D*			renderViewCopyStaging;
	uint8_t*					renderViewCopy;

	ID3D11ShaderResourceView*	inspectResourceView;
	ID3D11Texture2D*			inspectTex;

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
	ldiTransform				targetModelMat;
	std::vector<vec3>			targetLocalPoints;
	
	bool						showTargetMain = true;
	
	ldiCharucoResults			charucoResults = {};
	ldiCharucoResults			charucoTruth = {};
	double						charucoRms = 0.0;

	std::vector<ldiCalibSample>	calibSamples;
	int							calibSampleSelectId = -1;
	int							calibSampleLoadedImage = -1;

	cv::Mat						calibCameraMatrix;
	cv::Mat						calibCameraDist;

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
	// X: -10 to 10
	// Y: -10 to 10
	// Z: -10 to 10

	transformInit(&Horse->origin, vec3(-30.0f, 0, 0), vec3(-90, 0, 0));
	transformInit(&Horse->axisX, vec3(19.5f, 0, 9.8f), vec3(0, 0, 0));
	transformInit(&Horse->axisY, vec3(0, 0, 6.8f), vec3(0, 0, 0));
	transformInit(&Horse->axisZ, vec3(0, 0, 17.4f), vec3(0, 0, 0));
	transformInit(&Horse->axisA, vec3(6.9f, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisB, vec3(13.7f, 0, -7.5f), vec3(0, 0, 0));

	horseUpdateMats(Horse);
}

void horseUpdate(ldiHorse* Horse) {
	Horse->axisX.localPos.x = Horse->x + 19.5f;
	Horse->axisY.localPos.y = Horse->y;
	Horse->axisZ.localPos.z = Horse->z + 17.4f;
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

		//float v = (float)Tool->renderViewCopy[i * 4] / 255.0f;
		//Tool->renderViewCopy[i] = (uint8_t)(GammaToLinear(v) * 255.0f);
	}

	//imageWrite("Copy test.png", Tool->imageWidth, Tool->imageHeight, 1, Tool->imageWidth, Tool->renderViewCopy);
}

void _visionSimulatorBuildTargetLocalPoints(ldiVisionSimulator* Tool) {
	const float cubeSize = 5.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 6.0f;
	const float oneCellSize = cubeSize / squareCount;

	Tool->targetLocalPoints.clear();

	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		if (boardIter == 5) {
			continue;
		}

		int cornerCount = (int)_charucoBoards[boardIter]->chessboardCorners.size();

		for (int cornerIter = 0; cornerIter < cornerCount; ++cornerIter) {
			cv::Point3f p = _charucoBoards[boardIter]->chessboardCorners[cornerIter];

			vec3 offset(-chs, chs, chs);
			vec3 localPos = Tool->targetFaceMat[boardIter] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

			Tool->targetLocalPoints.push_back(localPos);
		}
	}
}

int visionSimulatorInit(ldiApp* AppContext, ldiVisionSimulator* Tool) {
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

	Tool->renderViewCopy = new uint8_t[Tool->imageWidth * Tool->imageHeight * 4];

	horseInit(&Tool->horse);

	gfxCreateRenderView(AppContext, &Tool->renderViewBuffers, Tool->imageWidth, Tool->imageHeight);

	Tool->mainCamera.position = vec3(40, 60, -20);
	Tool->mainCamera.rotation = vec3(-125, 40, 0);
	Tool->mainCamera.fov = 60.0f;

	Tool->calibCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	Tool->calibCameraMatrix.at<double>(0, 0) = 1693.30789;
	Tool->calibCameraMatrix.at<double>(0, 1) = 0.0;
	Tool->calibCameraMatrix.at<double>(0, 2) = 800;
	Tool->calibCameraMatrix.at<double>(1, 0) = 0.0;
	Tool->calibCameraMatrix.at<double>(1, 1) = 1693.30789;
	Tool->calibCameraMatrix.at<double>(1, 2) = 650;
	Tool->calibCameraMatrix.at<double>(2, 0) = 0.0;
	Tool->calibCameraMatrix.at<double>(2, 1) = 0.0;
	Tool->calibCameraMatrix.at<double>(2, 2) = 1.0;

	Tool->calibCameraDist = cv::Mat::zeros(8, 1, CV_64F);

	//----------------------------------------------------------------------------------------------------
	// Renderview staging texture.
	//----------------------------------------------------------------------------------------------------
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

		if (AppContext->d3dDevice->CreateTexture2D(&texDesc, NULL, &Tool->renderViewCopyStaging) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Prime inspect texture.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = Tool->imageWidth;
		tex2dDesc.Height = Tool->imageHeight;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DYNAMIC;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		tex2dDesc.MiscFlags = 0;

		if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->inspectTex) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}

		if (AppContext->d3dDevice->CreateShaderResourceView(Tool->inspectTex, NULL, &Tool->inspectResourceView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}
	}

	// NOTE: Board size must allow number of squares to evenly divide.
	const int boardSize = 600;
	static uint8_t boardBuffer[boardSize * boardSize * 4];

	// NOTE: Load charucos as textures from cv engine.
	for (int i = 0; i < 6; ++i) {
		cv::Mat markerImage;
		_charucoBoards[i]->draw(cv::Size(boardSize, boardSize), markerImage, 0, 1);

		//imageWrite("chrtestbrd.png", boardSize, boardSize, 1, boardSize, markerImage.data);

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
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC; 
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
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

	_visionSimulatorBuildTargetLocalPoints(Tool);
	
	Tool->targetModelMat.localPos = vec3(0, 0, 7.5f);
	Tool->targetModelMat.localRot = vec3(90, 0, 0);
	transformUpdateLocal(&Tool->targetModelMat);

	return 0;
}

void _renderTransformOrigin(ldiVisionSimulator* Tool, ldiTransform* Transform, std::string Text, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;
	vec3 root = transformGetWorldPoint(Transform, vec3(0, 0, 0));

	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(1, 0, 0)), vec3(1, 0, 0));
	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(0, 1, 0)), vec3(0, 1, 0));
	pushDebugLine(appContext, root, transformGetWorldPoint(Transform, vec3(0, 0, 1)), vec3(0, 0, 1));

	displayTextAtPoint(&Tool->mainCamera, root, Text, vec4(1.0f, 1.0f, 1.0f, 0.6f), TextBuffer);
}

void visionSimulatorMainRender(ldiVisionSimulator* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->mainViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	/*mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->mainCamera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->mainCamera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->mainCamera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)Tool->mainViewWidth, (float)Tool->mainViewHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;*/
	updateCamera3dBasicFps(&Tool->mainCamera, (float)Tool->mainViewWidth, (float)Tool->mainViewHeight);

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
	int gridCount = 30;
	float gridCellWidth = 1.0f;
	vec3 gridColor = Tool->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
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
		_renderTransformOrigin(Tool, &Tool->horse.origin, "Origin", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisX, "X", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisY, "Y", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisZ, "Z", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisA, "A", TextBuffer);
		_renderTransformOrigin(Tool, &Tool->horse.axisB, "B", TextBuffer);

		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		ldiTransform mvCameraTransform = {};
		mvCameraTransform.world = glm::inverse(camViewMat);
		_renderTransformOrigin(Tool, &mvCameraTransform, "Camera", TextBuffer);

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

		pushDebugLine(appContext, f0, f4, vec3(1, 0, 1));
		pushDebugLine(appContext, f1, f5, vec3(1, 0, 1));
		pushDebugLine(appContext, f2, f6, vec3(1, 0, 1));
		pushDebugLine(appContext, f3, f7, vec3(1, 0, 1));

		pushDebugLine(appContext, f0, f1, vec3(1, 0, 1));
		pushDebugLine(appContext, f1, f2, vec3(1, 0, 1));
		pushDebugLine(appContext, f2, f3, vec3(1, 0, 1));
		pushDebugLine(appContext, f3, f0, vec3(1, 0, 1));

		pushDebugLine(appContext, f4, f5, vec3(1, 0, 1));
		pushDebugLine(appContext, f5, f6, vec3(1, 0, 1));
		pushDebugLine(appContext, f6, f7, vec3(1, 0, 1));
		pushDebugLine(appContext, f7, f4, vec3(1, 0, 1));
	}

	//----------------------------------------------------------------------------------------------------
	// Calibration visualilzation.
	//----------------------------------------------------------------------------------------------------
	const float cubeSize = 5.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 6.0f;
	const float oneCellSize = cubeSize / squareCount;

	if (Tool->calibSampleSelectId != -1) {

		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		for (int boardIter = 0; boardIter < Tool->calibSamples[Tool->calibSampleSelectId].charucos.boards.size(); ++boardIter) {
			ldiCharucoBoard* board = &Tool->calibSamples[Tool->calibSampleSelectId].charucos.boards[boardIter];

			if (!board->localMat) {
				continue;
			}
			
			mat4 world = glm::inverse(camViewMat) * board->camLocalMat;

			ldiTransform boardTransform = {};
			boardTransform.world = world;
			_renderTransformOrigin(Tool, &boardTransform, "Board", TextBuffer);

			vec3 v0 = world * vec4(0, 0, 0, 1);
			vec3 v1 = world * vec4(0, cubeSize, 0, 1);
			vec3 v2 = world * vec4(cubeSize, 0, 0, 1);
			vec3 v3 = world * vec4(cubeSize, cubeSize, 0, 1);

			pushDebugTri(Tool->appContext, v0, v1, v2, vec3(0.5f, 0.5f, 0.5f));
			pushDebugTri(Tool->appContext, v1, v3, v2, vec3(0.5f, 0.5f, 0.5f));
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Render cube model.
	//----------------------------------------------------------------------------------------------------
	vec3 targetFaceColorBright[] = {
		{1.0f, 1.0f, 1.0f},
		{1.0f, 0.5f, 0.5f},
		{0.5f, 1.0f, 0.5f},
		{0.5f, 0.5f, 1.0f},
		{1.0f, 1.0f, 0.5f},
		{1.0f, 0.5f, 1.0f},
	};

	vec3 targetFaceColorDim[] = {
		{0.5f, 0.5f, 0.5f},
		{0.5f, 0.0f, 0.0f},
		{0.0f, 0.5f, 0.0f},
		{0.0f, 0.0f, 0.5f},
		{0.5f, 0.5f, 0.0f},
		{0.5f, 0.0f, 0.5f},
	};

	vec3 targetFaceColor[] = {
		{1.0f, 1.0f, 1.0f},
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
	};

	if (Tool->showTargetMain) {
		for (int i = 0; i < 6; ++i) {
			ldiBasicConstantBuffer constantBuffer;
			constantBuffer.mvp = Tool->mainCamera.projMat * Tool->mainCamera.viewMat * Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[i];
			constantBuffer.color = vec4(targetFaceColorDim[i], 1);

			if (i == 5) {
				constantBuffer.color = vec4(0, 0, 0, 1);
			}

			gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
			gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], Tool->targetTexSampler);
		}
	}

	for (int b = 0; b < 6; ++b) {
		if (b == 5) {
			continue;
		}

		vec3 faceOrigin = Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(0, chs, 0, 1);
		vec3 faceWorldNormal = Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(0, 1, 0, 0);

		float fdc = glm::dot((Tool->mainCamera.position - faceOrigin), faceWorldNormal);

		if (fdc < 0) {
			continue;
		}

		for (size_t i = 0; i < _charucoBoards[b]->chessboardCorners.size(); ++i) {
			cv::Point3f p = _charucoBoards[b]->chessboardCorners[i];

			vec3 offset(-chs, chs, chs);
			vec3 worldOrigin = Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

			pushDebugBox(appContext, worldOrigin, vec3(0.05f, 0.05f, 0.05f), vec3(1, 1, 1));
			displayTextAtPoint(&Tool->mainCamera, worldOrigin, std::to_string(i), vec4(targetFaceColorBright[b], 1.0f), TextBuffer);
		}
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

	renderDebugPrimitives(appContext);
}

void visionSimulatorRender(ldiVisionSimulator* Tool) {
	ldiApp* appContext = Tool->appContext;

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

	mat4 projMat = Tool->camera.projMat;
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
	// Render cube model.
	//----------------------------------------------------------------------------------------------------
	for (int i = 0; i < 6; ++i) {
		ldiBasicConstantBuffer constantBuffer;
		constantBuffer.mvp = projMat * camViewMat * Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[i];
		constantBuffer.color = vec4(1, 1, 1, 1);

		if (i == 5) {
			constantBuffer.color = vec4(0, 0, 0, 1);
		}

		gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
		gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], Tool->targetTexSampler);
	}
}

vec3 visionSimulatorGetTargetImagePos(ldiVisionSimulator* Tool, int BoardId, int CornerId) {
	const float cubeSize = 5.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 6.0f;
	const float oneCellSize = cubeSize / squareCount;

	cv::Point3f p = _charucoBoards[BoardId]->chessboardCorners[CornerId];

	vec3 offset(-chs, chs, chs);
	vec3 worldPos = Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[BoardId] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

	// TODO: Camera should really be updated only once.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);
	mat4 projMat = Tool->camera.projMat;
	mat4 projViewModelMat = projMat * camViewMat;

	vec4 clipPos = projViewModelMat * vec4(worldPos, 1);
	clipPos.x /= clipPos.w;
	clipPos.y /= clipPos.w;

	vec3 screenPos;
	screenPos.x = (clipPos.x * 0.5 + 0.5) * Tool->imageWidth;
	screenPos.y = (1.0f - (clipPos.y * 0.5 + 0.5)) * Tool->imageHeight;
	screenPos.z = clipPos.z;

	return screenPos;
}

vec3 visionSimulatorGetTargetCamPos(ldiVisionSimulator* Tool, int BoardId, int CornerId) {
	const float cubeSize = 5.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 6.0f;
	const float oneCellSize = cubeSize / squareCount;

	cv::Point3f p = _charucoBoards[BoardId]->chessboardCorners[CornerId];

	vec3 offset(-chs, chs, chs);
	vec3 worldPos = Tool->horse.axisB.world * Tool->targetModelMat.local * Tool->targetFaceMat[BoardId] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

	// TODO: Camera should really be updated only once.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);
	
	vec3 camPos = camViewMat * vec4(worldPos, 1);
	
	return camPos;
}

void visionSimulatorRenderAndSave(ldiVisionSimulator* Tool, ldiHorse* Horse) {
	visionSimulatorRender(Tool);
	_visionSimulatorCopyRenderToBuffer(Tool);
	ldiImage img = {};
	img.data = Tool->renderViewCopy;
	img.width = Tool->imageWidth;
	img.height = Tool->imageHeight;

	char filePath[256];
	sprintf_s(filePath, sizeof(filePath), "../cache/calib/%d", Tool->calibSamples.size());

	char fileExt[256];
	sprintf_s(fileExt, sizeof(filePath), "%s.png", filePath);
	imageWrite(fileExt, Tool->imageWidth, Tool->imageHeight, 1, Tool->imageWidth, Tool->renderViewCopy);

	ldiCalibSample calibSample;
	calibSample.x = Horse->x;
	calibSample.y = Horse->y;
	calibSample.z = Horse->z;
	calibSample.a = Horse->a;
	calibSample.b = Horse->b;
	
	findCharuco(img, Tool->appContext, &calibSample.charucos);
	
	Tool->calibSamples.push_back(calibSample);

	// Save data file:
	/*sprintf_s(fileExt, sizeof(filePath), "%s.dat", filePath);
	
	FILE* f;
	if (fopen_s(&f, fileExt, "wb") == 0) {
		int boardCount = charucoResults.boards.size();
		fwrite(&boardCount, sizeof(int), 1, f);

		for (int b = 0; b < boardCount; ++b) {

		}

		fclose(f);
	} else {
		std::cout << "Failed to save file\n";
	}*/
}

void visionSimulatorLoadImageSetInfo(ldiVisionSimulator* Tool) {

}

void visionSimulatorCreateImageSet(ldiVisionSimulator* Tool) {
	Tool->calibSamples.clear();

	srand(_getTime(Tool->appContext));

	/*for (int i = 0; i < 20; ++i) {
		Tool->horse.x = rand() % 16 - 8.0f;
		Tool->horse.y = rand() % 16 - 8.0f;
		Tool->horse.z = rand() % 16 - 8.0f;
		Tool->horse.a = rand() % 360;
		Tool->horse.b = rand() % 360;

		horseUpdate(&Tool->horse);
		visionSimulatorRenderAndSave(Tool, &Tool->horse);
	}*/

	/*for (int iX = 0; iX <= 4; ++iX) {
		for (int iY = 0; iY <= 4; ++iY) {
			Tool->horse.x = iX * 4 - 8.0f;
			Tool->horse.y = iY * 4 - 8.0f;
			Tool->horse.z = 0;
			Tool->horse.a = 0;
			Tool->horse.b = 0;

			horseUpdate(&Tool->horse);
			visionSimulatorRenderAndSave(Tool, &Tool->horse);
		}
	}*/

	/*for (int iX = 0; iX <= 10; ++iX) {
		for (int iY = 0; iY <= 10; ++iY) {
			Tool->horse.x = iX * 2 - 10.0f;
			Tool->horse.y = iY * 2 - 10.0f;
			Tool->horse.z = 0;
			Tool->horse.a = 0;
			Tool->horse.b = 0;

			horseUpdate(&Tool->horse);
			visionSimulatorRenderAndSave(Tool, &Tool->horse);
		}
	}*/

	for (int iX = 0; iX <= 20; ++iX) {
		for (int iY = 0; iY <= 20; ++iY) {
			Tool->horse.x = iX - 10.0f;
			Tool->horse.y = iY - 10.0f;
			Tool->horse.z = 0;
			Tool->horse.a = 0;
			Tool->horse.b = 0;

			horseUpdate(&Tool->horse);
			visionSimulatorRenderAndSave(Tool, &Tool->horse);
		}
	}

	for (int iX = 0; iX <= 20; ++iX) {
		for (int iY = 0; iY <= 20; ++iY) {
			Tool->horse.x = iX - 10.0f;
			Tool->horse.y = 0.0f;
			Tool->horse.z = iY - 10.0f;
			Tool->horse.a = 0;
			Tool->horse.b = 0;

			horseUpdate(&Tool->horse);
			visionSimulatorRenderAndSave(Tool, &Tool->horse);
		}
	}
}

void visionSimulatorBundleAdjust(ldiVisionSimulator* Tool) {
	int cornerSize = (int)_charucoBoards[0]->chessboardCorners.size();
	
	// Bundle adjust needs:
	// Pose estimation for each sample.
	// Image points for each sample.
	// Corner global IDs for each sample.
	// Sample ID.

	std::vector<mat4> poses;
	std::vector<int> viewImageIds;
	std::vector<std::vector<cv::Point2f>> viewImagePoints;
	std::vector<std::vector<int>> viewImagePointIds;

	int targetPointCount = (int)Tool->targetLocalPoints.size();
	int totalImagePointCount = 0;
	
	// Find a pose for each calibration sample.
	for (size_t sampleIter = 0; sampleIter < Tool->calibSamples.size(); ++sampleIter) {
		//std::cout << "Sample " << sampleIter << ":\n";

		// Combine all boards into a set of image points and 3d target local points.
		std::vector<cv::Point2f> imagePoints;
		std::vector<cv::Point3f> worldPoints;
		std::vector<int> imagePointIds;

		std::vector<ldiCharucoBoard>* boards = &Tool->calibSamples[sampleIter].charucos.boards;

		for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
			ldiCharucoBoard* board = &(*boards)[boardIter];

			//std::cout << "    Board " << boardIter << " (" << board->id << "):\n";

			for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
				ldiCharucoCorner* corner = &board->corners[cornerIter];

				int cornerGlobalId = (board->id * 25) + corner->id;
				vec3 targetPoint = Tool->targetLocalPoints[cornerGlobalId];

				imagePoints.push_back(cv::Point2f(corner->position.x + 0.5f, corner->position.y + 0.5f));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);

				//std::cout << "        Corner " << cornerIter << " (" << cornerGlobalId << ") " << corner->position.x << ", " << corner->position.y << " " << targetPoint.x << ", " << targetPoint.y << ", " << targetPoint.z << "\n";
			}
		}

		if (imagePoints.size() >= 6) {
			mat4 pose;

			if (computerVisionFindGeneralPose(&Tool->calibCameraMatrix, &Tool->calibCameraDist, &imagePoints, &worldPoints, &pose)) {
				poses.push_back(pose);
				viewImageIds.push_back(sampleIter);
				viewImagePoints.push_back(imagePoints);
				viewImagePointIds.push_back(imagePointIds);
				totalImagePointCount += (int)imagePoints.size();
			}
		}
	}

	// Write SSBA input file.
	FILE* f;
	fopen_s(&f, "../cache/ssba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d %d\n", targetPointCount, poses.size(), totalImagePointCount);

	// Camera intrinsics.
	fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
		Tool->calibCameraMatrix.at<double>(0), Tool->calibCameraMatrix.at<double>(1), Tool->calibCameraMatrix.at<double>(2), Tool->calibCameraMatrix.at<double>(4), Tool->calibCameraMatrix.at<double>(5),
		Tool->calibCameraDist.at<double>(0), Tool->calibCameraDist.at<double>(1), Tool->calibCameraDist.at<double>(2), Tool->calibCameraDist.at<double>(3));

	// 3D points.
	for (size_t pointIter = 0; pointIter < Tool->targetLocalPoints.size(); ++pointIter) {
		vec3 point = Tool->targetLocalPoints[pointIter];
		fprintf(f, "%d %f %f %f\n", pointIter, point.x, point.y, point.z);
	}

	// View poses.
	for (size_t poseIter = 0; poseIter < poses.size(); ++poseIter) {
		mat4 camRt = poses[poseIter];
		fprintf(f, "%d ", viewImageIds[poseIter]);

		// ViewPointImageId 12 value camRT matrix.
		//for (int j = 0; j < 12; ++j) {
			// NOTE: Index column major, output as row major.
			//fprintf(f, " %f", camRt[(j % 4) * 4 + (j / 4)]);
		//}

		// NOTE: Index column major, output as row major.
		fprintf(f, "%f ", camRt[0][0]);
		fprintf(f, "%f ", camRt[1][0]);
		fprintf(f, "%f ", camRt[2][0]);
		fprintf(f, "%f ", camRt[3][0]);

		fprintf(f, "%f ", camRt[0][1]);
		fprintf(f, "%f ", camRt[1][1]);
		fprintf(f, "%f ", camRt[2][1]);
		fprintf(f, "%f ", camRt[3][1]);

		fprintf(f, "%f ", camRt[0][2]);
		fprintf(f, "%f ", camRt[1][2]);
		fprintf(f, "%f ", camRt[2][2]);
		fprintf(f, "%f",  camRt[3][2]);

		fprintf(f, "\n");
	}

	// Image points.
	for (size_t viewIter = 0; viewIter < viewImagePoints.size(); ++viewIter) {
		std::vector<cv::Point2f>* viewPoints = &viewImagePoints[viewIter];

		for (size_t pointIter = 0; pointIter < viewImagePoints[viewIter].size(); ++pointIter) {
			cv::Point2f point = viewImagePoints[viewIter][pointIter];
			int pointId = viewImagePointIds[viewIter][pointIter];

			fprintf(f, "%d %d %f %f 1\n", viewImageIds[viewIter], pointId, point.x, point.y);
		}
	}

	fclose(f);

	//----------------------------------------------------------------------------------------------------
	// Run bundle adjuster.
	//----------------------------------------------------------------------------------------------------
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// 100um sides and smoothing.
	//char args[] = "\"Instant Meshes.exe\" ../../bin/cache/voxel.ply -o ../../bin/cache/output.ply --scale 0.02 --smooth 2";
	// 75um sides.
	char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt tangential";

	CreateProcessA(
		"../../assets/bin/BundleCommon.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		//"../../assets/bin",
		"../cache",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);
}

void visionSimulatorLoadCalibImage(ldiVisionSimulator* Tool) {
	if (Tool->calibSampleLoadedImage == Tool->calibSampleSelectId) {
		return;
	}

	Tool->calibSampleLoadedImage = Tool->calibSampleSelectId;

	if (Tool->calibSampleLoadedImage == -1) {
		return;
	}

	double t0 = _getTime(Tool->appContext);
	
	char filename[256];
	sprintf_s(filename, sizeof(filename), "../cache/calib/%d.png", Tool->calibSampleLoadedImage);
	int w, h, c;
	uint8_t* imgData = imageLoadRgba(filename, &w, &h, &c);
	
	if (w != Tool->imageWidth || h != Tool->imageHeight) {
		std::cout << "Load image size error\n";
		imageFree(imgData);
		return;
	}

	D3D11_MAPPED_SUBRESOURCE ms;
	Tool->appContext->d3dDeviceContext->Map(Tool->inspectTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	uint8_t* pixelData = (uint8_t*)ms.pData;

	for (int i = 0; i < Tool->imageHeight; ++i) {
		memcpy(pixelData + i * ms.RowPitch, imgData + i * Tool->imageWidth * 4, Tool->imageWidth * 4);
	}

	Tool->appContext->d3dDeviceContext->Unmap(Tool->inspectTex, 0);

	imageFree(imgData);

	t0 = _getTime(Tool->appContext) - t0;
	std::cout << "Load image: " << filename << " in " << t0 * 1000.0f << " ms\n";
}

void visionSimulatorShowUi(ldiVisionSimulator* Tool) {
	//std::cout << Tool->mainCamera.position.x << "," << Tool->mainCamera.position.y << "," << Tool->mainCamera.position.z << " " << Tool->mainCamera.rotation.x << "," << Tool->mainCamera.rotation.y << "\n";
	//----------------------------------------------------------------------------------------------------
	// Vision simulator controls.
	//----------------------------------------------------------------------------------------------------
	ImGui::Begin("Vision simulator controls");
	
	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
		ImGui::Text("Cursor position: %.2f, %.2f", Tool->imageCursorPos.x, Tool->imageCursorPos.y);
		ImGui::Text("Cursor value: %d", Tool->imagePixelValue);
		ImGui::SliderFloat("Scale", &Tool->imageScale, 0.1f, 10.0f);
	}

	if (ImGui::CollapsingHeader("Machine", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("XYZ", &Tool->horse.x, 0.1f, -10.0f, 10.0f);
		ImGui::DragFloat2("AB", &Tool->horse.a, 1.0f, 0.0f, 360.0f);
		ImGui::DragFloat3("Cam position", (float*)&Tool->camera.position, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat2("Cam rotation", (float*)&Tool->camera.rotation, 0.1f, -360.0f, 360.0f);
		ImGui::SliderFloat("Main camera speed", &Tool->mainCameraSpeed, 0.0f, 1.0f);
		ImGui::Checkbox("Show target cube", &Tool->showTargetMain);
	}

	char strbuf[256];

	if (ImGui::CollapsingHeader("Calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Find Charuco")) {
			double rms = 0.0;
			int rmsCount = 0;

			_visionSimulatorCopyRenderToBuffer(Tool);
			ldiImage img = {};
			img.data = Tool->renderViewCopy;
			img.width = Tool->imageWidth;
			img.height = Tool->imageHeight;
			findCharuco(img, Tool->appContext, &Tool->charucoResults);

			Tool->charucoTruth.boards.clear();
			
			cv::Mat rvec = cv::Mat::zeros(3, 1, CV_64F);
			cv::Mat tvec = cv::Mat::zeros(3, 1, CV_64F);
			
			// Compare each found point with the ground truth.
			for (size_t b = 0; b < Tool->charucoResults.boards.size(); ++b) {
				ldiCharucoBoard board;
				board.id = Tool->charucoResults.boards[b].id;

				for (size_t c = 0; c < Tool->charucoResults.boards[b].corners.size(); ++c) {
					ldiCharucoCorner corner;
					corner.id = Tool->charucoResults.boards[b].corners[c].id;
					vec3 camSpacePos = visionSimulatorGetTargetCamPos(Tool, board.id, corner.id);
					vec2 gtPos = visionSimulatorGetTargetImagePos(Tool, board.id, corner.id);

					// Get ground truth reprojection.
					std::vector<cv::Point3f> points;
					std::vector<cv::Point2f> imagePoints;
					points.push_back(cv::Point3f(-camSpacePos.x, camSpacePos.y, camSpacePos.z));
					//cv::projectPoints(points, rvec, tvec, cameraMatrix, distCoeffs, imagePoints);
					cv::projectPoints(points, rvec, tvec, Tool->calibCameraMatrix, Tool->calibCameraDist, imagePoints);

					corner.position = vec2(imagePoints[0].x, imagePoints[0].y);

					double d = glm::distance(corner.position, Tool->charucoResults.boards[b].corners[c].position + vec2(0.5, 0.5));
					//double d = glm::distance(corner.position, gtPos);
					
					rms += d * d;
					rmsCount++;

					board.corners.push_back(corner);
				}

				Tool->charucoTruth.boards.push_back(board);
			}

			if (rmsCount != 0) {
				Tool->charucoRms = sqrt(rms / rmsCount);
			} else {
				Tool->charucoRms = 1000000;
			}
		}

		ImGui::Text("RMS: %f", Tool->charucoRms);

		if (ImGui::Button("Create image set")) {
			visionSimulatorCreateImageSet(Tool);
		}

		if (ImGui::Button("Calibrate camera")) {
			computerVisionCalibrateCameraCharuco(Tool->appContext, &Tool->calibSamples, Tool->imageWidth, Tool->imageHeight, &Tool->calibCameraMatrix, &Tool->calibCameraDist);
		}

		if (ImGui::Button("Bundle adjust")) {
			visionSimulatorBundleAdjust(Tool);
		}

		if (ImGui::Button("Show live")) {
			Tool->calibSampleSelectId = -1;
		}

		ImGui::Text("Samples");
		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
			for (int n = 0; n <  Tool->calibSamples.size(); ++n) {
				const bool is_selected = (Tool->calibSampleSelectId == n);

				sprintf_s(strbuf, sizeof(strbuf), "%d", n);
				if (ImGui::Selectable(strbuf, is_selected)) {
					Tool->calibSampleSelectId = n;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	ImGui::End();

	//----------------------------------------------------------------------------------------------------
	// Sim updates.
	//----------------------------------------------------------------------------------------------------
	if (Tool->calibSampleSelectId != -1) {
		Tool->horse.x = Tool->calibSamples[Tool->calibSampleSelectId].x;
		Tool->horse.y = Tool->calibSamples[Tool->calibSampleSelectId].y;
		Tool->horse.z = Tool->calibSamples[Tool->calibSampleSelectId].z;
		Tool->horse.a = Tool->calibSamples[Tool->calibSampleSelectId].a;
		Tool->horse.b = Tool->calibSamples[Tool->calibSampleSelectId].b;
	}

	horseUpdate(&Tool->horse);

	//----------------------------------------------------------------------------------------------------
	// Vision simulator image view.
	//----------------------------------------------------------------------------------------------------
	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	if (ImGui::Begin("Vision simulator", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar)) {
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

		if (Tool->calibSampleSelectId == -1) {
			draw_list->AddImage(Tool->renderViewBuffers.mainViewResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		} else {
			visionSimulatorLoadCalibImage(Tool);
			draw_list->AddImage(Tool->inspectResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		}

		draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

		//----------------------------------------------------------------------------------------------------
		// Draw charuco results.
		//----------------------------------------------------------------------------------------------------
		std::vector<vec2> markerCentroids;

		const ldiCharucoResults* charucos = &Tool->charucoResults;

		if (Tool->calibSampleSelectId != -1) {
			charucos = &Tool->calibSamples[Tool->calibSampleSelectId].charucos;
		}

		for (int i = 0; i < charucos->markers.size(); ++i) {
			/*ImVec2 offset = pos;
			offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
			offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;*/

			ImVec2 points[5];

			vec2 markerCentroid(0.0f, 0.0f);

			for (int j = 0; j < 4; ++j) {
				vec2 o = charucos->markers[i].corners[j];

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

		for (int i = 0; i < charucos->markers.size(); ++i) {
			char strBuff[32];
			sprintf_s(strBuff, sizeof(strBuff), "%d", charucos->markers[i].id);
			draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;

				// TODO: Do we need half pixel offset? Check debug drawing commands.
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

				draw_list->AddCircleFilled(offset, 4.0f, ImColor(0, 255, 0, 200));
			}
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;
				int cornerId = charucos->boards[b].corners[i].id;

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale + 5;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale - 15;

				char strBuf[256];
				// NOTE: Half pixel offsets added to position values.
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x + 0.5f, o.y + 0.5f);

				draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Ground truth points.
		//----------------------------------------------------------------------------------------------------
		charucos = &Tool->charucoTruth;

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;

				// TODO: Do we need half pixel offset? Check debug drawing commands.
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x) * Tool->imageScale;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y) * Tool->imageScale;

				draw_list->AddCircleFilled(offset, 4.0f, ImColor(255, 0, 0, 200));
			}
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;
				int cornerId = charucos->boards[b].corners[i].id;

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x) * Tool->imageScale + 5;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y) * Tool->imageScale + 0;

				char strBuf[256];
				// NOTE: Half pixel offsets added to position values.
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x, o.y);

				draw_list->AddText(offset, ImColor(255, 0, 0), strBuf);
			}
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
	}

	ImGui::End();
	ImGui::PopStyleVar();

	//----------------------------------------------------------------------------------------------------
	// Vision simulator 3D view.
	//----------------------------------------------------------------------------------------------------
	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Visual simulator 3D view", 0, ImGuiWindowFlags_NoCollapse)) {
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