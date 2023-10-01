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

struct ldiCalibrationJob {
	std::vector<std::string> fileSamples;
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

	int							camLastNewFrameId = 0;
	std::atomic_int				camNewFrameId = 0;
	uint8_t						camPixelsFinal[CAM_IMG_WIDTH * CAM_IMG_HEIGHT] = {};

	ldiCalibrationJob			calibJob;
	int							calibJobSelectedSampleId;
	uint8_t						calibJobImage[CAM_IMG_WIDTH * CAM_IMG_HEIGHT];

	bool						edgePlaceStart = false;
	bool						edgePlaceEnd = false;
	vec2						edgeLineStart = vec2(700.5f, 768.5f);
	vec2						edgeLineEnd = vec2(700.5f, 800.5f);
	int							edgeLineLength = 10;

	ldiRotaryResults			rotaryResults;

	std::vector<ldiCameraCalibSample> cameraCalibSamples;
	int							cameraCalibImageWidth;
	int							cameraCalibImageHeight;
	int							cameraCalibSelectedSample = -1;
	cv::Mat						cameraCalibMatrix;
	cv::Mat						cameraCalibDist;
	bool						cameraCalibShowUndistorted = false;
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

	return 0;
}

void _imageInspectorCopyFrameDataToTexture(ldiImageInspector* Tool, uint8_t* FrameData) {
	D3D11_MAPPED_SUBRESOURCE ms;
	Tool->appContext->d3dDeviceContext->Map(Tool->camTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	uint8_t* pixelData = (uint8_t*)ms.pData;

	for (int i = 0; i < CAM_IMG_HEIGHT; ++i) {
		//memcpy(pixelData + i * ms.RowPitch, Tool->camPixelsFinal + i * 4064, 4064);
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

	std::cout << "Sel " << Tool->calibJobSelectedSampleId << "\n";

	FILE* file;
	std::string fileName = Tool->calibJob.fileSamples[Tool->calibJobSelectedSampleId];
	fopen_s(&file, fileName.c_str(), "rb");

	int width;
	int height;
	fread(&width, 4, 1, file);
	fread(&height, 4, 1, file);
	fread(&Tool->calibJobImage, width * height, 1, file);
	fclose(file);

	// Copy image to fullpixel input.
	/*for (int i = 0; i < width * height; ++i) {
		float src = ((float)Tool->calibJobImage[i]) / 255.0f;
		float corrected = pow(src, 1.0 / 2.2);
		uint8_t srcFinal = (uint8_t)(corrected * 255.0f);

		Tool->camPixelsFinal[i] = srcFinal;
	}*/
	memcpy(Tool->camPixelsFinal, Tool->calibJobImage, width * height);

	//imageWrite((fileName + ".png").c_str(), width, height, 1, width, Tool->calibJobImage);

	_imageInspectorCopyFrameDataToTexture(Tool, Tool->calibJobImage);
}

void _imageInspectorProcessCalibJob(ldiImageInspector* Tool) {
	if (Tool->calibJob.fileSamples.size() == 0) {
		std::cout << "No calibration job samples to process\n";
		return;
	}
}

void imageInspectorShowUi(ldiImageInspector* Tool) {
	if (Tool->imageMode == IIM_LIVE_CAMERA) {
		if (Tool->camNewFrameId != Tool->camLastNewFrameId) {
			Tool->camLastNewFrameId = Tool->camNewFrameId;

			std::cout << "Update frame\n";

			_imageInspectorCopyFrameDataToTexture(Tool, Tool->camPixelsFinal);

			if (Tool->camImageProcess) {
				ldiImage camImg = {};
				camImg.data = Tool->camPixelsFinal;
				camImg.width = CAM_IMG_WIDTH;
				camImg.height = CAM_IMG_HEIGHT;

				//findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
			}
		}
	}

	bool hawkNewFrame = false;
	for (int i = 0; i < 2; ++i) {
		ldiHawk* mvCam = &Tool->appContext->platform->hawks[i];

		if (hawkUpdate(Tool->appContext, mvCam)) {
			hawkNewFrame = true;
			// NOTE: Got a new frame.

			for (int iY = 0; iY < mvCam->imgHeight; ++iY) {
				memcpy(Tool->camPixelsFinal + (i * 1640) + iY * CAM_IMG_WIDTH, mvCam->frameBuffer + iY * mvCam->imgWidth, mvCam->imgWidth);
			}

			if (Tool->camCalibProcess) {
				ldiImage camImg = {};
				camImg.data = mvCam->frameBuffer;
				camImg.width = mvCam->imgWidth;
				camImg.height = mvCam->imgHeight;

				ldiCameraCalibSample calibSample;

				if (cameraCalibFindCirclesBoard(Tool->appContext, camImg, &calibSample)) {
					double currentTime = _getTime(Tool->appContext);
					if (currentTime - Tool->calibDetectTimeout >= 0.0) {
						std::cout << "Capture\n";
						Tool->calibDetectTimeout = currentTime + 1.0;

						ldiImage* newFrameImg = new ldiImage;
						newFrameImg->data = new uint8_t[mvCam->imgWidth * mvCam->imgHeight];
						newFrameImg->width = mvCam->imgWidth;
						newFrameImg->height = mvCam->imgHeight;
						memcpy(newFrameImg->data, mvCam->frameBuffer, mvCam->imgWidth * mvCam->imgHeight);

						calibSample.image = newFrameImg;
						
						Tool->cameraCalibSamples.push_back(calibSample);
						Tool->cameraCalibImageWidth = mvCam->imgWidth;
						Tool->cameraCalibImageHeight = mvCam->imgHeight;
					}
				}
			}

			if (Tool->camImageProcess) {
				ldiImage camImg = {};
				camImg.data = mvCam->frameBuffer;
				camImg.width = mvCam->imgWidth;
				camImg.height = mvCam->imgHeight;

				cv::Mat calibCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
				calibCameraMatrix.at<double>(0, 0) = 1693.30789;
				calibCameraMatrix.at<double>(0, 1) = 0.0;
				calibCameraMatrix.at<double>(0, 2) = 800;
				calibCameraMatrix.at<double>(1, 0) = 0.0;
				calibCameraMatrix.at<double>(1, 1) = 1693.30789;
				calibCameraMatrix.at<double>(1, 2) = 650;
				calibCameraMatrix.at<double>(2, 0) = 0.0;
				calibCameraMatrix.at<double>(2, 1) = 0.0;
				calibCameraMatrix.at<double>(2, 2) = 1.0;

				cv::Mat calibCameraDist = cv::Mat::zeros(8, 1, CV_64F);

				findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults, &calibCameraMatrix, &calibCameraDist);
			}
		}
	}

	if (hawkNewFrame) {
		_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, Tool->camPixelsFinal, CAM_IMG_WIDTH, CAM_IMG_HEIGHT);
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

	if (ImGui::CollapsingHeader("Edge detection")) {
		ImGui::Checkbox("Place start", &Tool->edgePlaceStart);
		ImGui::Checkbox("Place stack end", &Tool->edgePlaceEnd);
		ImGui::SliderInt("Line length", &Tool->edgeLineLength, 1, 50);
		ImGui::DragFloat2("Line start", (float*)&Tool->edgeLineStart, 0.1f, 0.0f, CAM_IMG_WIDTH);
		ImGui::DragFloat2("Line end", (float*)&Tool->edgeLineEnd, 0.1f, 0.0f, CAM_IMG_WIDTH);
	}

	for (int i = 0; i < 2; ++i) {
		ldiHawk* mvCam = &Tool->appContext->platform->hawks[i];

		ImGui::PushID(i);

		char tempBuffer[256];
		sprintf_s(tempBuffer, "Camera %d", i);

		if (ImGui::CollapsingHeader(tempBuffer, ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::SliderInt("Shutter speed", &mvCam->shutterSpeed, 1, 66000)) {
				hawkCommitSettings(mvCam);
			}

			if (ImGui::SliderInt("Analog gain", &mvCam->analogGain, 0, 100)) {
				hawkCommitSettings(mvCam);
			}

			ImGui::SliderFloat("Filter factor", &Tool->camImageFilterFactor, 0.0f, 1.0f);

			if (ImGui::Button("Start continuous mode")) {
				//Tool->imageMode = IIM_LIVE_CAMERA;
				hawkSetMode(mvCam, LDI_CAMERACAPTUREMODE_CONTINUOUS);
			}

			if (ImGui::Button("Stop continuous mode")) {
				//Tool->imageMode = IIM_LIVE_CAMERA;
				hawkSetMode(mvCam, LDI_CAMERACAPTUREMODE_WAIT);
			}

			if (ImGui::Button("Get single image")) {
				//Tool->imageMode = IIM_LIVE_CAMERA;
				hawkSetMode(mvCam, LDI_CAMERACAPTUREMODE_SINGLE);
			}

			if (ImGui::Button("Get average image")) {
				//Tool->imageMode = IIM_LIVE_CAMERA;
				hawkSetMode(mvCam, LDI_CAMERACAPTUREMODE_AVERAGE);
			}

			if (ImGui::Button("Save image")) {
				double t0 = _getTime(Tool->appContext);
				imageWrite("test.png", mvCam->imgWidth, mvCam->imgHeight, 1, mvCam->imgWidth, mvCam->frameBuffer);
				t0 = _getTime(Tool->appContext) - t0;
				std::cout << "Save frame: " << (t0 * 1000.0) << " ms\n";
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
				FILE* file;
				fopen_s(&file, "../cache/calibImages.dat", "wb");

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

				FILE* file;
				fopen_s(&file, "../cache/calibImages.dat", "rb");

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

			if (ImGui::Checkbox("Undistorred", &Tool->cameraCalibShowUndistorted)) {
				std::cout << "Undistorted: " << Tool->cameraCalibShowUndistorted << "\n";
			}
		}

		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Machine vision", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Process", &Tool->camImageProcess);

		if (ImGui::Button("Find Charuco")) {
			ldiImage camImg = {};
			camImg.data = Tool->camPixelsFinal;
			camImg.width = CAM_IMG_WIDTH;
			camImg.height = CAM_IMG_HEIGHT;

			//findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
		}
	}

	if (ImGui::CollapsingHeader("Camera calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
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
						
						_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, outputImage.data, calibImg->width, calibImg->height);
					} else {
						_imageInspectorCopyFrameDataToTextureEx(Tool->appContext, Tool->camTex, calibImg->data, calibImg->width, calibImg->height);
					}
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
	}

	if (ImGui::CollapsingHeader("Platform calibration")) {
		if (ImGui::Button("Load job")) {
			Tool->imageMode = IIM_CALIBRATION_JOB;
			Tool->calibJob.fileSamples.clear();
			_imageInspectorSelectCalibJob(Tool, -1);

			std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/calib/");

			for (int i = 0; i < filePaths.size(); ++i) {
				std::cout << "file " << i << ": " << filePaths[i] << "\n";

				if (endsWith(filePaths[i], ".dat")) {
					Tool->calibJob.fileSamples.push_back(filePaths[i]);
				}
			}
		}

		if (ImGui::Button("Process job")) {
			_imageInspectorProcessCalibJob(Tool);
		}

		ImGui::Text("Samples");
		//if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing()))) {
		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, -FLT_MIN))) {
			for (int n = 0; n < Tool->calibJob.fileSamples.size(); ++n) {
				bool isSelected = (Tool->calibJobSelectedSampleId == n);

				if (ImGui::Selectable(Tool->calibJob.fileSamples[n].c_str(), isSelected)) {
					// NOTE: Always switch back to calibration job mode if clicking on a job sample.
					Tool->imageMode == IIM_CALIBRATION_JOB;

					_imageInspectorSelectCalibJob(Tool, n);

					if (Tool->camImageProcess) {
						ldiImage camImg = {};
						camImg.data = Tool->camPixelsFinal;
						camImg.width = CAM_IMG_WIDTH;
						camImg.height = CAM_IMG_HEIGHT;

						//findCharuco(camImg, Tool->appContext, &Tool->camImageCharucoResults);
					}
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

		if (Tool->edgePlaceStart && Tool->edgePlaceEnd) {
			Tool->edgePlaceStart = false;
			Tool->edgePlaceEnd = false;
		}

		if (isMouseLeft && isActive && Tool->edgePlaceStart) {
			//Tool->edgeLineStart = worldPos;
			Tool->edgeLineStart.x = (int)worldPos.x + 0.5f;
			Tool->edgeLineStart.y = (int)worldPos.y + 0.5f;
		}

		if (isMouseLeft && isActive && Tool->edgePlaceEnd) {
			//Tool->edgeLineStart = worldPos;
			Tool->edgeLineEnd.x = (int)worldPos.x + 0.5f;
			Tool->edgeLineEnd.y = (int)worldPos.y + 0.5f;
		}

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
				draw_list->AddCircle(offset, 180.0f * imgScale, ImColor(200, 0, 200));

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
					offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;
				
					draw_list->AddCircle(offset, 4.0f, ImColor(rndCol.x, rndCol.y, rndCol.z));
				}

				if (Tool->cameraCalibSelectedSample != iterSamples) {
					continue;
				}

				for (size_t iterPoints = 0; iterPoints < sample->undistortedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->undistortedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

					draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
				}

				for (size_t iterPoints = 0; iterPoints < sample->reprojectedImagePoints.size(); ++iterPoints) {
					cv::Point2f o = sample->reprojectedImagePoints[iterPoints];

					ImVec2 offset = pos;
					offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
					offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

					draw_list->AddCircle(offset, 2.0f, ImColor(2, 117, 247));
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Draw charuco results.
		//----------------------------------------------------------------------------------------------------
		std::vector<vec2> markerCentroids;

		const ldiCharucoResults* charucos = &Tool->camImageCharucoResults;

		for (int i = 0; i < charucos->markers.size(); ++i) {
			/*ImVec2 offset = pos;
			offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
			offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;*/

			ImVec2 points[5];

			vec2 markerCentroid(0.0f, 0.0f);

			for (int j = 0; j < 4; ++j) {
				vec2 o = charucos->markers[i].corners[j];

				points[j] = pos;
				points[j].x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
				points[j].y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

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
				offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
				offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

				draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
			}
		}

		for (int b = 0; b < charucos->boards.size(); ++b) {
			for (int i = 0; i < charucos->boards[b].corners.size(); ++i) {
				vec2 o = charucos->boards[b].corners[i].position;
				int cornerId = charucos->boards[b].corners[i].id;

				ImVec2 offset = pos;
				offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale + 5;
				offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale - 15;

				char strBuf[256];
				sprintf_s(strBuf, 256, "%d %.2f, %.2f", cornerId, o.x + 0.5f, o.y + 0.5f);

				draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
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
					Tool->camImagePixelValue = Tool->camPixelsFinal[(int)pixelPos.y * CAM_IMG_WIDTH + (int)pixelPos.x];
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