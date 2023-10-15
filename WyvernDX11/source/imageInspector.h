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

#include "rotaryMeasurement.h"

enum ldiImageInspectorMode {
	IIM_LIVE_CAMERA,
	IIM_CALIBRATION_JOB
};

struct ldiImageInspector {
	ldiApp*						appContext;

	ldiImageInspectorMode		imageMode = IIM_LIVE_CAMERA;

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

	int							calibJobSelectedSampleId;
	int							calibCamSelectedId;
	uint8_t						calibJobImage[CAM_IMG_WIDTH * CAM_IMG_HEIGHT];

	ldiRotaryResults			rotaryResults;

	std::vector<ldiCameraCalibSample> cameraCalibSamples;
	int							cameraCalibImageWidth;
	int							cameraCalibImageHeight;
	int							cameraCalibSelectedSample = -1;
	cv::Mat						cameraCalibMatrix;
	cv::Mat						cameraCalibDist;
	bool						cameraCalibShowUndistorted = false;
	bool						cameraCalibShowUndistortedBundled = false;

	bool						showCharucoResults = false;

	ldiRenderViewBuffers		hawkViewBuffer;
	int							hawkViewWidth;
	int							hawkViewHeight;
	ldiCamera					hawkViewCamera;
	int							hawkLastFrameId[2] = { 0, 0 };
	uint8_t						hawkFrameBuffer[2][CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};
};

void _imageInspectorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;
	ldiApp* appContext = tool->appContext;

	//AddDrawCmd ??
	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
	appContext->d3dDeviceContext->PSSetShader(appContext->imgCamPixelShader, NULL, 0);
	appContext->d3dDeviceContext->VSSetShader(appContext->imgCamVertexShader, NULL, 0);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(tool->camImagePixelConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCamImagePixelConstants* constantBuffer = (ldiCamImagePixelConstants*)ms.pData;
		constantBuffer->params = vec4(1.0f, 1.0f, 1.0f, 0);
		appContext->d3dDeviceContext->Unmap(tool->camImagePixelConstants, 0);
	}

	appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &tool->camImagePixelConstants);
}

void _imageInspectorSetStateCallback2(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiImageInspector* tool = (ldiImageInspector*)cmd->UserCallbackData;
	ldiApp* appContext = tool->appContext;

	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
}

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
	// Prime camera texture.
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
	Tool->hawkViewWidth = 3280;
	Tool->hawkViewHeight = 2464;
	gfxCreateRenderView(AppContext, &Tool->hawkViewBuffer, Tool->hawkViewWidth, Tool->hawkViewHeight);

	return 0;
}

void _imageInspectorRenderHawk(ldiImageInspector* Tool) {
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
		ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;
		std::vector<vec3>* modelPoints = &calibContext->bundleAdjustResult.modelPoints;

		for (size_t i = 0; i < modelPoints->size(); ++i) {
			vec3 point = (*modelPoints)[i];
			pushDebugSphere(&appContext->defaultDebug, point, 0.5, vec3(1, 1, 1), 32);
		}

		ldiCalibTargetInfo targetInfo = calibContext->bundleAdjustResult.targetInfo;

		ldiPlane targetPlane = calibContext->bundleAdjustResult.targetInfo.plane;
		//pushDebugPlane(&appContext->defaultDebug, targetPlane.point, targetPlane.normal, 15, vec3(1, 0, 0));
		pushDebugLine(&appContext->defaultDebug, targetPlane.point, targetPlane.point + targetInfo.basisX * 10.0f, vec3(1, 0, 0));
		pushDebugLine(&appContext->defaultDebug, targetPlane.point, targetPlane.point + targetInfo.basisY * 10.0f, vec3(0, 1, 0));
		pushDebugLine(&appContext->defaultDebug, targetPlane.point, targetPlane.point + targetInfo.basisZ, vec3(0, 0, 1));

		pushDebugLine(&appContext->defaultDebug, targetPlane.point, targetPlane.point + targetInfo.basisXortho * 10.0f, vec3(0.5, 0, 0));
		pushDebugLine(&appContext->defaultDebug, targetPlane.point, targetPlane.point + targetInfo.basisYortho * 10.0f, vec3(0, 0.5, 0));

		for (size_t i = 0; i < calibContext->bundleAdjustResult.targetInfo.fits.size(); ++i) {
			ldiLineSegment line = calibContext->bundleAdjustResult.targetInfo.fits[i];

			pushDebugLine(&appContext->defaultDebug, line.a, line.b, vec3(0.1, 0.1, 0.1));
		}

		float widthH = (1.5 * 9) / 2.0;
		float heightH = (1.5 * 12) / 2.0;

		vec3 c0 = targetInfo.basisX * -widthH + targetInfo.basisY * -heightH + targetPlane.point;
		vec3 c1 = targetInfo.basisX * widthH + targetInfo.basisY * -heightH + targetPlane.point;
		vec3 c2 = targetInfo.basisX * widthH + targetInfo.basisY * heightH + targetPlane.point;
		vec3 c3 = targetInfo.basisX * -widthH + targetInfo.basisY * heightH + targetPlane.point;

		pushDebugLine(&appContext->defaultDebug, c0, c1, vec3(1.0, 0.0, 0.0));
		pushDebugLine(&appContext->defaultDebug, c1, c2, vec3(1.0, 0.0, 0.0));
		pushDebugLine(&appContext->defaultDebug, c2, c3, vec3(1.0, 0.0, 0.0));
		pushDebugLine(&appContext->defaultDebug, c3, c0, vec3(1.0, 0.0, 0.0));
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
	ldiApp* appContext = Tool->appContext;

	if (Tool->hawkViewWidth != Width || Tool->hawkViewHeight != Height) {
		Tool->hawkViewWidth = Width;
		Tool->hawkViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->hawkViewBuffer, Width, Height);
	}

	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->hawkViewBuffer.mainViewRtView, Tool->hawkViewBuffer.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	/*viewport.TopLeftX = ViewPortTopLeft.x;
	viewport.TopLeftY = ViewPortTopLeft.y;
	viewport.Width = ViewPortSize.x;
	viewport.Height = ViewPortSize.y;*/
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
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
	ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;
	ldiCalibrationJob* job = &calibContext->calibJob;

	if (job->samples.size() != 0) {
		std::vector<vec3>* modelPoints = &job->massModelTriangulatedPointsBundleAdjust;

		for (size_t i = 0; i < modelPoints->size(); ++i) {
			vec3 point = (*modelPoints)[i];// * 10.0f;

			int id = calibContext->calibJob.massModelBundleAdjustPointIds[i];
			int sampleId = id / (9 * 6);
			int boardId = (id % (9 * 6)) / 9;
			int pointId = id % (9);

			vec3 col(0, 0.5, 0);
			srand(sampleId);
			rand();
			col = getRandomColorHighSaturation();

			pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);

			//displayTextAtPoint(&Tool->mainCamera, point, std::to_string(sampleId), vec4(1, 1, 1, 0.5), TextBuffer);
		}

		//vec3 offset = glm::f64vec3(job->samples[Tool->calibJobSelectedSampleId].X, job->samples[Tool->calibJobSelectedSampleId].Y, job->samples[Tool->calibJobSelectedSampleId].Z) * job->stepsToCm;
		//offset -= glm::f64vec3(job->samples[0].X, job->samples[0].Y, job->samples[0].Z) * job->stepsToCm;

		vec3 offset = glm::f64vec3(appContext->platform->testPosX + 16000.0, appContext->platform->testPosY + 16000.0, appContext->platform->testPosZ + 16000.0) * job->stepsToCm;

		offset = offset.x * job->axisX.direction + offset.y * job->axisY.direction + offset.z * -job->axisZ.direction;

		for (size_t i = 0; i < job->cubePointCentroids.size(); ++i) {
			//if (job->cubePointCounts[i] == 0) {
				//continue;
			//}

			vec3 point = job->cubePointCentroids[i] + offset;
			pushDebugSphere(&appContext->defaultDebug, point, 0.02, vec3(0, 1, 1), 32);
		}

		//{
		//	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		//		int globalIdBase = boardIter * 9;

		//		for (int i = 0; i < 3; ++i) {
		//			// Rows
		//			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
		//				int pId0 = globalIdBase + (i * 3) + pairIter + 0;
		//				int pId1 = globalIdBase + (i * 3) + pairIter + 1;

		//				if (job->cubePointCounts[pId0] != 0 && job->cubePointCounts[pId1]) {
		//					pushDebugLine(&appContext->defaultDebug, job->cubePointCentroids[pId0] + offset, job->cubePointCentroids[pId1] + offset, vec3(139 / 256.0, 57 / 256.0, 227 / 256.0));
		//				}
		//			}

		//			// Columns
		//			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
		//				int pId0 = globalIdBase + i + (pairIter * 3) + 0;
		//				int pId1 = globalIdBase + i + (pairIter * 3) + 3;

		//				if (job->cubePointCounts[pId0] != 0 && job->cubePointCounts[pId1]) {
		//					pushDebugLine(&appContext->defaultDebug, job->cubePointCentroids[pId0] + offset, job->cubePointCentroids[pId1] + offset, vec3(139 / 256.0, 57 / 256.0, 227 / 256.0));
		//				}
		//			}
		//		}
		//	}
		//}
	}

	{
		std::vector<vec3>* modelPoints = &job->baIndvCubePoints;

		for (size_t i = 0; i < modelPoints->size(); ++i) {
			vec3 point = (*modelPoints)[i];

			pushDebugSphere(&appContext->defaultDebug, point, 0.02, vec3(0, 1, 1), 8);
			//displayTextAtPoint(&Tool->mainCamera, point, std::to_string(sampleId), vec4(1, 1, 1, 0.5), TextBuffer);
		}
	}

	{
		std::vector<vec3>* modelPoints = &job->stCubePoints;
		for (size_t cubeIter = 0; cubeIter < job->stCubeWorlds.size(); ++cubeIter) {
			srand(cubeIter);
			rand();
			vec3 col = getRandomColorHighSaturation();

			for (size_t i = 0; i < modelPoints->size(); ++i) {
				vec3 point = job->stCubeWorlds[cubeIter] * vec4((*modelPoints)[i], 1.0f);

				pushDebugSphere(&appContext->defaultDebug, point, 0.005, col, 8);
				//displayTextAtPoint(&Tool->mainCamera, point, std::to_string(sampleId), vec4(1, 1, 1, 0.5), TextBuffer);
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		mat4 viewMat = job->camVolumeMat[Tool->calibCamSelectedId];

		// NOTE: OpenCV cams look down +Z so we negate.
		//viewMat = job->baStereoCamWorld[Tool->calibCamSelectedId];
		viewMat[2] = -viewMat[2];
		viewMat = glm::inverse(viewMat);

		// Stereo test.
		//viewMat = glm::identity<mat4>();
		//viewMat[2] = vec4(0, 0, -1, 0.0f);

		/*for (size_t i = 0; i < job->baStereoPairs.size(); ++i) {
			if (job->baStereoPairs[i].sampleId == Tool->calibJobSelectedSampleId) {
				std::cout << "Found sample match\n";
				if (Tool->calibCamSelectedId == 0) {
					viewMat = glm::inverse(job->baStereoPairs[i].cam0world);
				} else {
					viewMat = glm::inverse(job->baStereoPairs[i].cam1world);
				}

				break;
			}
		}*/

		mat4 projMat = mat4(1.0);

		if (!job->camMatrix[Tool->calibCamSelectedId].empty()) {
			cv::Mat cameraMatrix = job->camMatrix[Tool->calibCamSelectedId];

			projMat = cameraCreateProjectionFromOpenCvCamera(CAM_IMG_WIDTH, CAM_IMG_HEIGHT, job->camMatrix[Tool->calibCamSelectedId], 0.01f, 100.0f);
			projMat = cameraProjectionToVirtualViewport(projMat, ViewPortTopLeft, ViewPortSize, vec2(Width, Height));
		}

		mat4 invProjMat = inverse(projMat);
		mat4 projViewMat = projMat * viewMat;

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
}

void _imageInspectorCopyFrameDataToTexture(ldiImageInspector* Tool, uint8_t* FrameData) {
	D3D11_MAPPED_SUBRESOURCE ms;
	Tool->appContext->d3dDeviceContext->Map(Tool->camTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	uint8_t* pixelData = (uint8_t*)ms.pData;

	for (int i = 0; i < CAM_IMG_HEIGHT; ++i) {
		memcpy(pixelData + i * ms.RowPitch, FrameData + i * CAM_IMG_WIDTH, CAM_IMG_WIDTH);
	}

	Tool->appContext->d3dDeviceContext->Unmap(Tool->camTex, 0);
}

void _imageInspectorCopyFrameDataToTextureEx(ldiApp* AppContext, ID3D11Texture2D* Tex, uint8_t* FrameData, int Width, int Height) {
	// TODO: Make sure texture width/height is acceptable.

	D3D11_MAPPED_SUBRESOURCE ms;
	AppContext->d3dDeviceContext->Map(Tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	uint8_t* pixelData = (uint8_t*)ms.pData;

	for (int i = 0; i < Height; ++i) {
		memcpy(pixelData + i * ms.RowPitch, FrameData + i * Width, Width);
	}

	AppContext->d3dDeviceContext->Unmap(Tex, 0);
}

void _imageInspectorSelectCalibJob(ldiImageInspector* Tool, int SelectionId) {
	if (Tool->calibJobSelectedSampleId == SelectionId) {
		return;
	}

	Tool->calibJobSelectedSampleId = SelectionId;

	if (SelectionId == -1) {
		return;
	}

	ldiCalibrationJob* job = &Tool->appContext->calibrationContext->calibJob;
	ldiCalibStereoSample* stereoSample = &job->samples[Tool->calibJobSelectedSampleId];
	
	_platformLoadStereoCalibSampleData(stereoSample);

	//for (int i = 0; i < 2; ++i) {
		ldiImage* calibImg = &stereoSample->frames[Tool->calibCamSelectedId];

		cv::Mat image(cv::Size(calibImg->width, calibImg->height), CV_8UC1, calibImg->data);
		cv::Mat outputImage(cv::Size(calibImg->width, calibImg->height), CV_8UC1);

		//cv::Mat calibCameraDist = cv::Mat::zeros(8, 1, CV_64F);
		//cv::undistort(image, outputImage, job->camMatrix[Tool->calibCamSelectedId], calibCameraDist);
		cv::undistort(image, outputImage, job->camMatrix[Tool->calibCamSelectedId], job->camDist[Tool->calibCamSelectedId]);
		//cv::undistort(image, outputImage, Tool->appContext->platform->hawks[Tool->calibCamSelectedId].defaultCameraMat, Tool->appContext->platform->hawks[Tool->calibCamSelectedId].defaultDistMat);

		_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, outputImage.data, calibImg->width, calibImg->height);

		/*for (int iY = 0; iY < stereoSample->frames[i].height; ++iY) {
			memcpy(Tool->hawkFrameBuffer[i] + iY * CAM_IMG_WIDTH, stereoSample->frames[i].data + iY * stereoSample->frames[i].width, stereoSample->frames[i].width);
		}*/
	//}

	_platformFreeStereoCalibImages(stereoSample);
}

void _imageInspectorProcessCalibJob(ldiImageInspector* Tool) {
	if (Tool->appContext->calibrationContext->calibJob.samples.size() == 0) {
		std::cout << "No calibration job samples to process\n";
		return;
	}

	platformCalculateStereoExtrinsics(Tool->appContext, &Tool->appContext->calibrationContext->calibJob);
}

void imageInspectorShowUi(ldiImageInspector* Tool) {
	ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;

	//bool hawkNewFrame = false;
	for (int i = 0; i < 2; ++i) {
		ldiHawk* mvCam = &Tool->appContext->platform->hawks[i];

		hawkUpdateValues(mvCam);

		bool newFrame = false;
		ldiImage camImg = {};

		{
			std::unique_lock<std::mutex> lock(mvCam->valuesMutex);
			if (Tool->hawkLastFrameId[i] != mvCam->latestFrameId) {
				Tool->hawkLastFrameId[i] = mvCam->latestFrameId;

				for (int iY = 0; iY < mvCam->imgHeight; ++iY) {
					memcpy(Tool->hawkFrameBuffer[i] + iY * CAM_IMG_WIDTH, mvCam->frameBuffer + iY * mvCam->imgWidth, mvCam->imgWidth);
				}

				newFrame = true;
				camImg.data = Tool->hawkFrameBuffer[i];
				camImg.width = mvCam->imgWidth;
				camImg.height = mvCam->imgHeight;
			}
		}

		if (newFrame) {
			_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, camImg.data, CAM_IMG_WIDTH, CAM_IMG_HEIGHT);

			if (Tool->camCalibProcess) {
				ldiCameraCalibSample calibSample;

				if (cameraCalibFindCirclesBoard(Tool->appContext, camImg, &calibSample)) {
					double currentTime = _getTime(Tool->appContext);
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

				findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults, &calibCameraMatrix, &calibCameraDist);
			}
		}
	}

	if (rotaryMeasurementProcess(&Tool->rotaryResults)) {
		_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, Tool->rotaryResults.greyFrame, 640, 480);
	}
	
	ImGui::Begin("Image inspector controls");

	static float imgScale = 1.0f;
	static vec2 imgOffset(0.0f, 0.0f);

	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Scale", &imgScale, 0.1f, 10.0f);
		ImGui::Text("Cursor position: %.2f, %.2f", Tool->camImageCursorPos.x, Tool->camImageCursorPos.y);
		ImGui::Text("Cursor value: %d", Tool->camImagePixelValue);
		ImGui::Checkbox("Show charuco results", &Tool->showCharucoResults);
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

	for (int i = 0; i < 2; ++i) {
		ldiHawk* mvCam = &Tool->appContext->platform->hawks[i];

		ImGui::PushID(i);

		char tempBuffer[256];
		sprintf_s(tempBuffer, "Camera %d", i);

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
				std::string dataPath = "../cache/calibImages_" + std::to_string(i) + ".dat";

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

				std::string dataPath = "../cache/calibImages_" + std::to_string(i) + ".dat";

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

			if (ImGui::Button("Bundle adjust")) {
				computerVisionBundleAdjust(Tool->cameraCalibSamples, &Tool->cameraCalibMatrix, &Tool->cameraCalibDist);
			}

			if (ImGui::Button("Load bundle results")) {
				computerVisionLoadBundleAdjust(Tool->cameraCalibSamples, &calibContext->bundleAdjustResult);
			}

			if (ImGui::Checkbox("Undistorted", &Tool->cameraCalibShowUndistorted)) {
				std::cout << "Undistorted: " << Tool->cameraCalibShowUndistorted << "\n";
			}

			if (ImGui::Checkbox("Undistorted bundle", &Tool->cameraCalibShowUndistortedBundled)) {
				std::cout << "Undistorted: " << Tool->cameraCalibShowUndistortedBundled << "\n";
			}
		}

		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Machine vision")) {
		ImGui::Checkbox("Process", &Tool->camImageProcess);
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

					if (Tool->cameraCalibShowUndistortedBundled) {
						cv::Mat image(cv::Size(calibImg->width, calibImg->height), CV_8UC1, calibImg->data);
						cv::Mat outputImage(cv::Size(calibImg->width, calibImg->height), CV_8UC1);
						cv::undistort(image, outputImage, calibContext->bundleAdjustResult.cameraMatrix, calibContext->bundleAdjustResult.distCoeffs);

						_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, outputImage.data, calibImg->width, calibImg->height);
					} else if (Tool->cameraCalibShowUndistorted) {
						cv::Mat image(cv::Size(calibImg->width, calibImg->height), CV_8UC1, calibImg->data);
						cv::Mat outputImage(cv::Size(calibImg->width, calibImg->height), CV_8UC1);
						cv::undistort(image, outputImage, Tool->cameraCalibMatrix, Tool->cameraCalibDist);
						
						_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, outputImage.data, calibImg->width, calibImg->height);
					} else {
						_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, calibImg->data, calibImg->width, calibImg->height);
					}

					_imageInspectorRenderHawk(Tool);
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	if (ImGui::CollapsingHeader("Platform calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Load job")) {
			Tool->imageMode = IIM_CALIBRATION_JOB;

			for (size_t i = 0; i < Tool->appContext->calibrationContext->calibJob.samples.size(); ++i) {
				_platformFreeStereoCalibImages(&Tool->appContext->calibrationContext->calibJob.samples[i]);
			}

			Tool->appContext->calibrationContext->calibJob.samples.clear();
			_imageInspectorSelectCalibJob(Tool, -1);

			std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/volume_calib_space/");

			for (int i = 0; i < filePaths.size(); ++i) {
				std::cout << "file " << i << ": " << filePaths[i] << "\n";

				if (endsWith(filePaths[i], ".dat")) {
					ldiCalibStereoSample sample = {};
					sample.path = filePaths[i];

					_platformLoadStereoCalibSampleData(&sample);

					for (int j = 0; j < 2; ++j) {
						findCharuco(sample.frames[j], Tool->appContext, &sample.cubes[j], &Tool->appContext->platform->hawks[j].defaultCameraMat, &Tool->appContext->platform->hawks[j].defaultDistMat);
					}

					_platformFreeStereoCalibImages(&sample);

					Tool->appContext->calibrationContext->calibJob.samples.push_back(sample);
				}
			}

			Tool->appContext->calibrationContext->calibJob.camMatrix[0] = Tool->appContext->platform->hawks[0].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.camDist[0] = Tool->appContext->platform->hawks[0].defaultDistMat.clone();

			Tool->appContext->calibrationContext->calibJob.camMatrix[1] = Tool->appContext->platform->hawks[1].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.camDist[1] = Tool->appContext->platform->hawks[1].defaultDistMat.clone();
		}

		if (ImGui::Button("Process job")) {
			_imageInspectorProcessCalibJob(Tool);
		}

		ImGui::Separator();

		if (ImGui::Button("Bundle adjust##volcal")) {
			Tool->appContext->calibrationContext->calibJob.startCamMat[0] = Tool->appContext->platform->hawks[0].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[0] = Tool->appContext->platform->hawks[0].defaultDistMat.clone();

			Tool->appContext->calibrationContext->calibJob.startCamMat[1] = Tool->appContext->platform->hawks[1].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[1] = Tool->appContext->platform->hawks[1].defaultDistMat.clone();

			computerVisionBundleAdjustStereo(&Tool->appContext->calibrationContext->calibJob);
		}

		if (ImGui::Button("Load bundle adjust##volcal")) {
			computerVisionLoadBundleAdjustStereo(&Tool->appContext->calibrationContext->calibJob);
		}

		ImGui::Separator();

		if (ImGui::Button("Bundle adjust individual")) {
			Tool->appContext->calibrationContext->calibJob.startCamMat[0] = Tool->appContext->platform->hawks[0].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[0] = Tool->appContext->platform->hawks[0].defaultDistMat.clone();

			Tool->appContext->calibrationContext->calibJob.startCamMat[1] = Tool->appContext->platform->hawks[1].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[1] = Tool->appContext->platform->hawks[1].defaultDistMat.clone();

			computerVisionBundleAdjustStereoIndividual(&Tool->appContext->calibrationContext->calibJob);
		}
		
		if (ImGui::Button("Bundle adjust individual load")) {
			computerVisionBundleAdjustStereoIndividualLoad(&Tool->appContext->calibrationContext->calibJob);
		}

		ImGui::Separator();

		if (ImGui::Button("Bundle adjust both")) {
			Tool->appContext->calibrationContext->calibJob.startCamMat[0] = Tool->appContext->platform->hawks[0].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[0] = Tool->appContext->platform->hawks[0].defaultDistMat.clone();

			Tool->appContext->calibrationContext->calibJob.startCamMat[1] = Tool->appContext->platform->hawks[1].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[1] = Tool->appContext->platform->hawks[1].defaultDistMat.clone();

			computerVisionBundleAdjustStereoBoth(&Tool->appContext->calibrationContext->calibJob);
		}

		if (ImGui::Button("Bundle adjust both load")) {
			computerVisionBundleAdjustStereoBothLoad(&Tool->appContext->calibrationContext->calibJob);
		}

		ImGui::Separator();

		/*if (ImGui::Button("Bundle adjust py")) {
			computerVisionBundleAdjustStereoBothPy(&Tool->appContext->calibrationContext->calibJob);
		}*/

		if (ImGui::Button("Stereo calibrate")) {
			Tool->appContext->calibrationContext->calibJob.startCamMat[0] = Tool->appContext->platform->hawks[0].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[0] = Tool->appContext->platform->hawks[0].defaultDistMat.clone();

			Tool->appContext->calibrationContext->calibJob.startCamMat[1] = Tool->appContext->platform->hawks[1].defaultCameraMat.clone();
			Tool->appContext->calibrationContext->calibJob.startDistMat[1] = Tool->appContext->platform->hawks[1].defaultDistMat.clone();

			computerVisionStereoCameraCalibrate(&Tool->appContext->calibrationContext->calibJob);
		}

		ImGui::Separator();


		ImGui::InputInt("Cam ID", &Tool->calibCamSelectedId);
		if (Tool->calibCamSelectedId < 0 || Tool->calibCamSelectedId > 1) {
			Tool->calibCamSelectedId = 0;
		}

		ImGui::Text("Samples (%d)", Tool->appContext->calibrationContext->calibJob.samples.size());
		//if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, -FLT_MIN))) {
			for (int n = 0; n < Tool->appContext->calibrationContext->calibJob.samples.size(); ++n) {
				bool isSelected = (Tool->calibJobSelectedSampleId == n);

				if (ImGui::Selectable(Tool->appContext->calibrationContext->calibJob.samples[n].path.c_str(), isSelected)) {
					// NOTE: Always switch back to calibration job mode if clicking on a job sample.
					Tool->imageMode = IIM_CALIBRATION_JOB;

					_imageInspectorSelectCalibJob(Tool, n);

					//if (Tool->camImageProcess) {
					//	ldiImage camImg = {};
					//	camImg.data = Tool->camPixelsFinal;
					//	camImg.width = CAM_IMG_WIDTH;
					//	camImg.height = CAM_IMG_HEIGHT;

					//	//findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
					//}
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	//ImGui::Checkbox("Grid", &camInspector->showGrid);
	//if (ImGui::Button("Run sample test")) {
		//camInspectorRunTest(camInspector);
	//}
	ImGui::End();

	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// This will catch our interactions.
		ImGui::InvisibleButton("__imgInspecViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

		const bool isMouseRight = ImGui::GetIO().MouseDown[1];
		const bool isMouseLeft = ImGui::GetIO().MouseDown[0];

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

				if (imgScale < 0.01) {
					imgScale = 0.01;
				}

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

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
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

		ImVec2 imgMin;
		imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
		imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

		ImVec2 imgMax;
		imgMax.x = imgMin.x + CAM_IMG_WIDTH * imgScale;
		imgMax.y = imgMin.y + CAM_IMG_HEIGHT * imgScale;

		draw_list->AddCallback(_imageInspectorSetStateCallback, Tool);
		draw_list->AddImage(Tool->camResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

		uv_min = ImVec2(0.0f, 0.0f);
		uv_max = ImVec2(1.0f, 1.0f);
		tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		//draw_list->AddCallback(_imageInspectorSetStateCallback2, Tool);

		_imageInspectorRenderHawkVolume(Tool, viewSize.x, viewSize.y, vec2(imgOffset.x, imgOffset.y) * imgScale, vec2((float)CAM_IMG_WIDTH, (float)CAM_IMG_HEIGHT) * imgScale);
		draw_list->AddImage(Tool->hawkViewBuffer.mainViewResourceView, screenStartPos, screenStartPos + viewSize, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		//draw_list->AddImage(Tool->hawkViewBuffer.mainViewResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));
		//draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

		draw_list->AddText(imgMin, ImColor(200, 200, 200), "Hawk 0");
		
		//----------------------------------------------------------------------------------------------------
		// Draw webcam results.
		//----------------------------------------------------------------------------------------------------
		if (Tool->rotaryResults.showGizmos) {
			ldiRotaryResults* rotary = &Tool->rotaryResults;
			
			{
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + 320) * imgScale;
				offset.y = screenStartPos.y + (imgOffset.y + 240) * imgScale;

				// Center tracking.
				draw_list->AddCircle(offset, 40.0f * imgScale, ImColor(200, 0, 200));

				// Inner track.
				//draw_list->AddCircle(offset, 180.0f * imgScale, ImColor(200, 0, 200));

				// Outer track.
				draw_list->AddCircle(offset, 260.0f * imgScale, ImColor(200, 0, 200));
			}

			for (int i = 0; i < rotary->blobs.size(); ++i) {
				vec2 k = rotary->blobs[i];

				// NOTE: Half pixel offset required.
				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + k.x) * imgScale;
				offset.y = screenStartPos.y + (imgOffset.y + k.y) * imgScale;

				draw_list->AddCircle(offset, 2.0f, ImColor(0, 0, 255));

				/*char strBuf[256];
				sprintf_s(strBuf, 256, "%.2f, %.2f : %.3f", k->pt.x, k->pt.y, k->size);
				draw_list->AddText(offset, ImColor(0, 175, 0), strBuf);*/
			}

			if (rotary->foundCenter && rotary->foundLocator) {
				{
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + rotary->centerPos.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + rotary->centerPos.y) * imgScale;
					draw_list->AddCircle(offset, 4.0f * imgScale, ImColor(0, 255, 0));
				}

				{
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + rotary->locatorPos.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + rotary->locatorPos.y) * imgScale;
					draw_list->AddCircle(offset, 4.0f * imgScale, ImColor(255, 255, 0));
				}
				
				
				//for (int i = 0; i < rotary->points.size(); ++i) {
				//	ldiRotaryPoint* k = &rotary->points[i];

				//	// NOTE: Half pixel offset required.
				//	ImVec2 offset = pos;
				//	offset.x = screenStartPos.x + (imgOffset.x + k->pos.x) * imgScale;
				//	offset.y = screenStartPos.y + (imgOffset.y + k->pos.y) * imgScale;

				//	draw_list->AddCircle(offset, 4.0f, ImColor(255, 0, 0));

				//	char strBuf[256];
				//	//sprintf_s(strBuf, 256, "%d %.2f, %.2f : %f", k->id, k->pos.x, k->pos.y, k->info);
				//	sprintf_s(strBuf, 256, "%d %.3f", k->id, k->info);
				//	draw_list->AddText(offset, ImColor(0, 255, 0), strBuf);
				//}

				for (int i = 0; i < 32; ++i) {
					ldiRotaryPoint* k = &rotary->accumPoints[i];

					// NOTE: Half pixel offset required.
					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + k->pos.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + k->pos.y) * imgScale;

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

					// NOTE: Half pixel offset required.
					ImVec2 aPos;
					aPos.x = screenStartPos.x + (imgOffset.x + a->pos.x) * imgScale;
					aPos.y = screenStartPos.y + (imgOffset.y + a->pos.y) * imgScale;

					ImVec2 bPos;
					bPos.x = screenStartPos.x + (imgOffset.x + b->pos.x) * imgScale;
					bPos.y = screenStartPos.y + (imgOffset.y + b->pos.y) * imgScale;

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
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;
				
					draw_list->AddCircle(offset, 4.0f, ImColor(rndCol.x, rndCol.y, rndCol.z));
				}

				if (Tool->cameraCalibSelectedSample != iterSamples) {
					continue;
				}

				for (size_t iterPoints = 0; iterPoints < sample->undistortedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->undistortedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

					draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
				}

				for (size_t iterPoints = 0; iterPoints < sample->reprojectedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->reprojectedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

					draw_list->AddCircle(offset, 2.0f, ImColor(2, 117, 247));
				}
			}

			for (size_t iterSamples = 0; iterSamples < calibContext->bundleAdjustResult.samples.size(); ++iterSamples) {
				ldiCameraCalibSample* sample = &calibContext->bundleAdjustResult.samples[iterSamples];

				if (Tool->cameraCalibSelectedSample != iterSamples) {
					continue;
				}

				for (size_t iterPoints = 0; iterPoints < sample->undistortedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->undistortedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

					draw_list->AddCircle(offset, 2.0f, ImColor(100, 255, 0));
				}

				for (size_t iterPoints = 0; iterPoints < sample->reprojectedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->reprojectedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

					draw_list->AddCircle(offset, 3.0f, ImColor(0, 0, 255));
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw calib job results.
		//----------------------------------------------------------------------------------------------------
		if (Tool->imageMode == IIM_CALIBRATION_JOB) {
			ldiCalibrationJob* calibJob = &Tool->appContext->calibrationContext->calibJob;

			for (size_t i = 0; i < calibJob->massModelImagePoints[0].size(); ++i) {
				int selectedCam = Tool->calibCamSelectedId;

				ImVec2 uiPos = screenStartPos + (toImVec2(imgOffset) + toImVec2(calibJob->massModelImagePoints[selectedCam][i])) * imgScale;
				draw_list->AddCircle(uiPos, 2.0f, ImColor(224, 93, 11));

				uiPos = screenStartPos + (toImVec2(imgOffset) + toImVec2(calibJob->massModelUndistortedPoints[selectedCam][i])) * imgScale;
				draw_list->AddCircle(uiPos, 2.0f, ImColor(65, 158, 14));
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw charuco results.
		//----------------------------------------------------------------------------------------------------
		if (Tool->showCharucoResults) {
			std::vector<vec2> markerCentroids;

			ldiCharucoResults* charucos = &Tool->camImageCharucoResults;

			if (Tool->imageMode == IIM_CALIBRATION_JOB && Tool->calibJobSelectedSampleId != -1) {
				charucos = &Tool->appContext->calibrationContext->calibJob.samples[Tool->calibJobSelectedSampleId].cubes[Tool->calibCamSelectedId];
			}

			for (int i = 0; i < charucos->markers.size(); ++i) {
				/*ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
				offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;*/

				ImVec2 points[5];

				vec2 markerCentroid(0.0f, 0.0f);

				for (int j = 0; j < 4; ++j) {
					vec2 o = charucos->markers[i].corners[j];

					points[j] = pos;
					points[j].x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					points[j].y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

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
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale;

					draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
				}
			}

			for (int b = 0; b < charucos->boards.size(); ++b) {
				for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
					vec2 o = charucos->boards[b].corners[i].position;
					int cornerId = charucos->boards[b].corners[i].globalId;

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x) * imgScale + 5;
					offset.y = screenStartPos.y + (imgOffset.y + o.y) * imgScale - 15;

					char strBuf[256];
					sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x, o.y);

					draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
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

			Tool->camImageCursorPos = worldPos;

			if (pixelPos.x >= 0 && pixelPos.x < CAM_IMG_WIDTH) {
				if (pixelPos.y >= 0 && pixelPos.y < CAM_IMG_HEIGHT) {
					//Tool->camImagePixelValue = Tool->camPixelsFinal[(int)pixelPos.y * CAM_IMG_WIDTH + (int)pixelPos.x];
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

		// Viewport overlay.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_imageInspectorOverlay", ImVec2(200, 70), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			//ImGui::Text("%.3f %.3f - %.3f", tool->surfelViewImgOffset.x, tool->surfelViewImgOffset.y, tool->surfelViewScale);
			ImGui::Text("%.3f %.3f", worldPos.x, worldPos.y);
			//ImGui::Text("Surfels: %d", laserViewSurfelCount);

			ImGui::EndChild();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}