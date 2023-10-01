#pragma once

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
	
	ldiRenderModel				targetModel;
	mat4						targetFaceMat[6];
	ldiTransform				targetModelMat;
	std::vector<vec3>			targetLocalPoints;
	std::vector<int>			targetLocalPointsBoard;
	std::vector<int>			targetLocalPointsGlobalId;
	
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

	// Bundle adjust results.
	bool						barLoaded = false;
	ldiCalibCube				barCalibCube;
	std::vector<mat4>			barViews;
	std::vector<int>			barViewIds;

	bool						cameraCalibTargetShow = false;
	ID3D11Texture2D*			cameraCalibTargetTexture;
	ID3D11ShaderResourceView*	cameraCalibTargetSrv;
	ldiRenderModel				cameraCalibTargetModel;
	ldiTransform				cameraCalibTargetTransform;
	std::vector<ldiCameraCalibSample> cameraCalibSamples;
	int							cameraCalibSampleSelectId = -1;
};

void _visionSimulatorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiApp* appContext = (ldiApp*)cmd->UserCallbackData;

	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
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
	const float cubeSize = 4.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 4.0f;
	const float oneCellSize = cubeSize / squareCount;

	Tool->targetLocalPoints.clear();
	Tool->targetLocalPointsBoard.clear();
	Tool->targetLocalPointsGlobalId.clear();

	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		// NOTE: This just ignores final board.
		if (boardIter == 5) {
			continue;
		}

		int cornerCount = (int)_charucoBoards[boardIter]->chessboardCorners.size();

		for (int cornerIter = 0; cornerIter < cornerCount; ++cornerIter) {
			cv::Point3f p = _charucoBoards[boardIter]->chessboardCorners[cornerIter];

			vec3 offset(-chs, chs, chs);
			vec3 localPos = Tool->targetFaceMat[boardIter] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

			Tool->targetLocalPoints.push_back(localPos);
			Tool->targetLocalPointsBoard.push_back(boardIter);
			Tool->targetLocalPointsGlobalId.push_back(boardIter * 9 + cornerIter);
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

	//----------------------------------------------------------------------------------------------------
	// Calibration cube.
	//----------------------------------------------------------------------------------------------------
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

		ldiImage boardImage = {};
		boardImage.data = boardBuffer;
		boardImage.width = boardSize;
		boardImage.height = boardSize;
	
		if (!gfxCreateTextureR8G8B8A8GenerateMips(AppContext, &boardImage, &Tool->targetTexFace[i], &Tool->targetShaderViewFace[i])) {
			return 1;
		}
	}

	// NOTE: Create cube model.
	ldiModel targetModel;

	float cubeSize = 4.0f;
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

	//----------------------------------------------------------------------------------------------------
	// Camera calibration target.
	//----------------------------------------------------------------------------------------------------
	{
		//_calibrationTargetImage.
		// Create R8 version of texture.
		uint8_t* tempTexture = new uint8_t[_calibrationTargetImage.width * _calibrationTargetImage.height * 4];
		ldiImage tempImage = {};
		tempImage.data = tempTexture;
		tempImage.width = _calibrationTargetImage.width;
		tempImage.height = _calibrationTargetImage.height;

		for (int p = 0; p < _calibrationTargetImage.width * _calibrationTargetImage.height; ++p) {
			uint8_t v = _calibrationTargetImage.data[p];

			tempTexture[p * 4 + 0] = v;
			tempTexture[p * 4 + 1] = v;
			tempTexture[p * 4 + 2] = v;
			tempTexture[p * 4 + 3] = 255;
		}

		if (!gfxCreateTextureR8G8B8A8GenerateMips(AppContext, &tempImage, &Tool->cameraCalibTargetTexture, &Tool->cameraCalibTargetSrv)) {
			delete[] tempTexture;
			return 1;
		}

		delete[] tempTexture;

		ldiModel targetModel;
		vec2 targetSize(_calibrationTargetFullWidthCm, _calibrationTargetFullHeightCm);
		vec2 tsh = targetSize * 0.5f;

		targetModel.verts.push_back({ {-tsh.x, 0, -tsh.y}, {0, 1, 0}, {0, 1} });
		targetModel.verts.push_back({ {+tsh.x, 0, -tsh.y}, {0, 1, 0}, {1, 1} });
		targetModel.verts.push_back({ {+tsh.x, 0, +tsh.y}, {0, 1, 0}, {1, 0} });
		targetModel.verts.push_back({ {-tsh.x, 0, +tsh.y}, {0, 1, 0}, {0, 0} });
		targetModel.indices.push_back(0);
		targetModel.indices.push_back(1);
		targetModel.indices.push_back(2);
		targetModel.indices.push_back(0);
		targetModel.indices.push_back(2);
		targetModel.indices.push_back(3);

		Tool->cameraCalibTargetModel = gfxCreateRenderModel(AppContext, &targetModel);
		
		Tool->cameraCalibTargetTransform.localPos = vec3(0, 0, -10);
		Tool->cameraCalibTargetTransform.localRot = vec3(45, 0, 0);
		transformUpdateLocal(&Tool->cameraCalibTargetTransform);
	}

	return 0;
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
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.origin, "Origin", TextBuffer);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisX, "X", TextBuffer);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisY, "Y", TextBuffer);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisZ, "Z", TextBuffer);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisA, "A", TextBuffer);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &Tool->horse.axisC, "C", TextBuffer);

		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		ldiTransform mvCameraTransform = {};
		mvCameraTransform.world = glm::inverse(camViewMat);
		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &mvCameraTransform, "Camera", TextBuffer);

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
	// Calibration visualilzation.
	//----------------------------------------------------------------------------------------------------
	const float cubeSize = 4.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 4.0f;
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
			renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "Board", TextBuffer);

			vec3 v0 = world * vec4(0, 0, 0, 1);
			vec3 v1 = world * vec4(0, cubeSize, 0, 1);
			vec3 v2 = world * vec4(cubeSize, 0, 0, 1);
			vec3 v3 = world * vec4(cubeSize, cubeSize, 0, 1);

			pushDebugTri(&Tool->appContext->defaultDebug, v0, v1, v2, vec3(0.5f, 0.5f, 0.5f));
			pushDebugTri(&Tool->appContext->defaultDebug, v1, v3, v2, vec3(0.5f, 0.5f, 0.5f));
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
			constantBuffer.mvp = Tool->mainCamera.projMat * Tool->mainCamera.viewMat * Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[i];
			constantBuffer.color = vec4(targetFaceColorDim[i], 1);

			if (i == 5) {
				constantBuffer.color = vec4(0, 0, 0, 1);
			}

			gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
			gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], appContext->defaultAnisotropicSamplerState);
		}
	}

	for (int b = 0; b < 6; ++b) {
		if (b == 5) {
			continue;
		}

		vec3 faceOrigin = Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(0, chs, 0, 1);
		vec3 faceWorldNormal = Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(0, 1, 0, 0);

		float fdc = glm::dot((Tool->mainCamera.position - faceOrigin), faceWorldNormal);

		if (fdc < 0) {
			continue;
		}

		displayTextAtPoint(&Tool->mainCamera, faceOrigin + faceWorldNormal * 0.5f, "<" + std::to_string(b) + ">", vec4(targetFaceColorBright[b], 1.0f), TextBuffer);

		for (size_t i = 0; i < _charucoBoards[b]->chessboardCorners.size(); ++i) {
			cv::Point3f p = _charucoBoards[b]->chessboardCorners[i];

			vec3 offset(-chs, chs, chs);
			vec3 worldOrigin = Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[b] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

			pushDebugBox(&appContext->defaultDebug, worldOrigin, vec3(0.05f, 0.05f, 0.05f), vec3(1, 1, 1));
			int globalId = b * 9 + i;
			displayTextAtPoint(&Tool->mainCamera, worldOrigin, std::to_string(i) + " (" + std::to_string(globalId) + ")", vec4(targetFaceColorBright[b], 1.0f), TextBuffer);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Loaded bundle adjust results.
	//----------------------------------------------------------------------------------------------------
	if (Tool->barLoaded) {
		mat4 calibCubeWorldMat = glm::identity<mat4>();

		if (Tool->calibSampleSelectId != -1) {
			mat4 viewRotMat = mat4(1.0f);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(180.0f), vec3Up);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(180.0f), vec3Forward);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
			mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);
			
			for (size_t i = 0; i < Tool->barViews.size(); ++i) {
				if (Tool->barViewIds[i] == Tool->calibSampleSelectId) {
					calibCubeWorldMat = glm::inverse(camViewMat) * Tool->barViews[i];
					break;
				}
			}
		}

		for (size_t i = 0; i < Tool->barCalibCube.points.size(); ++i) {
			ldiCalibCubePoint* point = &Tool->barCalibCube.points[i];
			vec3 worldOrigin = calibCubeWorldMat * vec4(point->position, 1.0f);

			pushDebugBox(&appContext->defaultDebug, worldOrigin, vec3(0.05f, 0.05f, 0.05f), targetFaceColor[point->boardId]);
			displayTextAtPoint(&Tool->mainCamera, worldOrigin, std::to_string(point->id) + " (" + std::to_string(point->globalId) + ")", vec4(targetFaceColorBright[point->boardId], 1.0f), TextBuffer);
		}

		for (int i = 0; i < 5; ++i) {
			ldiPlane* plane = &Tool->barCalibCube.sidePlanes[i];
			pushDebugLine(&appContext->defaultDebug, plane->point, plane->point + plane->normal, vec3(255, 255, 0));
		}

		for (size_t i = 0; i < Tool->barViews.size(); ++i) {
			ldiTransform boardTransform = {};
			boardTransform.world = glm::inverse(Tool->barViews[i]);
			renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "View " + std::to_string(Tool->barViewIds[i]), TextBuffer);

			//pushDebugBox(&appContext->defaultDebug, worldOrigin, vec3(0.05f, 0.05f, 0.05f), targetFaceColor[point->boardId]);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Camera calibration target.
	//----------------------------------------------------------------------------------------------------
	if (Tool->cameraCalibTargetShow) {
		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);
		mat4 camWorldMat = glm::inverse(camViewMat);

		transformUpdateLocal(&Tool->cameraCalibTargetTransform);
		mat4 calibTargetLocalMat = Tool->cameraCalibTargetTransform.local;

		if (Tool->cameraCalibSampleSelectId != -1) {
			calibTargetLocalMat = Tool->cameraCalibSamples[Tool->cameraCalibSampleSelectId].simTargetCamLocalMat;
		}

		ldiBasicConstantBuffer constantBuffer;
		constantBuffer.mvp = Tool->mainCamera.projMat * Tool->mainCamera.viewMat * camWorldMat * calibTargetLocalMat;
		constantBuffer.color = vec4(1, 1, 1, 1);
		gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
		gfxRenderModel(appContext, &Tool->cameraCalibTargetModel, false, Tool->cameraCalibTargetSrv, appContext->defaultAnisotropicSamplerState);

		//for (size_t i = 0; i < Tool->cameraCalibSamples.size(); ++i) {
		if (Tool->cameraCalibSampleSelectId != -1) {
			ldiTransform boardTransform = {};
			boardTransform.world = camWorldMat * Tool->cameraCalibSamples[Tool->cameraCalibSampleSelectId].camLocalMat;
			renderTransformOrigin(appContext, &Tool->mainCamera, &boardTransform, "View " + std::to_string(Tool->cameraCalibSampleSelectId), TextBuffer);

			for (size_t pointIter = 0; pointIter < _calibrationTargetLocalPoints.size(); ++pointIter) {
				vec3 pWorld = boardTransform.world * vec4(_calibrationTargetLocalPoints[pointIter], 1.0f);
				pushDebugBox(&appContext->defaultDebug, pWorld, vec3(0.05f, 0.05f, 0.05f), vec3(255, 0, 0));
			}
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

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
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
	if (Tool->showTargetMain) {
		for (int i = 0; i < 6; ++i) {
			ldiBasicConstantBuffer constantBuffer;
			constantBuffer.mvp = projMat * camViewMat * Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[i];
			constantBuffer.color = vec4(1, 1, 1, 1);

			if (i == 5) {
				constantBuffer.color = vec4(0, 0, 0, 1);
			}

			gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
			gfxRenderModel(appContext, &Tool->targetModel, false, Tool->targetShaderViewFace[i], appContext->defaultAnisotropicSamplerState);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Camera calibration target.
	//----------------------------------------------------------------------------------------------------
	if (Tool->cameraCalibTargetShow) {
		transformUpdateLocal(&Tool->cameraCalibTargetTransform);
		ldiBasicConstantBuffer constantBuffer;
		constantBuffer.mvp = projMat * Tool->cameraCalibTargetTransform.local;
		constantBuffer.color = vec4(1, 1, 1, 1);
		gfxMapConstantBuffer(appContext, appContext->mvpConstantBuffer, &constantBuffer, sizeof(ldiBasicConstantBuffer));
		gfxRenderModel(appContext, &Tool->cameraCalibTargetModel, false, Tool->cameraCalibTargetSrv, appContext->defaultAnisotropicSamplerState);
	}
}

vec3 visionSimulatorGetTargetImagePos(ldiVisionSimulator* Tool, int BoardId, int CornerId) {
	const float cubeSize = 4.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 4.0f;
	const float oneCellSize = cubeSize / squareCount;

	cv::Point3f p = _charucoBoards[BoardId]->chessboardCorners[CornerId];

	vec3 offset(-chs, chs, chs);
	vec3 worldPos = Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[BoardId] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

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
	const float cubeSize = 4.0f;
	const float chs = cubeSize * 0.5f;
	const float squareCount = 4.0f;
	const float oneCellSize = cubeSize / squareCount;

	cv::Point3f p = _charucoBoards[BoardId]->chessboardCorners[CornerId];

	vec3 offset(-chs, chs, chs);
	vec3 worldPos = Tool->horse.axisC.world * Tool->targetModelMat.local * Tool->targetFaceMat[BoardId] * vec4(vec3(p.x, p.z, -p.y) + offset, 1);

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

	//char fileExt[256];
	//sprintf_s(fileExt, sizeof(filePath), "%s.png", filePath);
	//imageWrite(fileExt, Tool->imageWidth, Tool->imageHeight, 1, Tool->imageWidth, Tool->renderViewCopy);

	ldiCalibSample calibSample;
	calibSample.x = Horse->x;
	calibSample.y = Horse->y;
	calibSample.z = Horse->z;
	calibSample.a = Horse->a;
	calibSample.b = Horse->b;
	
	findCharuco(img, Tool->appContext, &calibSample.charucos, &Tool->calibCameraMatrix, &Tool->calibCameraDist);
	
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

void visionSimulatorCameraCalibRenderAndProcess(ldiVisionSimulator* Tool) {
	visionSimulatorRender(Tool);
	_visionSimulatorCopyRenderToBuffer(Tool);
	ldiImage img = {};
	img.data = Tool->renderViewCopy;
	img.width = Tool->imageWidth;
	img.height = Tool->imageHeight;

	ldiCameraCalibSample sample;
	if (cameraCalibFindChessboard(Tool->appContext, img, &sample)) {
		sample.simTargetCamLocalMat = Tool->cameraCalibTargetTransform.local;
		Tool->cameraCalibSamples.push_back(sample);
	}
}

void visionSimulatorCreateCameraCalibImageSet(ldiVisionSimulator* Tool) {
	Tool->cameraCalibSamples.clear();

	vec3 positions[] = {
		{0.00, 0.00, -10.00}, {45.00, 0.00, 0.00},
		
		// Flat on.
		{0.00, 0.00, -10.00}, {90.00, 0.00, 0.00},
		{-2.60, 2.50, -16.00}, {90.00, 0.00, 0.00},
		{2.80, 2.50, -16.00}, {90.00, 0.00, 0.00},
		{2.80, -2.50, -16.00}, {90.00, 0.00, 0.00},
		{-2.60, -2.50, -16.00}, {90.00, 0.00, 0.00},
		{0.20, -0.10, -16.00}, {90.00, 0.00, 0.00},

		// Facing right.
		{-3.20, 1.50, -16.00}, {90.90, 0.00, -61.60},
		{-3.20, -1.20, -16.00}, {90.90, 0.00, -61.60},
		{3.90, -1.20, -15.80}, {90.90, 0.00, -41.50},
		{3.90, 1.60, -15.80}, {90.90, 0.00, -41.50},
		{0.00, 1.60, -15.80}, {90.90, 0.00, -41.50},

		// Facing left.
		{2.80, 1.60, -15.80}, {90.90, 0.00, 57.60},
		{-4.80, 1.60, -15.80}, {90.90, 0.00, 39.40},
		{-5.00, -1.60, -15.80}, {90.90, 0.00, 39.40},
		{3.00, -1.60, -15.80}, {90.90, 0.00, 58.30},

		// Facing down.
		{-2.00, 2.90, -15.80}, {89.90, 89.80, 74.90},
		{2.50, 2.90, -15.80}, {89.90, 89.80, 74.90},
		{2.60, -3.10, -15.80}, {89.90, 89.80, 50.30},
		{-2.80, -3.10, -15.80}, {89.90, 89.80, 50.30},

		// Facing up.
		{-2.30, -2.30, -15.80}, {69.30, -89.50, 50.30},
		{2.20, -2.30, -15.80}, {69.30, -89.50, 50.30},
		{2.70, 4.00, -15.80}, {69.30, -89.50, 29.00},
		{-2.90, 4.00, -15.80}, {69.30, -89.50, 29.00},
	};

	std::cout << sizeof(positions) / 24 << "\n";

	int posCount = sizeof(positions) / 24;
	for (int i = 0; i < posCount; ++i) {
		transformInit(&Tool->cameraCalibTargetTransform, positions[i * 2 + 0], positions[i * 2 + 1]);
		visionSimulatorCameraCalibRenderAndProcess(Tool);
	}
}

void visionSimulatorCreateImageSet(ldiVisionSimulator* Tool) {
	Tool->calibSamples.clear();

	srand(_getTime(Tool->appContext));

	for (int i = 0; i < 1000; ++i) {
		Tool->horse.x = rand() % 16 - 8.0f;
		Tool->horse.y = rand() % 16 - 8.0f;
		Tool->horse.z = rand() % 16 - 8.0f;
		Tool->horse.a = rand() % 360;
		Tool->horse.b = rand() % 360;

		horseUpdate(&Tool->horse);
		std::cout << i << "/999\n";
		visionSimulatorRenderAndSave(Tool, &Tool->horse);
	}

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

	/*for (int iX = 0; iX <= 20; ++iX) {
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
	}*/
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

				//int cornerGlobalId = (board->id * 25) + corner->id;
				int cornerGlobalId = (board->id * 9) + corner->id;
				vec3 targetPoint = Tool->targetLocalPoints[cornerGlobalId];

				// NOTE: Basic tests have shown that the SSBA utility considers 0,0 to be corner of a pixel.
				imagePoints.push_back(cv::Point2f(corner->position.x + 0.5f, corner->position.y + 0.5f));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);

				//std::cout << "        Corner " << cornerIter << " (" << cornerGlobalId << ") " << corner->position.x << ", " << corner->position.y << " " << targetPoint.x << ", " << targetPoint.y << ", " << targetPoint.z << "\n";
			}
		}

		if (imagePoints.size() >= 6) {
			mat4 pose;

			std::cout << "Find pose - Sample: " << sampleIter << " :\n";
			if (computerVisionFindGeneralPose(&Tool->calibCameraMatrix, &Tool->calibCameraDist, &imagePoints, &worldPoints, &pose)) {
				//std::cout << "Pose:\n" << GetMat4DebugString(&pose);
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

	// <mode> is one of metric, focal, prinipal, radial, tangental.
	char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt tangential";

	CreateProcessA(
		"../../assets/bin/BundleCommon.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		"../cache",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);
}

bool visionSimulatorLoadBundleAdjustResults(ldiVisionSimulator* Tool) {
	FILE* file;
	fopen_s(&file, "../cache/refined.txt", "r");
	
	float ci[9];
	int modelPointCount = 0;
	int viewCount = 0;
	int imagePointCount = 0;

	fscanf_s(file, "%f %f %f %f %f %f %f %f %f\r\n", &ci[0], &ci[1], &ci[2], &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]);
	Tool->calibCameraMatrix.at<double>(0, 0) = ci[0];
	Tool->calibCameraMatrix.at<double>(0, 1) = ci[1];
	Tool->calibCameraMatrix.at<double>(0, 2) = ci[2];
	Tool->calibCameraMatrix.at<double>(1, 0) = 0.0;
	Tool->calibCameraMatrix.at<double>(1, 1) = ci[3];
	Tool->calibCameraMatrix.at<double>(1, 2) = ci[4];
	Tool->calibCameraMatrix.at<double>(2, 0) = 0.0;
	Tool->calibCameraMatrix.at<double>(2, 1) = 0.0;
	Tool->calibCameraMatrix.at<double>(2, 2) = 1.0;

	Tool->calibCameraDist.at<double>(0) = ci[5];
	Tool->calibCameraDist.at<double>(1) = ci[6];
	Tool->calibCameraDist.at<double>(2) = ci[7];
	Tool->calibCameraDist.at<double>(3) = ci[8];

	fscanf_s(file, "%d %d %d\r\n", &modelPointCount, &viewCount, &imagePointCount);
	std::cout << "Model point count: " << modelPointCount << " View count: " << viewCount << " Image point count: " << imagePointCount << "\n";

	calibCubeInit(&Tool->barCalibCube, 1.0f, 4, 4.0f);
	Tool->barViews.clear();

	for (int i = 0; i < modelPointCount; ++i) {
		vec3 position;
		int pointId;

		fscanf_s(file, "%d %f %f %f\r\n", &pointId, &position[0], &position[1], &position[2]);
		//std::cout << pointId << " " << position.x << ", " << position.y << ", " << position.z << "\n";
		
		if (pointId != i) {
			fclose(file);
			std::cout << __FUNCTION__ ": Loading point with incorrect id." << "\n";
			return false;
		}

		ldiCalibCubePoint point = {};
		point.boardId = i / 9;
		point.id = i % 9;
		point.globalId = i;
		point.position = position;

		Tool->barCalibCube.points.push_back(point);
	}

	for (int i = 0; i < viewCount; ++i) {
		int viewId;
		mat4 viewMat = glm::identity<mat4>();

		fscanf_s(file, "%d %f %f %f %f %f %f %f %f %f %f %f %f\r\n", &viewId,
			&viewMat[0][0], &viewMat[1][0], &viewMat[2][0], &viewMat[3][0],
			&viewMat[0][1], &viewMat[1][1], &viewMat[2][1], &viewMat[3][1],
			&viewMat[0][2], &viewMat[1][2], &viewMat[2][2], &viewMat[3][2]);

		std::cout << "View " << viewId << "\n" << GetMat4DebugString(&viewMat);

		Tool->barViews.push_back(viewMat);
		Tool->barViewIds.push_back(viewId);
	}

	fclose(file);

	calibCubeCalculatePlanes(&Tool->barCalibCube);

	{
		std::cout << "Image position reprojection\n";
		cv::Mat rvec = cv::Mat::zeros(3, 1, CV_64F);
		cv::Mat tvec = cv::Mat::zeros(3, 1, CV_64F);

		for (size_t viewIter = 0; viewIter < Tool->barViews.size(); ++viewIter) {
			//std::cout << "View " << Tool->barViewIds[viewIter] << "\n";
			std::vector<cv::Point3f> points;

			for (size_t pointIter = 0; pointIter < Tool->barCalibCube.points.size(); ++pointIter) {
				vec3 camSpacePos = Tool->barViews[viewIter] * vec4(Tool->barCalibCube.points[pointIter].position, 1.0f);
				points.push_back(cv::Point3f(camSpacePos.x, camSpacePos.y, camSpacePos.z));
			}
			
			std::vector<cv::Point2f> imagePoints;
			cv::projectPoints(points, rvec, tvec, Tool->calibCameraMatrix, Tool->calibCameraDist, imagePoints);

			for (size_t pointIter = 0; pointIter < Tool->barCalibCube.points.size(); ++pointIter) {
				//std::cout << "  " << pointIter << ": " << imagePoints[pointIter].x << ", " << imagePoints[pointIter].y << "\n";
			}
		}
	}

	Tool->barLoaded = true;

	return true;
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

	if (ImGui::CollapsingHeader("Camera calibration target")) {
		ImGui::DragFloat3("Pos", (float*)&Tool->cameraCalibTargetTransform.localPos, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat3("Rot", (float*)&Tool->cameraCalibTargetTransform.localRot, 0.1f, -360.0f, 360.0f);
		ImGui::Checkbox("Show camera calibration target", &Tool->cameraCalibTargetShow);

		char buff[256];
		sprintf_s(buff, "{%.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f}",
			Tool->cameraCalibTargetTransform.localPos.x,
			Tool->cameraCalibTargetTransform.localPos.y,
			Tool->cameraCalibTargetTransform.localPos.z,
			Tool->cameraCalibTargetTransform.localRot.x,
			Tool->cameraCalibTargetTransform.localRot.y,
			Tool->cameraCalibTargetTransform.localRot.z);

		ImGui::InputText("Target transform", buff, 256);
		// {0, 0, -10}, { 45, 0, 0 }
	}

	char strbuf[256];

	if (ImGui::CollapsingHeader("Camera intrinsics")) {
		for (int i = 0; i < 9; ++i) {
			double* matPtr = (double*)Tool->calibCameraMatrix.ptr(0) + 1 * i;
			char inputBuff[256];
			sprintf_s(inputBuff, "I%d", i);
			ImGui::InputDouble(inputBuff, matPtr);
		}

		ImGui::Separator();

		for (int i = 0; i < 8; ++i) {
			double* matPtr = (double*)Tool->calibCameraDist.ptr(0) + 1 * i;
			char inputBuff[256];
			sprintf_s(inputBuff, "D%d", i);
			ImGui::InputDouble(inputBuff, matPtr);
		}
	}

	if (ImGui::CollapsingHeader("Camera calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Find Chessboard")) {
			_visionSimulatorCopyRenderToBuffer(Tool);
			ldiImage img = {};
			img.data = Tool->renderViewCopy;
			img.width = Tool->imageWidth;
			img.height = Tool->imageHeight;

			ldiCameraCalibSample sample;
			if (cameraCalibFindChessboard(Tool->appContext, img, &sample)) {
				Tool->cameraCalibSamples.push_back(sample);
			}
		}

		if (ImGui::Button("Create image set")) {
			visionSimulatorCreateCameraCalibImageSet(Tool);
		}

		if (ImGui::Button("Calibrate camera")) {
			cameraCalibRunCalibration(Tool->appContext, Tool->cameraCalibSamples, Tool->imageWidth, Tool->imageHeight);
		}

		ImGui::Text("Samples");
		if (ImGui::BeginListBox("##camcaliblistbox", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
			for (int n = 0; n < Tool->cameraCalibSamples.size(); ++n) {
				const bool is_selected = (Tool->cameraCalibSampleSelectId == n);

				sprintf_s(strbuf, sizeof(strbuf), "%d", n);
				if (ImGui::Selectable(strbuf, is_selected)) {
					Tool->cameraCalibSampleSelectId = n;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	if (ImGui::CollapsingHeader("Calibration")) {
		if (ImGui::Button("Find Charuco")) {
			double rms = 0.0;
			int rmsCount = 0;

			_visionSimulatorCopyRenderToBuffer(Tool);
			ldiImage img = {};
			img.data = Tool->renderViewCopy;
			img.width = Tool->imageWidth;
			img.height = Tool->imageHeight;
			findCharuco(img, Tool->appContext, &Tool->charucoResults, &Tool->calibCameraMatrix, &Tool->calibCameraDist);

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

		if (ImGui::Button("Load bundle adjust")) {
			visionSimulatorLoadBundleAdjustResults(Tool);
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

				// NOTE: Half pixel offset required.
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

				draw_list->AddCircleFilled(offset, 4.0f, ImColor(0, 255, 0, 200));
			}

			{
				vec2 o = charucos->boards[b].charucoEstimatedImageCenter;
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
				offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;
				draw_list->AddCircle(offset, 6.0f, ImColor(255, 255, 0, 255));
			
				o = charucos->boards[b].charucoEstimatedImageNormal;
				ImVec2 offset2 = pos;
				offset2.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
				offset2.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

				if (charucos->boards[b].charucoEstimatedBoardAngle < 0.3f) {
					draw_list->AddLine(offset, offset2, ImColor(255, 0, 0, 255), 3.0f);
				} else {
					draw_list->AddLine(offset, offset2, ImColor(0, 255, 0, 255), 3.0f);
				}

				char strBuf[256];
				sprintf_s(strBuf, 256, "%.3f", charucos->boards[b].charucoEstimatedBoardAngle);
				draw_list->AddText(offset2, ImColor(0, 200, 0), strBuf);
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
		// Chessboard.
		//----------------------------------------------------------------------------------------------------
		if (Tool->cameraCalibTargetShow) {
			if (Tool->cameraCalibSamples.size() > 0) {
				ldiCameraCalibSample* latest = &Tool->cameraCalibSamples[Tool->cameraCalibSamples.size() - 1];
				for (size_t i = 0; i < latest->imagePoints.size(); ++i) {
					vec2 o = latest->imagePoints[i];

					// NOTE: Half pixel offset required.
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imageOffset.x + o.x + 0.5) * Tool->imageScale;
					offset.y = screenStartPos.y + (Tool->imageOffset.y + o.y + 0.5) * Tool->imageScale;

					draw_list->AddCircleFilled(offset, 4.0f, ImColor(0, 255, 0, 230));
				
					char strBuf[256];
					sprintf_s(strBuf, 256, "%u %.2f, %.2f", i, o.x, o.y);
					draw_list->AddText(offset, ImColor(255, 0, 0), strBuf);
				}
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
					//Tool->imagePixelValue = _pixelsFinal[(int)pixelPos.y * tool->imageWidth + (int)pixelPos.x];
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