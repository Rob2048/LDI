#pragma once

// NOTE: OV9281
//#define CAM_IMG_WIDTH 1280
//#define CAM_IMG_HEIGHT 800

// NOTE: OV2311
//#define CAM_IMG_WIDTH 1600
//#define CAM_IMG_HEIGHT 1300

// NOTE: IMX477
//#define CAM_IMG_WIDTH 4056
//#define CAM_IMG_HEIGHT 3040

// NOTE: IMX219
//#define CAM_IMG_WIDTH 1920
//#define CAM_IMG_HEIGHT 1080
#define CAM_IMG_WIDTH 3280
#define CAM_IMG_HEIGHT 2464

#define TEST_CAM_WIDTH 1600
#define TEST_CAM_HEIGHT 1300

#include "rotaryMeasurement.h"

enum ldiImageInspectorMode {
	IIM_LIVE_CAMERA,
	IIM_CALIBRATION_JOB
};

struct ldiImageInspector {
	ldiApp*						appContext;

	ldiImageInspectorMode		imageMode = IIM_LIVE_CAMERA;

	float						imgScale = 1.0f;
	vec2						imgOffset = vec2(0.0f, 0.0f);

	float						camImageFilterFactor = 0.0f;

	ldiCharucoResults			camImageCharucoResults = {};

	vec2						camImageCursorPos;
	uint8_t						camImagePixelValue;
	
	bool						camImageProcess = false;
	bool						camCalibProcess = false;
	double						calibDetectTimeout = 0;

	ID3D11Buffer*				camImagePixelConstants;
	ID3D11ShaderResourceView*	camResourceView;
	ID3D11Texture2D*			camTex;

	//int							camLastNewFrameId = 0;
	//std::atomic_int				camNewFrameId = 0;
	//uint8_t						camPixelsFinal[CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};

	int							calibJobSelectedSampleType;
	int							calibJobSelectedSampleId;
	uint8_t						calibJobImage[CAM_IMG_WIDTH * CAM_IMG_HEIGHT];

	ldiRotaryResults			rotaryResults;

	std::vector<ldiCameraCalibSample> cameraCalibSamples;
	int							cameraCalibImageWidth;
	int							cameraCalibImageHeight;
	int							cameraCalibSelectedSample = -1;
	cv::Mat						cameraCalibMatrix;
	cv::Mat						cameraCalibDist;
	bool						cameraCalibShowUndistorted = false;
	
	bool						showCharucoResults = false;
	bool						showCharucoRejectedMarkers = false;
	bool						showUndistorted = true;
	bool						showReprojection = false;
	float						sceneOpacity = 0.75f;
	float						imageContrast = 1.0f;
	float						imageBrightness = 0.0f;

	ID3D11ShaderResourceView*	hawkResourceView;
	ID3D11Texture2D*			hawkTex;
	ldiRenderViewBuffers		hawkViewBuffer;
	int							hawkViewWidth;
	int							hawkViewHeight;
	float						hawkImageScale = 1;
	vec2						hawkImageOffset = vec2(0.0f, 0.0f);
	int							hawkLastFrameId = 0;
	uint8_t						hawkFrameBuffer[CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};
	
	bool						hawkScanProcessImage = false;
	bool						hawkEnableGammaTest = false;
	int							hawkScanMappingMin = 22;
	int							hawkScanMappingMax = 75;

	std::vector<vec4>			hawkScanSegs;
	std::vector<vec2>			hawkScanPoints;

	ldiHawk						testCam;
	int							testCamLastFrameId = 0;
	uint8_t						testCamFrameBuffer[TEST_CAM_WIDTH * TEST_CAM_HEIGHT] = {};
	float						testCamFrameBufferFilter[TEST_CAM_WIDTH * TEST_CAM_HEIGHT] = {};
	ID3D11Texture2D*			testhawkTex[2];
	bool						showHeatmap = false;
	std::vector<cv::KeyPoint>	laserProfileBlobs;
};

void _imageInspectorSetCamTexState(ldiImageInspector* Tool, vec4 Params) {
	ldiApp* appContext = Tool->appContext;

	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
	appContext->d3dDeviceContext->PSSetShader(appContext->imgCamPixelShader, NULL, 0);
	appContext->d3dDeviceContext->VSSetShader(appContext->imgCamVertexShader, NULL, 0);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(Tool->camImagePixelConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCamImagePixelConstants* constantBuffer = (ldiCamImagePixelConstants*)ms.pData;
		constantBuffer->params = Params;
		appContext->d3dDeviceContext->Unmap(Tool->camImagePixelConstants, 0);
	}

	appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &Tool->camImagePixelConstants);
}

void _imageInspectorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;

	if (tool->showHeatmap) {
		_imageInspectorSetCamTexState(tool, vec4(tool->imageBrightness, tool->imageContrast, 0.0f, 1.0f));
	} else {
		_imageInspectorSetCamTexState(tool, vec4(tool->imageBrightness, tool->imageContrast, 0.0f, 0));
	}
}

//void _imageInspectorSetStateCallbackHeatmap(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
//	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;
//	_imageInspectorSetCamTexState(tool, vec4(1.0f, 0.0f, 0.0f, 1.0f));
//}
//
//void _imageInspectorSetStateCallback2(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
//	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;
//	ldiApp* appContext = tool->appContext;
//
//	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
//}

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
	// Primary view texture.
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

	//----------------------------------------------------------------------------------------------------
	// Rotary measurment.
	//----------------------------------------------------------------------------------------------------
	rotaryMeasurementInit(&Tool->rotaryResults);

	//----------------------------------------------------------------------------------------------------
	// Machine vision.
	//----------------------------------------------------------------------------------------------------
	{
		Tool->hawkViewWidth = 3280;
		Tool->hawkViewHeight = 2464;
		Tool->hawkImageOffset = vec2(0.0f, 0.0f);
		Tool->hawkImageScale = 1.0f;
		gfxCreateRenderView(AppContext, &Tool->hawkViewBuffer, Tool->hawkViewWidth, Tool->hawkViewHeight);

		// Camera texture.
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = Tool->hawkViewWidth;
			tex2dDesc.Height = Tool->hawkViewHeight;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R8_UNORM;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DYNAMIC;
			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			tex2dDesc.MiscFlags = 0;

			if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->hawkTex) != S_OK) {
				std::cout << "Texture failed to create\n";
				return 1;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->hawkTex, NULL, &Tool->hawkResourceView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Test camera.
	//----------------------------------------------------------------------------------------------------
	hawkInit(&Tool->testCam, "192.168.3.16", 6969);

	return 0;
}

void _imageInspectorRenderHawkCalibration(ldiImageInspector* Tool) {
	ldiApp* appContext = Tool->appContext;

	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->hawkViewBuffer.mainViewRtView, Tool->hawkViewBuffer.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->hawkViewWidth;
	viewport.Height = Tool->hawkViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	vec4 bgCol(0, 0, 0, 0);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->hawkViewBuffer.mainViewRtView, (float*)&bgCol);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->hawkViewBuffer.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	beginDebugPrimitives(&appContext->defaultDebug);

	vec3 gizmoPos(0, 0, 0);
	pushDebugLine(&appContext->defaultDebug, gizmoPos, vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(&appContext->defaultDebug, gizmoPos, vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(&appContext->defaultDebug, gizmoPos, vec3(0, 0, 1), vec3(0, 0, 1));

	//----------------------------------------------------------------------------------------------------
	// Calibration stuff.
	//----------------------------------------------------------------------------------------------------
	{
	
	}

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		/*mat4 viewMat = Tool->appContext->calibrationContext->bundleAdjustResult.samples[Tool->cameraCalibSelectedSample].camLocalMat;
		mat4 projMat = cameraCreateProjectionFromOpenCvCamera(3280, 2464, Tool->appContext->calibrationContext->bundleAdjustResult.cameraMatrix, 0.01f, 100.0f);
		mat4 invProjMat = inverse(projMat);
		mat4 projViewMat = projMat * viewMat;

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);*/
	}

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
}

void _imageInspectorRenderHawkVolume(ldiImageInspector* Tool, int Width, int Height, vec2 ViewPortTopLeft, vec2 ViewPortSize) {
	if (Tool->hawkViewWidth != Width || Tool->hawkViewHeight != Height) {
		Tool->hawkViewWidth = Width;
		Tool->hawkViewHeight = Height;
		gfxCreateRenderView(Tool->appContext, &Tool->hawkViewBuffer, Width, Height);
	}

	ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;
	ldiCalibrationJob* job = &calibContext->calibJob;

	mat4 camWorldMat = job->camVolumeMat;
	//mat4 camWorldMat = job->opInitialCamWorld[HawkId];

	{
		vec3 refToAxis = job->axisA.origin;
		float axisAngleDeg = (Tool->appContext->platform->testPosA) * (360.0 / (32.0 * 200.0 * 90.0));
		mat4 axisRot = glm::rotate(mat4(1.0f), glm::radians(-axisAngleDeg), job->axisA.direction);

		camWorldMat[3] = vec4(vec3(camWorldMat[3]) - refToAxis, 1.0f);
		camWorldMat = axisRot * camWorldMat;
		camWorldMat[3] = vec4(vec3(camWorldMat[3]) + refToAxis, 1.0f);
	}

	mat4 viewMat = cameraConvertOpenCvWorldToViewMat(camWorldMat);
	mat4 projMat = mat4(1.0);

	if (!job->camMat.empty()) {
		projMat = cameraCreateProjectionFromOpenCvCamera(CAM_IMG_WIDTH, CAM_IMG_HEIGHT, job->camMat, 0.01f, 100.0f);
	}

	projMat = cameraProjectionToVirtualViewport(projMat, ViewPortTopLeft, ViewPortSize, vec2(Width, Height));

	ldiCamera camera = {};
	camera.viewMat = viewMat;
	camera.projMat = projMat;
	camera.invProjMat = inverse(projMat);
	camera.projViewMat = projMat * viewMat;
	camera.viewWidth = Width;
	camera.viewHeight = Height;

	std::vector<ldiTextInfo> textBuffer;
	platformRender(Tool->appContext->platform, &Tool->hawkViewBuffer, Width, Height, &camera, &textBuffer, false);
}

void _imageInspectorSelectCalibJob(ldiImageInspector* Tool, int SelectionId, int SelectionType) {
	Tool->calibJobSelectedSampleType = SelectionType;
	Tool->calibJobSelectedSampleId = SelectionId;

	if (SelectionId == -1 || SelectionType == -1) {
		return;
	}

	ldiCalibrationJob* job = &Tool->appContext->calibrationContext->calibJob;
	ldiCalibSample* sample;

	if (SelectionType == 0) {
		sample = &job->samples[Tool->calibJobSelectedSampleId];
	} else if (SelectionType == 1) {
		sample = &job->scanSamples[Tool->calibJobSelectedSampleId];
	} else {
		return;
	}
	
	calibLoadSampleImages(sample);

	ldiImage calibImg = sample->frame;

	if (Tool->showUndistorted) {
		cv::Mat image(cv::Size(calibImg.width, calibImg.height), CV_8UC1, calibImg.data);
		cv::Mat outputImage(cv::Size(calibImg.width, calibImg.height), CV_8UC1);

		cv::undistort(image, outputImage, job->camMat, job->camDist);

		calibImg.data = outputImage.data;
		gfxCopyToTexture2D(Tool->appContext, Tool->hawkTex, calibImg);
	} else {
		cv::Mat srcImage(cv::Size(calibImg.width, calibImg.height), CV_8UC1, calibImg.data);
		cv::Mat downscaleImage;
		cv::Mat upscaleImage;

		// TODO: Remove me.
		cv::resize(srcImage, downscaleImage, cv::Size(3280 / 2, 2464 / 2));
		cv::resize(downscaleImage, upscaleImage, cv::Size(3280, 2464));

		calibImg.data = upscaleImage.data;
		gfxCopyToTexture2D(Tool->appContext, Tool->hawkTex, calibImg);
	}

	calibFreeCalibImages(sample);

	Tool->appContext->platform->testPosX = sample->X;
	Tool->appContext->platform->testPosY = sample->Y;
	Tool->appContext->platform->testPosZ = sample->Z;
	Tool->appContext->platform->testPosC = sample->C;
	Tool->appContext->platform->testPosA = sample->A;
}

struct ldiUiPositionInfo {
	ImVec2 screenStartPos;
	ImVec2 imgOffset;
	float imgScale;
};

void imageInspectorDrawCharucoResults(ldiUiPositionInfo uiInfo, ldiCharucoResults* Charucos, bool ShowRejectedMarkers, bool ShowRejectedBoards) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	{
		std::vector<ImVec2> markerCentroids;

		for (size_t i = 0; i < Charucos->markers.size(); ++i) {
			ImVec2 points[5];
			ImVec2 markerCentroid(0.0f, 0.0f);

			for (int j = 0; j < 4; ++j) {
				ImVec2 offset = toImVec2(Charucos->markers[i].corners[j]);
				points[j] = uiInfo.screenStartPos + (uiInfo.imgOffset + offset) * uiInfo.imgScale;
				markerCentroid += points[j];
			}
			points[4] = points[0];

			markerCentroid /= 4.0f;
			markerCentroids.push_back(markerCentroid);

			draw_list->AddPolyline(points, 5, ImColor(0, 0, 255), 0, 1.0f);
		}

		for (int i = 0; i < Charucos->markers.size(); ++i) {
			char strBuff[32];
			sprintf_s(strBuff, sizeof(strBuff), "%d", Charucos->markers[i].id);
			draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
		}
	}

	if (ShowRejectedMarkers) {
		for (size_t i = 0; i < Charucos->rejectedMarkers.size(); ++i) {
			ImVec2 points[5];

			for (int j = 0; j < 4; ++j) {
				ImVec2 offset = toImVec2(Charucos->rejectedMarkers[i].corners[j]);
				points[j] = uiInfo.screenStartPos + (uiInfo.imgOffset + offset) * uiInfo.imgScale;
			}
			points[4] = points[0];

			draw_list->AddPolyline(points, 5, ImColor(255, 0, 0), 0, 1.0f);
		}
	}

	{
		for (int b = 0; b < Charucos->boards.size(); ++b) {
			ImVec2 points[5];

			for (int j = 0; j < 4; ++j) {
				ImVec2 offset = toImVec2(Charucos->boards[b].outline[j]);
				points[j] = uiInfo.screenStartPos + (uiInfo.imgOffset + offset) * uiInfo.imgScale;
			}
			points[4] = points[0];

			float angle = Charucos->boards[b].charucoEstimatedBoardAngle;

			float lerp = 255 * angle;

			draw_list->AddPolyline(points, 5, ImColor((int)(255 - lerp), (int)lerp, 0, 128), 0, 8.0f);

			{
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + toImVec2(Charucos->boards[b].outline[0])) * uiInfo.imgScale;

				char strBuf[256];
				sprintf_s(strBuf, 256, "%.2f", angle);
				draw_list->AddText(offset, ImColor(200, 0, 200), strBuf);
			}
		}

		for (int b = 0; b < Charucos->boards.size(); ++b) {
			for (int i = 0; i < Charucos->boards[b].corners.size(); ++i) {
				ImVec2 cornerPos = toImVec2(Charucos->boards[b].corners[i].position);
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + cornerPos) * uiInfo.imgScale;
				int cornerId = Charucos->boards[b].corners[i].globalId;

				draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));

				char strBuf[256];
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, cornerPos.x, cornerPos.y);
				draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
			}
		}


		for (int b = 0; b < Charucos->boards.size(); ++b) {
			for (int i = 0; i < Charucos->boards[b].reprojectdCorners.size(); ++i) {
				ImVec2 cornerPos = toImVec2(Charucos->boards[b].reprojectdCorners[i]);
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + cornerPos) * uiInfo.imgScale;

				draw_list->AddCircle(offset, 2.0f, ImColor(255, 0, 0));
			}
		}
	}

	if (ShowRejectedBoards) {
		for (int b = 0; b < Charucos->rejectedBoards.size(); ++b) {
			ImVec2 points[5];

			if (Charucos->rejectedBoards[b].outline.empty()) {
				continue;
			}

			for (int j = 0; j < 4; ++j) {
				ImVec2 offset = toImVec2(Charucos->rejectedBoards[b].outline[j]);
				points[j] = uiInfo.screenStartPos + (uiInfo.imgOffset + offset) * uiInfo.imgScale;
			}
			points[4] = points[0];

			float angle = Charucos->rejectedBoards[b].charucoEstimatedBoardAngle;

			draw_list->AddPolyline(points, 5, ImColor(255, 0, 0, 128), 0, 8.0f);

			{
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + toImVec2(Charucos->rejectedBoards[b].outline[0])) * uiInfo.imgScale;

				char strBuf[256];
				sprintf_s(strBuf, 256, "%.2f", angle);
				draw_list->AddText(offset, ImColor(200, 0, 200), strBuf);
			}
		}

		for (int b = 0; b < Charucos->rejectedBoards.size(); ++b) {
			for (int i = 0; i < Charucos->rejectedBoards[b].corners.size(); ++i) {
				ImVec2 cornerPos = toImVec2(Charucos->rejectedBoards[b].corners[i].position);
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + cornerPos) * uiInfo.imgScale;
				int cornerId = Charucos->rejectedBoards[b].corners[i].globalId;

				draw_list->AddCircle(offset, 4.0f, ImColor(255, 255, 0));

				char strBuf[256];
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, cornerPos.x, cornerPos.y);

				draw_list->AddText(offset, ImColor(200, 200, 0), strBuf);
			}
		}

		for (int b = 0; b < Charucos->rejectedBoards.size(); ++b) {
			for (int i = 0; i < Charucos->rejectedBoards[b].reprojectdCorners.size(); ++i) {
				ImVec2 cornerPos = toImVec2(Charucos->rejectedBoards[b].reprojectdCorners[i]);
				ImVec2 offset = uiInfo.screenStartPos + (uiInfo.imgOffset + cornerPos) * uiInfo.imgScale;

				draw_list->AddCircle(offset, 2.0f, ImColor(255, 0, 0));
			}
		}
	}
}

void imageInspectorShowUi(ldiImageInspector* Tool) {
	ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;

	{
		ldiHawk* mvCam = &Tool->appContext->platform->hawk;

		hawkUpdateValues(mvCam);

		bool newFrame = false;
		ldiImage camImg = {};

		{
			std::unique_lock<std::mutex> lock(mvCam->valuesMutex);
			if (Tool->hawkLastFrameId != mvCam->latestFrameId) {
				Tool->hawkLastFrameId = mvCam->latestFrameId;

				for (int iY = 0; iY < mvCam->imgHeight; ++iY) {
					memcpy(Tool->hawkFrameBuffer + iY * CAM_IMG_WIDTH, mvCam->frameBuffer + iY * mvCam->imgWidth, mvCam->imgWidth);
				}

				newFrame = true;
				camImg.data = Tool->hawkFrameBuffer;
				camImg.width = mvCam->imgWidth;
				camImg.height = mvCam->imgHeight;
			}
		}

		if (newFrame) {
			if (Tool->hawkScanProcessImage) {
				Tool->hawkScanSegs.clear();
				Tool->hawkScanPoints = computerVisionFindScanLine(camImg);
				Tool->appContext->platform->liveScanPoints.resize(Tool->hawkScanPoints.size());

				// Scan plane at machine position.
				ldiHorsePosition horsePos = {};
				horsePos.x = Tool->appContext->platform->testPosX;
				horsePos.y = Tool->appContext->platform->testPosY;
				horsePos.z = Tool->appContext->platform->testPosZ;
				horsePos.c = Tool->appContext->platform->testPosC;
				horsePos.a = Tool->appContext->platform->testPosA;

				mat4 workTrans = horseGetWorkTransform(&calibContext->calibJob, horsePos);
				mat4 invWorkTrans = glm::inverse(workTrans);
				
				ldiPlane scanPlane = horseGetScanPlane(&calibContext->calibJob, horsePos);
				scanPlane.normal = -scanPlane.normal;
				
				// Project points onto scan plane.
				ldiCamera camera = horseGetCamera(&calibContext->calibJob, horsePos, 3280, 2464);

				for (size_t pIter = 0; pIter < Tool->hawkScanPoints.size(); ++pIter) {
					ldiLine ray = screenToRay(&camera, Tool->hawkScanPoints[pIter]);
					
					vec3 worldPoint;
					if (getRayPlaneIntersection(ray, scanPlane, worldPoint)) {
						//std::cout << "Ray failed " << pIter << "\n";

						// Bake point into work space.
						worldPoint = invWorkTrans * vec4(worldPoint, 1.0f);

						Tool->appContext->platform->liveScanPoints[pIter] = worldPoint;
						//Tool->appContext->platform->liveScanPoints[pIter] = ray.origin + ray.direction;
					}
				}

				Tool->appContext->platform->liveScanPointsUpdated = true;

				//Tool->appContext->platform->liveScanPoints[0] = scanPlane.point;
				//Tool->appContext->platform->liveScanPoints[1] = scanPlane.point + scanPlane.normal;
			}

			if (Tool->showUndistorted) {
				cv::Mat image(cv::Size(camImg.width, camImg.height), CV_8UC1, camImg.data);
				cv::Mat outputImage(cv::Size(camImg.width, camImg.height), CV_8UC1);

				cv::undistort(image, outputImage, calibContext->calibJob.camMat, calibContext->calibJob.camDist);

				ldiImage tempImg = camImg;
				tempImg.data = outputImage.data;
				gfxCopyToTexture2D(Tool->appContext, Tool->hawkTex, tempImg);
			} else {
				gfxCopyToTexture2D(Tool->appContext, Tool->hawkTex, camImg);
			}

			if (Tool->camCalibProcess) {
				ldiCameraCalibSample calibSample;

				if (cameraCalibFindCirclesBoard(Tool->appContext, camImg, &calibSample)) {
					double currentTime = getTime();
					if (currentTime - Tool->calibDetectTimeout >= 0.0) {
						std::cout << "Capture\n";
						Tool->calibDetectTimeout = currentTime + 1.0;

						ldiImage* newFrameImg = new ldiImage;
						newFrameImg->data = new uint8_t[camImg.width * camImg.height];
						newFrameImg->width = camImg.width;
						newFrameImg->height = camImg.height;
						memcpy(newFrameImg->data, camImg.data, camImg.width * camImg.height);

						calibSample.image = newFrameImg;
						
						Tool->cameraCalibSamples.push_back(calibSample);
						Tool->cameraCalibImageWidth = camImg.width;
						Tool->cameraCalibImageHeight = camImg.height;
					}
				}
			}

			if (Tool->camImageProcess) {
				// TODO: Temp default cam matrix.
				cv::Mat calibCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
				calibCameraMatrix.at<double>(0, 0) = 2666.92;
				calibCameraMatrix.at<double>(0, 1) = 0.0;
				calibCameraMatrix.at<double>(0, 2) = 1704.76;
				calibCameraMatrix.at<double>(1, 0) = 0.0;
				calibCameraMatrix.at<double>(1, 1) = 2683.62;
				calibCameraMatrix.at<double>(1, 2) = 1294.87;
				calibCameraMatrix.at<double>(2, 0) = 0.0;
				calibCameraMatrix.at<double>(2, 1) = 0.0;
				calibCameraMatrix.at<double>(2, 2) = 1.0;

				cv::Mat calibCameraDist = cv::Mat::zeros(8, 1, CV_64F);
				calibCameraDist.at<double>(0) = 0.1937769949436188;
				calibCameraDist.at<double>(1) = -0.3512980043888092;
				calibCameraDist.at<double>(2) = 0.002319999970495701;
				calibCameraDist.at<double>(3) = 0.00217368989251554;

				computerVisionFindCharuco(camImg, &Tool->camImageCharucoResults, &calibCameraMatrix, &calibCameraDist);
			}
		}

		ImGui::PushID("hawkView");
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (ImGui::Begin("Hawk View")) {
			ImVec2 viewSize = ImGui::GetContentRegionAvail();
			ImVec2 startPos = ImGui::GetCursorPos();
			ImVec2 screenStartPos = ImGui::GetCursorScreenPos();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			auto surfaceResult = uiViewportSurface2D("__captureSurface", &Tool->hawkImageScale, &Tool->hawkImageOffset);

			ImVec2 imgOffset = toImVec2(Tool->hawkImageOffset);
			float imgScale = Tool->hawkImageScale;

			ImVec2 imgMin;
			imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
			imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

			ImVec2 imgMax;
			imgMax.x = imgMin.x + CAM_IMG_WIDTH * imgScale;
			imgMax.y = imgMin.y + CAM_IMG_HEIGHT * imgScale;

			draw_list->AddCallback(_imageInspectorSetStateCallback, Tool);
			draw_list->AddImage(Tool->hawkResourceView, imgMin, imgMax);
			draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

			//----------------------------------------------------------------------------------------------------
			// 3D view overlay.
			//----------------------------------------------------------------------------------------------------
			_imageInspectorRenderHawkVolume(Tool, viewSize.x, viewSize.y, Tool->hawkImageOffset * imgScale, vec2((float)CAM_IMG_WIDTH, (float)CAM_IMG_HEIGHT) * imgScale);

			ImVec2 uv_min(0.0f, 0.0f);
			ImVec2 uv_max(1.0f, 1.0f);
			ImVec4 tint_col(1.0f, 1.0f, 1.0f, Tool->sceneOpacity);
			draw_list->AddImage(Tool->hawkViewBuffer.mainViewResourceView, screenStartPos, screenStartPos + viewSize, uv_min, uv_max, ImGui::GetColorU32(tint_col));

			draw_list->AddText(imgMin, ImColor(200, 200, 200), "Camera");

			//----------------------------------------------------------------------------------------------------
			// Draw charucos.
			//----------------------------------------------------------------------------------------------------
			if (Tool->showCharucoResults) {
				ldiCharucoResults* charucos = &Tool->camImageCharucoResults;

				if (Tool->imageMode == IIM_CALIBRATION_JOB && Tool->calibJobSelectedSampleId != -1 && Tool->calibJobSelectedSampleType == 0) {
					charucos = &Tool->appContext->calibrationContext->calibJob.samples[Tool->calibJobSelectedSampleId].cube;
				}

				ldiUiPositionInfo uiInfo = {};
				uiInfo.imgOffset = toImVec2(Tool->hawkImageOffset);
				uiInfo.imgScale = Tool->hawkImageScale;
				uiInfo.screenStartPos = screenStartPos;

				imageInspectorDrawCharucoResults(uiInfo, charucos, Tool->showCharucoRejectedMarkers, true);
			}

			//----------------------------------------------------------------------------------------------------
			// Draw reprojection errors.
			//----------------------------------------------------------------------------------------------------
			if (Tool->showReprojection) {
				if (Tool->appContext->calibrationContext->calibJob.metricsCalculated) {
					ldiCalibrationJob* job = &Tool->appContext->calibrationContext->calibJob;

					for (size_t i = 0; i < job->projObs.size(); ++i) {
						ImVec2 uiPos = screenStartPos + (imgOffset + toImVec2(job->projObs[i])) * imgScale;
						draw_list->AddCircle(uiPos, max(1, 0.4f * imgScale), ImColor(0, 0, 255));
					}

					for (size_t i = 0; i < job->projReproj.size(); ++i) {
						ImVec2 uiPos = screenStartPos + (imgOffset + toImVec2(job->projReproj[i])) * imgScale;

						float t = job->projError[i] * 0.5;


						draw_list->AddCircle(uiPos, max(1, 0.4f * imgScale), ImColor(lerp(0, 1, t), 0.0f, 0.0f));
					}
				}
			}

			//----------------------------------------------------------------------------------------------------
			// Draw scan line results.
			//----------------------------------------------------------------------------------------------------
			{
				if (Tool->imageMode == IIM_CALIBRATION_JOB && Tool->calibJobSelectedSampleId != -1 && Tool->calibJobSelectedSampleType == 1) {

					if (calibContext->calibJob.scanPoints.size() > 0) {
						std::vector<vec2>& points = calibContext->calibJob.scanPoints[Tool->calibJobSelectedSampleId];

						for (size_t i = 0; i < points.size(); ++i) {
							vec2 point = points[i];

							ImVec2 uiPos = screenStartPos + (imgOffset + ImVec2(point.x, point.y)) * imgScale;
							draw_list->AddCircle(uiPos, max(1, 0.4f * imgScale), ImColor(0, 255, 0));
						}
					}
				}
			}

			//----------------------------------------------------------------------------------------------------
			// Draw scanner debug info.
			//----------------------------------------------------------------------------------------------------
			if (Tool->hawkScanProcessImage) {
				for (size_t i = 0; i < Tool->hawkScanSegs.size(); ++i) {
					//draw_list->AddCircle(offset, 40.0f * Tool->imgScale, ImColor(200, 0, 200));
					vec4 point = Tool->hawkScanSegs[i];

					ImVec2 uiPos = screenStartPos + (imgOffset + ImVec2(point.x + 0.5f, point.y + 0.5f)) * imgScale;
					draw_list->AddCircle(uiPos, max(1, 0.4f * imgScale), ImColor(255, 0, 0));

					uiPos = screenStartPos + (imgOffset + ImVec2(point.z + 0.5f, point.w + 0.5f)) * imgScale;
					draw_list->AddCircle(uiPos, max(1, 0.2f * imgScale), ImColor(0, 0, 255));
				}

				for (size_t i = 0; i < Tool->hawkScanPoints.size(); ++i) {
					vec2 point = Tool->hawkScanPoints[i];

					ImVec2 uiPos = screenStartPos + (imgOffset + ImVec2(point.x, point.y)) * imgScale;
					draw_list->AddCircle(uiPos, max(1, 0.4f * imgScale), ImColor(0, 255, 0));
				}
			}

			//----------------------------------------------------------------------------------------------------
			// Draw cursor.
			//----------------------------------------------------------------------------------------------------
			if (surfaceResult.isHovered) {
				vec2 pixelPos;
				pixelPos.x = (int)surfaceResult.worldPos.x;
				pixelPos.y = (int)surfaceResult.worldPos.y;

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

				//ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				//ImGui::Text("%.3f %.3f - %.3f", tool->surfelViewImgOffset.x, tool->surfelViewImgOffset.y, tool->surfelViewScale);

				if (surfaceResult.isHovered) {
					ImGui::Text("%.3f %.3f", surfaceResult.worldPos.x, surfaceResult.worldPos.y);
				}

				ImGui::EndChild();
			}
		}
		ImGui::PopStyleVar();
		ImGui::End();
		ImGui::PopID();
	}

	//----------------------------------------------------------------------------------------------------
	// Rotary measurement.
	//----------------------------------------------------------------------------------------------------
	if (rotaryMeasurementProcess(&Tool->rotaryResults)) {
		gfxCopyToTexture2D(Tool->appContext, Tool->camTex, { 640, 480, Tool->rotaryResults.greyFrame });
	}

	//----------------------------------------------------------------------------------------------------
	// Test camera.
	//----------------------------------------------------------------------------------------------------
	{
		ldiHawk* mvCam = &Tool->testCam;

		hawkUpdateValues(mvCam);

		bool newFrame = false;
		ldiImage camImg = {};

		{
			std::unique_lock<std::mutex> lock(mvCam->valuesMutex);
			if (Tool->testCamLastFrameId != mvCam->latestFrameId) {
				for (int iY = 0; iY < mvCam->imgHeight; ++iY) {
					memcpy(Tool->testCamFrameBuffer + iY * TEST_CAM_WIDTH, mvCam->frameBuffer + iY * mvCam->imgWidth, mvCam->imgWidth);
				}

				/*for (int p = 0; p < mvCam->imgWidth * mvCam->imgHeight; ++p) {
					Tool->testCamFrameBufferFilter[p] = Tool->testCamFrameBufferFilter[p] * 0.5f + mvCam->frameBuffer[p] * 0.5f;
					Tool->testCamFrameBuffer[p] = (uint8_t)(Tool->testCamFrameBufferFilter[p] + 0.5f);
				}*/

				/*for (int p = 0; p < mvCam->imgWidth * mvCam->imgHeight; ++p) {
					float lin = GammaToLinear(mvCam->frameBuffer[p] / 256.0f);
					Tool->testCamFrameBuffer[p] = (uint8_t)(lin * 255.0f);
				}*/

				newFrame = true;
				camImg.data = Tool->testCamFrameBuffer;
				camImg.width = mvCam->imgWidth;
				camImg.height = mvCam->imgHeight;

				// Search image
				cv::Mat image(cv::Size(camImg.width, camImg.height), CV_8UC1, camImg.data);

				cv::SimpleBlobDetector::Params params = cv::SimpleBlobDetector::Params();
				params.blobColor = 255;
				//std::cout << params.minArea << " " << params.maxArea << "\n";
				//params.minArea = 70;
				//params.maxArea = 700;

				cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
				detector->detect(image, Tool->laserProfileBlobs);

				//std::cout << "Blobs: " << Tool->laserProfileBlobs.size() << "\n";
			}
		}

		if (newFrame) {
			gfxCopyToTexture2D(Tool->appContext, Tool->camTex, camImg);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Volume calibration.
	//----------------------------------------------------------------------------------------------------
	if (ImGui::Begin("Calibration")) {
		ldiCalibrationJob* job = &calibContext->calibJob;

		if (ImGui::Button("Load job")) {
			Tool->imageMode = IIM_CALIBRATION_JOB;
			_imageInspectorSelectCalibJob(Tool, -1, -1);

			std::string filePath;
			if (showOpenFileDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, filePath, L"Calibration file", L"*.cal")) {
				calibLoadCalibJob(filePath, job);
			}
		}

		if (ImGui::Button("Save job")) {
			std::string filePath;
			if (showSaveFileDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, filePath, L"Calibration file", L"*.cal", L"cal")) {
				calibSaveCalibJob(filePath, job);
			}
		}

		ImGui::Separator();
		ImGui::Text("Tools");

		//if (ImGui::Button("Split stereo samples")) {
		//	//calibSplitStereoSamplesJob(job);
		//	calibSplitStereoSamplesScanner();
		//}

		if (ImGui::Button("Compare calibrations")) {
			calibCompareVolumeCalibrations("../cache/calib_hawk0_optimized.cal", "../cache/calib_hawk1_optimized.cal");
		}

		ImGui::Separator();
		ImGui::Text("Volume");

		if (ImGui::Button("Initial observations")) {
			Tool->imageMode = IIM_CALIBRATION_JOB;
			_imageInspectorSelectCalibJob(Tool, -1, -1);

			std::string directoryPath;
			if (showOpenDirectoryDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, directoryPath)) {
				calibFindInitialObservations(job, directoryPath);
			}
		}

		if (ImGui::Button("Initial estimations")) {
			calibGetInitialEstimations(job);
		}

		if (ImGui::Button("Optimize")) {
			calibOptimizeVolume(job);
		}

		ImGui::Separator();
		ImGui::Text("Scanner");

		if (ImGui::Button("Calibrate scanner")) {
			_imageInspectorSelectCalibJob(Tool, -1, -1);

			std::string directoryPath;
			if (showOpenDirectoryDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, directoryPath)) {
				calibCalibrateScanner(job, directoryPath);
			}
		}

		ImGui::Separator();
		ImGui::Text("Galvo");

		if (ImGui::Button("Calibrate galvo")) {
		}

		ImGui::Separator();

		ImGui::Text("Initial estimations: %s", job->initialEstimations ? "Yes" : "No");
		ImGui::Text("Metrics calibrated: %s", job->metricsCalculated ? "Yes" : "No");
		ImGui::Text("Scanner calibrated: %s", job->scannerCalibrated ? "Yes" : "No");
		ImGui::Text("Galvo calibrated: %s", job->galvoCalibrated ? "Yes" : "No");
		ImGui::Text("Volume calibration samples: %d", job->samples.size());

		if (ImGui::CollapsingHeader("Volume calibration samples")) {
			if (ImGui::BeginTable("table_volume_samples", 8, ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("Phase", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Boards", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableHeadersRow();

				for (size_t sampleIter = 0; sampleIter < job->samples.size(); ++sampleIter) {
					ldiCalibSample* sample = &job->samples[sampleIter];

					ImGui::TableNextRow();

					ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;

					ImGui::TableSetColumnIndex(0);

					bool selected = (Tool->calibJobSelectedSampleId == sampleIter) && (Tool->calibJobSelectedSampleType == 0);
					if (ImGui::Selectable(std::to_string(sampleIter).c_str(), selected, selectable_flags, ImVec2(0, 0))) {
						Tool->imageMode = IIM_CALIBRATION_JOB;
						_imageInspectorSelectCalibJob(Tool, sampleIter, 0);
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", sample->phase);

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%d", sample->X);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", sample->Y);

					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%d", sample->Z);

					ImGui::TableSetColumnIndex(5);
					ImGui::Text("%d", sample->A);

					ImGui::TableSetColumnIndex(6);
					ImGui::Text("%d", sample->C);

					ImGui::TableSetColumnIndex(7);
					ImGui::Text("%d - %d", sample->cube.boards.size(), sample->cube.rejectedBoards.size());
				}
				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Scanner calibration samples")) {
			if (ImGui::BeginTable("table_scanner_samples", 7, ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("Phase", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("C", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableHeadersRow();

				for (size_t sampleIter = 0; sampleIter < job->scanSamples.size(); ++sampleIter) {
					ldiCalibSample* sample = &job->scanSamples[sampleIter];

					ImGui::TableNextRow();

					ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;

					ImGui::TableSetColumnIndex(0);
					
					bool selected = (Tool->calibJobSelectedSampleId == sampleIter) && (Tool->calibJobSelectedSampleType == 1);
					if (ImGui::Selectable(std::to_string(sampleIter).c_str(), selected, selectable_flags, ImVec2(0, 0))) {
						Tool->imageMode = IIM_CALIBRATION_JOB;
						_imageInspectorSelectCalibJob(Tool, sampleIter, 1);
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", sample->phase);

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%d", sample->X);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", sample->Y);

					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%d", sample->Z);

					ImGui::TableSetColumnIndex(5);
					ImGui::Text("%d", sample->A);

					ImGui::TableSetColumnIndex(6);
					ImGui::Text("%d", sample->C);
				}
				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Galvo calibration samples")) {

		}
	}
	ImGui::End();

	//----------------------------------------------------------------------------------------------------
	// Image inspector.
	//----------------------------------------------------------------------------------------------------
	if (ImGui::Begin("Image inspector controls")) {
		if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SliderFloat("Scale", &Tool->imgScale, 0.1f, 10.0f);
			ImGui::Text("Cursor position: %.2f, %.2f", Tool->camImageCursorPos.x, Tool->camImageCursorPos.y);
			ImGui::Text("Cursor value: %d", Tool->camImagePixelValue);
			ImGui::Separator();
			ImGui::Checkbox("Show charuco results", &Tool->showCharucoResults);
			ImGui::Checkbox("Show charuco rejected markers", &Tool->showCharucoRejectedMarkers);
			
			if (ImGui::Checkbox("Show undistorted", &Tool->showUndistorted)) {
				_imageInspectorSelectCalibJob(Tool, Tool->calibJobSelectedSampleId, Tool->calibJobSelectedSampleType);
			}

			ImGui::Checkbox("Show reprojection", &Tool->showReprojection);

			ImGui::SliderFloat("Scene opacity", &Tool->sceneOpacity, 0.0f, 1.0f);

			ImGui::Checkbox("Show heatmap", &Tool->showHeatmap);

			ImGui::DragFloat("Brightness", &Tool->imageBrightness, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat("Contrast", &Tool->imageContrast, 0.1f, 0.0f, 20.0f);

			if (ImGui::Button("Reset")) {
				Tool->imageBrightness = 0.0f;
				Tool->imageContrast = 1.0f;
			}
		}

		if (ImGui::CollapsingHeader("Rotary measurement")) {
			ImGui::Checkbox("Show gizmos", &Tool->rotaryResults.showGizmos);

			ImGui::PushFont(Tool->appContext->fontBig);
			if (Tool->rotaryResults.stdDev > Tool->rotaryResults.stdDevStoppedLimit) {
				ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "Rotating");
			} else {
				ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Stopped");
			}
			ImGui::PopFont();

			if (ImGui::Button("Zero")) {
				for (int i = 0; i < 16; ++i) {
					Tool->rotaryResults.accumPoints[i].zeroAngle = Tool->rotaryResults.accumPoints[i].info;
				}
			}

			ImGui::Text("Angle: %.3f", Tool->rotaryResults.relativeAngleToZero);
			ImGui::InputFloat("Stopped limit: ", &Tool->rotaryResults.stdDevStoppedLimit);
			ImGui::Text("Std dev: %.3f", Tool->rotaryResults.stdDev);
			ImGui::Text("Process time: %.3f ms", Tool->rotaryResults.processTime);
		}

		{
			ldiHawk* mvCam = &Tool->appContext->platform->hawk;

			ImGui::PushID(0);

			char tempBuffer[256];
			sprintf_s(tempBuffer, "Camera (Hawk)");

			if (ImGui::CollapsingHeader(tempBuffer)) {
				if (ImGui::SliderInt("Shutter speed", &mvCam->uiShutterSpeed, 1, 66000)) {
					hawkCommitValues(mvCam);
				}

				if (ImGui::SliderInt("Analog gain", &mvCam->uiAnalogGain, 0, 100)) {
					hawkCommitValues(mvCam);
				}

				ImGui::SliderFloat("Filter factor", &Tool->camImageFilterFactor, 0.0f, 1.0f);

				if (ImGui::Button("Start continuous mode")) {
					Tool->imageMode = IIM_LIVE_CAMERA;
					hawkSetMode(mvCam, CCM_CONTINUOUS);
				}

				if (ImGui::Button("Stop continuous mode")) {
					Tool->imageMode = IIM_LIVE_CAMERA;
					hawkSetMode(mvCam, CCM_WAIT);
				}

				if (ImGui::Button("Get single image")) {
					Tool->imageMode = IIM_LIVE_CAMERA;
					hawkSetMode(mvCam, CCM_SINGLE);
				}

				if (ImGui::Button("Get average image")) {
					Tool->imageMode = IIM_LIVE_CAMERA;
					hawkSetMode(mvCam, CCM_AVERAGE);
				}

				if (ImGui::Button("Get average (no flash)")) {
					Tool->imageMode = IIM_LIVE_CAMERA;
					hawkSetMode(mvCam, CCM_AVERAGE_NO_FLASH);
				}

				if (Tool->camCalibProcess) {
					if (ImGui::Button("Stop calibration")) {
						Tool->camCalibProcess = false;
					}
				} else {
					if (ImGui::Button("Start calibration")) {
						Tool->camCalibProcess = true;
						Tool->camImageProcess = false;
						Tool->cameraCalibSamples.clear();
					}
				}
			
				if (ImGui::Button("Save calibration")) {
					std::string dataPath = "../cache/calibImages.dat";

					FILE* file;
					fopen_s(&file, dataPath.c_str(), "wb");

					int sampleCount = Tool->cameraCalibSamples.size();
					fwrite(&sampleCount, sizeof(int), 1, file);

					for (int i = 0; i < sampleCount; ++i) {
						ldiCameraCalibSample* sample = &Tool->cameraCalibSamples[i];
						ldiImage* img = sample->image;

						fwrite(&img->width, sizeof(int), 1, file);
						fwrite(&img->height, sizeof(int), 1, file);
						fwrite(img->data, 1, img->width * img->height, file);

						int pointCount = sample->imagePoints.size();
						fwrite(&pointCount, sizeof(int), 1, file);

						for (int j = 0; j < pointCount; ++j) {
							vec2 point = sample->imagePoints[j];
							fwrite(&point, sizeof(vec2), 1, file);
						}
					}

					fclose(file);
				}

				if (ImGui::Button("Load calibration")) {
					// TODO: Clear memory allocated.
					Tool->cameraCalibSamples.clear();

					std::string dataPath = "../cache/calibImages.dat";

					FILE* file;
					fopen_s(&file, dataPath.c_str(), "rb");

					int sampleCount = 0;
					fread(&sampleCount, sizeof(int), 1, file);

					for (int i = 0; i < sampleCount; ++i) {
						int width = 0;
						int height = 0;

						fread(&width, sizeof(int), 1, file);
						fread(&height, sizeof(int), 1, file);

						ldiImage* newFrameImg = new ldiImage;
						newFrameImg->data = new uint8_t[width * height];
						newFrameImg->width = width;
						newFrameImg->height = height;

						fread(newFrameImg->data, 1, width * height, file);

						ldiCameraCalibSample calibSample = {};
						calibSample.image = newFrameImg;

						int pointCount = 0;
						fread(&pointCount, sizeof(int), 1, file);

						for (int j = 0; j < pointCount; ++j) {
							vec2 point;
							fread(&point, sizeof(vec2), 1, file);
							calibSample.imagePoints.push_back(point);
						}

						Tool->cameraCalibSamples.push_back(calibSample);
						Tool->cameraCalibImageWidth = width;
						Tool->cameraCalibImageHeight = height;
					}

					fclose(file);
				}

				if (ImGui::Button("Calibrate")) {
					Tool->camCalibProcess = false;
					cameraCalibRunCalibrationCircles(Tool->appContext, Tool->cameraCalibSamples, Tool->cameraCalibImageWidth, Tool->cameraCalibImageHeight, &Tool->cameraCalibMatrix, &Tool->cameraCalibDist);
				}

				if (ImGui::Checkbox("Undistorted", &Tool->cameraCalibShowUndistorted)) {
					std::cout << "Undistorted: " << Tool->cameraCalibShowUndistorted << "\n";
				}
			}

			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Machine vision")) {
			ImGui::Checkbox("Process", &Tool->camImageProcess);
			ImGui::Checkbox("Process scan mapping", &Tool->hawkScanProcessImage);
			ImGui::Checkbox("Scan gamma test", &Tool->hawkEnableGammaTest);
			ImGui::DragInt("Scan mapping min", &Tool->hawkScanMappingMin, 0.4f, 0, 255);
			ImGui::DragInt("Scan mapping max", &Tool->hawkScanMappingMax, 0.4f, 0, 255);

			if (Tool->hawkScanMappingMax < Tool->hawkScanMappingMin) {
				Tool->hawkScanMappingMax = Tool->hawkScanMappingMin;
			}

		}

		if (ImGui::CollapsingHeader("Camera calibration")) {
			ImGui::Text("Samples");
			if (ImGui::BeginListBox("##listbox_camera_samples", ImVec2(-FLT_MIN, -FLT_MIN))) {
				{
					bool isSelected = (Tool->cameraCalibSelectedSample == -1);

					if (ImGui::Selectable("None", isSelected)) {
						Tool->cameraCalibSelectedSample = -1;
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
			
				for (size_t n = 0; n < Tool->cameraCalibSamples.size(); ++n) {
					bool isSelected = (Tool->cameraCalibSelectedSample == n);

					char itemLabel[256];
					sprintf_s(itemLabel, "Sample %d", n);

					if (ImGui::Selectable(itemLabel, isSelected)) {
						Tool->cameraCalibSelectedSample = n;
						ldiImage* calibImg = Tool->cameraCalibSamples[n].image;

						if (Tool->cameraCalibShowUndistorted) {
							cv::Mat image(cv::Size(calibImg->width, calibImg->height), CV_8UC1, calibImg->data);
							cv::Mat outputImage(cv::Size(calibImg->width, calibImg->height), CV_8UC1);
							cv::undistort(image, outputImage, Tool->cameraCalibMatrix, Tool->cameraCalibDist);
						
							gfxCopyToTexture2D(Tool->appContext, Tool->camTex, { calibImg->width, calibImg->height, outputImage.data });
						} else {
							gfxCopyToTexture2D(Tool->appContext, Tool->camTex, *calibImg);
						}

						//_imageInspectorRenderHawkCalibration(Tool);
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
		}
	}
	ImGui::End();

	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		auto surfaceResult = uiViewportSurface2D("__imgInspecViewButton", &Tool->imgScale, &Tool->imgOffset);

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(0.5f, 0.5f, 0.5f, 0.0f); // 50% opaque white
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 imgMin;
		imgMin.x = screenStartPos.x + Tool->imgOffset.x * Tool->imgScale;
		imgMin.y = screenStartPos.y + Tool->imgOffset.y * Tool->imgScale;

		ImVec2 imgMax;
		imgMax.x = imgMin.x + CAM_IMG_WIDTH * Tool->imgScale;
		imgMax.y = imgMin.y + CAM_IMG_HEIGHT * Tool->imgScale;

		draw_list->AddCallback(_imageInspectorSetStateCallback, Tool);
		draw_list->AddImage(Tool->camResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

		uv_min = ImVec2(0.0f, 0.0f);
		uv_max = ImVec2(1.0f, 1.0f);
		tint_col = ImVec4(1.0f, 1.0f, 1.0f, Tool->sceneOpacity);
		//draw_list->AddCallback(_imageInspectorSetStateCallback2, Tool);

		//----------------------------------------------------------------------------------------------------
		// Draw webcam results.
		//----------------------------------------------------------------------------------------------------
		if (Tool->rotaryResults.showGizmos) {
			ldiRotaryResults* rotary = &Tool->rotaryResults;
			
			{
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imgOffset.x + 320) * Tool->imgScale;
				offset.y = screenStartPos.y + (Tool->imgOffset.y + 240) * Tool->imgScale;

				// Center tracking.
				draw_list->AddCircle(offset, 40.0f * Tool->imgScale, ImColor(200, 0, 200));

				// Inner track.
				//draw_list->AddCircle(offset, 180.0f * Tool->imgScale, ImColor(200, 0, 200));

				// Outer track.
				draw_list->AddCircle(offset, 260.0f * Tool->imgScale, ImColor(200, 0, 200));
			}

			for (int i = 0; i < rotary->blobs.size(); ++i) {
				vec2 k = rotary->blobs[i];

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imgOffset.x + k.x) * Tool->imgScale;
				offset.y = screenStartPos.y + (Tool->imgOffset.y + k.y) * Tool->imgScale;

				draw_list->AddCircle(offset, 2.0f, ImColor(0, 0, 255));

				/*char strBuf[256];
				sprintf_s(strBuf, 256, "%.2f, %.2f : %.3f", k->pt.x, k->pt.y, k->size);
				draw_list->AddText(offset, ImColor(0, 175, 0), strBuf);*/
			}

			if (rotary->foundCenter && rotary->foundLocator) {
				{
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + rotary->centerPos.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + rotary->centerPos.y) * Tool->imgScale;
					draw_list->AddCircle(offset, 4.0f * Tool->imgScale, ImColor(0, 255, 0));
				}

				{
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + rotary->locatorPos.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + rotary->locatorPos.y) * Tool->imgScale;
					draw_list->AddCircle(offset, 4.0f * Tool->imgScale, ImColor(255, 255, 0));
				}
				
				
				//for (int i = 0; i < rotary->points.size(); ++i) {
				//	ldiRotaryPoint* k = &rotary->points[i];

				//	// NOTE: Half pixel offset required.
				//	ImVec2 offset = pos;
				//	offset.x = screenStartPos.x + (Tool->imgOffset.x + k->pos.x) * Tool->imgScale;
				//	offset.y = screenStartPos.y + (Tool->imgOffset.y + k->pos.y) * Tool->imgScale;

				//	draw_list->AddCircle(offset, 4.0f, ImColor(255, 0, 0));

				//	char strBuf[256];
				//	//sprintf_s(strBuf, 256, "%d %.2f, %.2f : %f", k->id, k->pos.x, k->pos.y, k->info);
				//	sprintf_s(strBuf, 256, "%d %.3f", k->id, k->info);
				//	draw_list->AddText(offset, ImColor(0, 255, 0), strBuf);
				//}

				for (int i = 0; i < 32; ++i) {
					ldiRotaryPoint* k = &rotary->accumPoints[i];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + k->pos.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + k->pos.y) * Tool->imgScale;

					if (i < 16) {
						draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));

						char strBuf[256];
						//sprintf_s(strBuf, 256, "%d %.2f, %.2f : %f", k->id, k->pos.x, k->pos.y, k->info);
						sprintf_s(strBuf, 256, "%d %.3f %.3f", k->id, k->info, k->relAngle);
						draw_list->AddText(offset, ImColor(0, 255, 0), strBuf);
					} else {
						draw_list->AddCircle(offset, 4.0f, ImColor(255, 0, 0));

						char strBuf[256];
						sprintf_s(strBuf, 256, "%d", k->id);
						draw_list->AddText(offset, ImColor(255, 0, 0), strBuf);
					}
				}

				for (int i = 0; i < 16; ++i) {
					ldiRotaryPoint* a = &rotary->accumPoints[i];
					ldiRotaryPoint* b = &rotary->accumPoints[i + 16];

					ImVec2 aPos;
					aPos.x = screenStartPos.x + (Tool->imgOffset.x + a->pos.x) * Tool->imgScale;
					aPos.y = screenStartPos.y + (Tool->imgOffset.y + a->pos.y) * Tool->imgScale;

					ImVec2 bPos;
					bPos.x = screenStartPos.x + (Tool->imgOffset.x + b->pos.x) * Tool->imgScale;
					bPos.y = screenStartPos.y + (Tool->imgOffset.y + b->pos.y) * Tool->imgScale;

					draw_list->AddLine(aPos, bPos, ImColor(0, 0, 0 + (255 / 16) * i));
				}

			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw circle calibration board results.
		//----------------------------------------------------------------------------------------------------
		{
			srand(0);

			for (size_t iterSamples = 0; iterSamples < Tool->cameraCalibSamples.size(); ++iterSamples) {
				ldiCameraCalibSample* sample = &Tool->cameraCalibSamples[iterSamples];

				if (Tool->cameraCalibSelectedSample != -1 && Tool->cameraCalibSelectedSample != iterSamples) {
					continue;
				}

				vec3 rndCol = getRandomColorHighSaturation();
				
				for (size_t iterPoints = 0; iterPoints < sample->imagePoints.size(); ++iterPoints) {
					vec2 o = sample->imagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + o.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + o.y) * Tool->imgScale;
				
					draw_list->AddCircle(offset, 4.0f, ImColor(rndCol.x, rndCol.y, rndCol.z));
				}

				if (Tool->cameraCalibSelectedSample != iterSamples) {
					continue;
				}

				for (size_t iterPoints = 0; iterPoints < sample->undistortedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->undistortedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + o.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + o.y) * Tool->imgScale;

					draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
				}

				for (size_t iterPoints = 0; iterPoints < sample->reprojectedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->reprojectedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (Tool->imgOffset.x + o.x) * Tool->imgScale;
					offset.y = screenStartPos.y + (Tool->imgOffset.y + o.y) * Tool->imgScale;

					draw_list->AddCircle(offset, 2.0f, ImColor(2, 117, 247));
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw laser beam profiler results.
		//----------------------------------------------------------------------------------------------------
		if (Tool->showHeatmap) {
			for (size_t iterSamples = 0; iterSamples < Tool->laserProfileBlobs.size(); ++iterSamples) {
				cv::KeyPoint kp = Tool->laserProfileBlobs[iterSamples];

				cv::Point2f o = kp.pt;

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (Tool->imgOffset.x + o.x) * Tool->imgScale;
				offset.y = screenStartPos.y + (Tool->imgOffset.y + o.y) * Tool->imgScale;

				draw_list->AddCircle(offset, kp.size / 2.0f * Tool->imgScale, ImColor(255, 0, 0));

				draw_list->AddCircle(offset, 2.0f, ImColor(255, 0, 0));
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw cursor.
		//----------------------------------------------------------------------------------------------------
		if (surfaceResult.isHovered) {
			vec2 pixelPos;
			pixelPos.x = (int)surfaceResult.worldPos.x;
			pixelPos.y = (int)surfaceResult.worldPos.y;

			Tool->camImageCursorPos = surfaceResult.worldPos;

			if (pixelPos.x >= 0 && pixelPos.x < CAM_IMG_WIDTH) {
				if (pixelPos.y >= 0 && pixelPos.y < CAM_IMG_HEIGHT) {
					//Tool->camImagePixelValue = Tool->camPixelsFinal[(int)pixelPos.y * CAM_IMG_WIDTH + (int)pixelPos.x];
				}
			}

			ImVec2 rMin;
			rMin.x = screenStartPos.x + (Tool->imgOffset.x + pixelPos.x) * Tool->imgScale;
			rMin.y = screenStartPos.y + (Tool->imgOffset.y + pixelPos.y) * Tool->imgScale;

			ImVec2 rMax = rMin;
			rMax.x += Tool->imgScale;
			rMax.y += Tool->imgScale;

			draw_list->AddRect(rMin, rMax, ImColor(255, 0, 255));
		}

		// Viewport overlay.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_imageInspectorOverlay", ImVec2(200, 70), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			//ImGui::Text("%.3f %.3f - %.3f", tool->surfelViewImgOffset.x, tool->surfelViewImgOffset.y, tool->surfelViewScale);

			if (surfaceResult.isHovered) {
				ImGui::Text("%.3f %.3f", surfaceResult.worldPos.x, surfaceResult.worldPos.y);
			}

			ImGui::EndChild();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}
