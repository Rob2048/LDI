#pragma once

#include "horse.h"
#include "hawk.h"
#include "panther.h"

enum ldiPlatformJobType {
	PJT_MOVE_AXIS,
	PJT_DIAG,
	PJT_HOME,
	PJT_CAPTURE_CALIBRATION,
};

struct ldiPlatformJobHeader {
	int type;
};

struct ldiPlatformJobMoveAxis {
	ldiPlatformJobHeader header;
	ldiPantherAxis axisId;
	int steps;
	float velocity;
	bool relative;
};

struct ldiPlatform {
	ldiApp*						appContext;

	std::thread					workerThread;
	std::atomic_bool			workerThreadRunning = true;
	std::atomic_bool			jobCancel = false;
	std::atomic_bool			jobExecuting = false;
	int							jobState = 0;
	ldiPlatformJobHeader*		job;
	std::mutex					jobAvailableMutex;
	std::condition_variable		jobAvailableCondVar;

	//ldiCamera					camera;
	
	ldiRenderViewBuffers		mainViewBuffers;
	int							mainViewWidth;
	int							mainViewHeight;
	ldiCamera					mainCamera;
	float						mainCameraSpeed = 1.0f;
	vec4						mainBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	// Motion platform.
	ldiHorse					horse;
	float						positionX;
	float						positionY;
	float						positionZ;
	float						positionC;
	float						positionA;
	bool						homed;

	float						mmPerStepX;

	// Dual machine vision cameras.
	ldiHawk						hawks[2];

	// Motion/Galvo/Laser control.
	ldiPanther					panther;

	int							testPosX;
	int							testPosY;
	int							testPosZ;
};

void platformCalculateStereoExtrinsics(ldiApp* AppContext, ldiCalibrationJob* Job) {
	std::vector<cv::Point2f> camImagePoints[2];
	std::vector<cv::Point2f> camUndistortedPoints[2];

	std::cout << "Creating massive model\n";

	Job->massModelPointIds.clear();

	//----------------------------------------------------------------------------------------------------
	// Find matching image points in each cam pair for each calib sample.
	//----------------------------------------------------------------------------------------------------
	for (int sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];
		int matchedPoints = 0;

		for (int c0BoardIter = 0; c0BoardIter < sample->cubes[0].boards.size(); ++c0BoardIter) {
			for (int c0CornerIter = 0; c0CornerIter < sample->cubes[0].boards[c0BoardIter].corners.size(); ++c0CornerIter) {
				ldiCharucoCorner* corner0 = &sample->cubes[0].boards[c0BoardIter].corners[c0CornerIter];
				ldiCharucoCorner* corner1 = nullptr;

				bool foundCornerMatch = false;

				for (int c1BoardIter = 0; c1BoardIter < sample->cubes[1].boards.size(); ++c1BoardIter) {
					if (foundCornerMatch) {
						break;
					}

					for (int c1CornerIter = 0; c1CornerIter < sample->cubes[1].boards[c1BoardIter].corners.size(); ++c1CornerIter) {
						corner1 = &sample->cubes[1].boards[c1BoardIter].corners[c1CornerIter];

						if (corner0->globalId == corner1->globalId) {
							foundCornerMatch = true;
							break;
						}
					}
				}

				if (foundCornerMatch) {
					// TODO: Add to mass model.
					++matchedPoints;
					camImagePoints[0].push_back(cv::Point2f(corner0->position.x, corner0->position.y));
					camImagePoints[1].push_back(cv::Point2f(corner1->position.x, corner1->position.y));
					Job->massModelPointIds.push_back(sampleIter * (9 * 6) + corner0->globalId);
				}
			}
		}

		std::cout << "Sample: " << sampleIter << " Matches: " << matchedPoints << "\n";
	}

	std::cout << "Total matched points: " << camImagePoints[0].size() << "\n";
	
	//----------------------------------------------------------------------------------------------------
	// Undistort matched points.
	//----------------------------------------------------------------------------------------------------
	cv::Mat camDefaultMat[2];
	camDefaultMat[0] = AppContext->platform->hawks[0].defaultCameraMat;
	camDefaultMat[1] = AppContext->platform->hawks[1].defaultCameraMat;

	cv::Mat distDefaultMat[2];
	distDefaultMat[0] = AppContext->platform->hawks[0].defaultDistMat;
	distDefaultMat[1] = AppContext->platform->hawks[1].defaultDistMat;

	cv::undistortPoints(camImagePoints[0], camUndistortedPoints[0], camDefaultMat[0], distDefaultMat[0], cv::noArray(), camDefaultMat[0]);
	cv::undistortPoints(camImagePoints[1], camUndistortedPoints[1], camDefaultMat[1], distDefaultMat[1], cv::noArray(), camDefaultMat[1]);

	Job->massModelImagePoints[0].clear();
	Job->massModelImagePoints[1].clear();

	Job->massModelUndistortedPoints[0].clear();
	Job->massModelUndistortedPoints[1].clear();

	for (size_t i = 0; i < camImagePoints[0].size(); ++i) {
		Job->massModelImagePoints[0].push_back(vec2(camImagePoints[0][i].x, camImagePoints[0][i].y));
		Job->massModelUndistortedPoints[0].push_back(vec2(camUndistortedPoints[0][i].x, camUndistortedPoints[0][i].y));

		Job->massModelImagePoints[1].push_back(vec2(camImagePoints[1][i].x, camImagePoints[1][i].y));
		Job->massModelUndistortedPoints[1].push_back(vec2(camUndistortedPoints[1][i].x, camUndistortedPoints[1][i].y));
	}

	////----------------------------------------------------------------------------------------------------
	//// Build fundamental matrix.
	////----------------------------------------------------------------------------------------------------
	//cv::Mat fMats;
	//try {
	//	fMats = cv::findFundamentalMat(camUndistortedPoints[0], camUndistortedPoints[1], cv::FM_LMEDS, 3.0, 0.99);
	//	//fMats = cv::findFundamentalMat(trackerPoints[0], trackerPoints[1], CV_FM_RANSAC, 3.0, 0.99);
	//} catch (cv::Exception& e) {
	//	const char* err_msg = e.what();
	//	std::cout << "Fundamental Mat Failed: " << err_msg;
	//	return;
	//}

	//std::cout << "FMat: " << fMats << "\n";
	//cv::Mat essentialMat(3, 3, CV_64F);
	// NOTE: Second camera K first.
	//essentialMat = calibCameraMatrix.t() * fMats * calibCameraMatrix;

	cv::Mat essentialMat;
	try {
		essentialMat = cv::findEssentialMat(camUndistortedPoints[0], camUndistortedPoints[1], camDefaultMat[0], distDefaultMat[0], camDefaultMat[1], distDefaultMat[1]);
		std::cout << "Essential mat: " << essentialMat << "\n";
	} catch (cv::Exception& e) {
		const char* err_msg = e.what();
		std::cout << "Fundamental Mat failed: " << err_msg;
		return;
	}

	//---------------------------------------------------------------------------------------------------
	// Recover camera poses.
	//---------------------------------------------------------------------------------------------------
	cv::Mat r;
	cv::Mat t;
	
	try {
		cv::recoverPose(essentialMat, camUndistortedPoints[0], camUndistortedPoints[1], r, t, 2660, cv::Point2d(3280 / 2, 2464 / 2));
		//cv::recoverPose(essentialMat, camUndistortedPoints[0], camUndistortedPoints[1], calibCameraMatrix, r, t, 100.0, cv::noArray(), triPoints);
		//cv::recoverPose(camUndistortedPoints[0], camUndistortedPoints[1], camDefaultMat[0], distDefaultMat[0], camDefaultMat[1], distDefaultMat[1], essentialMat, r, t);
	} catch (cv::Exception& e) {
		const char* err_msg = e.what();
		std::cout << "Recover pose failed: " << err_msg;
		return;
	}

	//cv::Mat trueT = -(r).t() * t;

	cv::Mat pose0 = cv::Mat::eye(3, 4, r.type());
	cv::Mat pose1(3, 4, r.type());
	pose1(cv::Range::all(), cv::Range(0, 3)) = r * 1.0;
	pose1.col(3) = t * 1.0;
	//Pose = pose1.clone();

	Job->rtMat[0] = pose0.clone();
	Job->rtMat[1] = pose1.clone();

	cv::Mat proj0 = camDefaultMat[0] * pose0;
	cv::Mat proj1 = camDefaultMat[1] * pose1;

	std::cout << "Pose0: " << pose0 << "\n";
	std::cout << "Pose1: " << pose1 << "\n";

	//---------------------------------------------------------------------------------------------------
	// Triangulate points.
	//---------------------------------------------------------------------------------------------------
	cv::Mat Q;
	cv::triangulatePoints(proj0, proj1, camUndistortedPoints[0], camUndistortedPoints[1], Q);

	Job->massModelTriangulatedPoints.clear();
	for (int i = 0; i < Q.size().width; ++i) {
		float w = Q.at<float>(3, i);
		float x = Q.at<float>(0, i) / w;
		float y = Q.at<float>(1, i) / w;
		float z = Q.at<float>(2, i) / w;

		Job->massModelTriangulatedPoints.push_back(vec3(x, y, z));

		//std::cout << i << ": " << x << "," << y << "," << z << "," << w << "\n";
	}

	//for (size_t i = 0; i < triPoints.size(); ++i) {
		//Job->triangulatedCalibModelPoints.push_back(vec3(triPoints[i].x, triPoints[i].y, triPoints[i].z));
	//}

	//std::cout << triPoints.size() << "\n";

	/*for (int i = 0; i < triPoints.size().width; ++i) {
		float w = triPoints.at<float>(3, i);
		float x = triPoints.at<float>(0, i);
		float y = triPoints.at<float>(1, i);
		float z = triPoints.at<float>(2, i);

		Job->massModelTriangulatedPoints.push_back(vec3(x, y, z));

		std::cout << i << ": " << x << "," << y << "," << z << "," << w << "\n";
	}*/
}

void _platformSaveStereoCalibImage(ldiImage* Image0, ldiImage* Image1, int X, int Y, int Z, int C, int A) {
	std::string path = "../cache/volume_calib_space/"
		+ std::to_string(X) + "_" + std::to_string(Y) + "_" + std::to_string(Z) + "_" + std::to_string(C) + "_" + std::to_string(A)
		+ ".dat";
	
	FILE* file;
	fopen_s(&file, path.c_str(), "wb");

	fwrite(&X, sizeof(int), 1, file);
	fwrite(&Y, sizeof(int), 1, file);
	fwrite(&Z, sizeof(int), 1, file);
	fwrite(&C, sizeof(int), 1, file);
	fwrite(&A, sizeof(int), 1, file);

	fwrite(&Image0->width, sizeof(int), 1, file);
	fwrite(&Image0->height, sizeof(int), 1, file);
	fwrite(Image0->data, 1, Image0->width * Image0->height, file);
	fwrite(Image1->data, 1, Image0->width * Image0->height, file);

	fclose(file);
}

void _platformLoadStereoCalibSampleData(ldiCalibStereoSample* Sample) {
	FILE* file;
	fopen_s(&file, Sample->path.c_str(), "rb");

	fread(&Sample->X, sizeof(int), 1, file);
	fread(&Sample->Y, sizeof(int), 1, file);
	fread(&Sample->Z, sizeof(int), 1, file);
	fread(&Sample->C, sizeof(int), 1, file);
	fread(&Sample->A, sizeof(int), 1, file);

	int width;
	int height;
	fread(&width, 4, 1, file);
	fread(&height, 4, 1, file);

	for (int i = 0; i < 2; ++i) {
		Sample->frames[i].data = new uint8_t[width * height];
		Sample->frames[i].width = width;
		Sample->frames[i].height = height;
	
		fread(Sample->frames[i].data, width * height, 1, file);
	}

	Sample->imageLoaded = true;

	fclose(file);
}

void _platformFreeStereoCalibImages(ldiCalibStereoSample* Sample) {
	if (Sample->imageLoaded) {
		delete[] Sample->frames[0].data;
		delete[] Sample->frames[1].data;
		Sample->imageLoaded = false;
	}
}


void platformWorkerThreadJobComplete(ldiPlatform* Platform) {
	delete Platform->job;
	Platform->jobState = 0;
	Platform->jobExecuting = false;
}

bool _platformCaptureCalibration(ldiPlatform* Platform) {
	ldiApp* appContext = Platform->appContext;
	ldiPanther* panther = &Platform->panther;

	// TODO: Make sure directories exist for saving results.

	// TODO: Set cam to mode 0. Prepare other hardware.
	// TODO: Set starting point.

	// Account for backlash, always approach +/CW.

	// 8000 steps per cm.

	// C: start at 13000

	// Extents.

	// -6 to 6cm = -48000 to 48000
	// 4 captures, each step is 32000

	// Pre arm camera.
	/*hawkClearWaitPacket(&Platform->hawks[0]);
	hawkSetMode(&Platform->hawks[0], CCM_AVERAGE);
	std::cout << "Wait for gather " << _getTime(appContext) << "\n";
	hawkWaitForPacket(&Platform->hawks[0], HO_AVERAGE_GATHERED);
	std::cout << "Got gather " << _getTime(appContext) << "\n";*/
	//hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
	//std::cout << "Got frame " << _getTime(appContext) << "\n";

	if (!pantherMoveAndWait(panther, PA_X, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_X, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Y, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Y, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Z, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Z, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

	int posX = -48000;
	int posY = -48000;
	int posZ = -48000;
	int posC = 13000;
	int posA = 0;

	int capPosX = 0;
	int capPosY = 0;
	int capPosZ = 0;
	int capPosC = 0;
	int capPosA = 0;
	
	for (int iX = 0; iX < 4; ++iX) {
		posX = -48000 + iX * 32000;
		if (!pantherMoveAndWait(panther, PA_X, posX, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Y, -49000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Y, -48000, 0.0f)) { return false; }

		Sleep(300);

		for (int iY = 0; iY < 4; ++iY) {
			posY = -48000 + iY * 32000;
			if (!pantherMoveAndWait(panther, PA_Y, posY, 0.0f)) { return false; }

			if (!pantherMoveAndWait(panther, PA_Z, -49000, 0.0f)) { return false; }
			if (!pantherMoveAndWait(panther, PA_Z, -48000, 0.0f)) { return false; }

			Sleep(300);

			for (int iZ = 0; iZ < 4 + 1; ++iZ) {
				if (iZ < 4) {
					posZ = -48000 + iZ * 32000;
					if (!pantherMoveAndWait(panther, PA_Z, posZ, 0.0f)) { return false; }
				}

				if (iZ != 0) {
					hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
					hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

					std::cout << "Got frame " << _getTime(appContext) << "\n";

					{
						std::unique_lock<std::mutex> lock0(Platform->hawks[0].valuesMutex);
						std::unique_lock<std::mutex> lock1(Platform->hawks[1].valuesMutex);
						
						ldiImage frame0 = {};
						frame0.data = Platform->hawks[0].frameBuffer;
						frame0.width = Platform->hawks[0].imgWidth;
						frame0.height = Platform->hawks[0].imgHeight;

						ldiImage frame1 = {};
						frame1.data = Platform->hawks[1].frameBuffer;
						frame1.width = Platform->hawks[1].imgWidth;
						frame1.height = Platform->hawks[1].imgHeight;

						if (frame0.width != frame1.width || frame0.height != frame1.height) {
							std::cout << "Can't save calib image due to different sizes\n";
							return false;
						}

						_platformSaveStereoCalibImage(&frame0, &frame1, capPosX, capPosY, capPosZ, capPosC, capPosA);
					}
				}

				if (iZ < 4) {
					hawkClearWaitPacket(&Platform->hawks[0]);
					hawkSetMode(&Platform->hawks[0], CCM_AVERAGE);

					hawkClearWaitPacket(&Platform->hawks[1]);
					hawkSetMode(&Platform->hawks[1], CCM_AVERAGE);

					hawkWaitForPacket(&Platform->hawks[0], HO_AVERAGE_GATHERED);
					hawkWaitForPacket(&Platform->hawks[1], HO_AVERAGE_GATHERED);
					
					std::cout << "Capture at " << posX << ", " << posY << ", " << posZ << ", " << posC << ", " << posA << "\n";

					capPosX = posX;
					capPosY = posY;
					capPosZ = posZ;
					capPosC = posC;
					capPosA = posA;
				}

				if (Platform->jobCancel) {
					return false;
				}
			}
		}
	}

	// Final frame.
	/*hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
	std::cout << "Got frame " << _getTime(appContext) << "\n";
	std::cout << "Capture at " << posX << ", " << posY << ", " << posZ << ", " << posC << ", " << posA << "\n";*/

	

	// Prepare the hawks.
	// TODO: Set exposures?
	//hawkSetMode(&Platform->hawks[0], CCM_WAIT);
	//hawkSetMode(&Platform->hawks[1], CCM_WAIT);

	

	//for (int i = 0; i < 3; ++i) {
	//	if (Platform->jobCancel) {
	//		break;
	//	}

	//	std::cout << "Start move " << i << "\n";
	//	double t0 = _getTime(appContext);
	//	pantherSendMoveRelativeCommand(panther, PA_X, 8000, 30.0f);
	//	if (!pantherWaitForExecutionComplete(panther)) {
	//		break;
	//	}

	//	t0 = _getTime(appContext) - t0;
	//	std::cout << "Move: " << (t0 * 1000.0) << " ms\n";

	//	t0 = _getTime(appContext);
	//	Sleep(200);
	//	t0 = _getTime(appContext) - t0;
	//	std::cout << "Sleep: " << (t0 * 1000.0) << " ms\n";

	//	// Trigger camera start.
	//	t0 = _getTime(appContext);

	//	ldiProtocolMode setMode;
	//	setMode.header.packetSize = sizeof(ldiProtocolMode) - 4;
	//	setMode.header.opcode = 1;
	//	setMode.mode = 2;

	//	networkSend(&Platform->appContext->server, (uint8_t*)&setMode, sizeof(ldiProtocolMode));

	//	// Wait until next frame recvd.
	//	//imageInspectorWaitForCameraFrame(Platform->appContext->imageInspector);
	//	t0 = _getTime(appContext) - t0;
	//	std::cout << "Frame: " << (t0 * 1000.0) << " ms\n";

	//	// NOTE: the frame data here is not yet thread protected. We just assume that because we are not callling for a new frame that it won't change.
	//	t0 = _getTime(appContext);

	//	char fileName[256];
	//	sprintf_s(fileName, sizeof(fileName), "../cache/calib/data_%d.dat", i);

	//	/*int imageWidth = CAM_IMG_WIDTH;
	//	int imageHeight = CAM_IMG_HEIGHT;

	//	FILE* file;
	//	fopen_s(&file, fileName, "wb");
	//	fwrite(&imageWidth, 4, 1, file);
	//	fwrite(&imageHeight, 4, 1, file);
	//	fwrite(Platform->appContext->imageInspector->camPixelsFinal, CAM_IMG_WIDTH * CAM_IMG_HEIGHT, 1, file);
	//	fclose(file);*/

	//	t0 = _getTime(appContext) - t0;
	//	std::cout << "Save: " << (t0 * 1000.0) << " ms\n";
	//}

	return true;
}

void platformWorkerThread(ldiPlatform* Platform) {
	std::cout << "Running platform thread\n";

	while (Platform->workerThreadRunning) {
		Platform->jobCancel = false;

		std::unique_lock<std::mutex> lock(Platform->jobAvailableMutex);
		Platform->jobAvailableCondVar.wait(lock);

		while (Platform->jobExecuting) {
			ldiPanther* panther = &Platform->panther;

			switch (Platform->job->type) {
				case PJT_MOVE_AXIS: {
					ldiPlatformJobMoveAxis* job = (ldiPlatformJobMoveAxis*)Platform->job;

					if (job->relative) {
						pantherSendMoveRelativeCommand(panther, job->axisId, job->steps, job->velocity);
					} else {
						pantherSendMoveCommand(panther, job->axisId, job->steps, job->velocity);
					}

					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_DIAG: {
					pantherSendDiagCommand(panther);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_HOME: {
					pantherSendHomingCommand(panther);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_CAPTURE_CALIBRATION: {
					_platformCaptureCalibration(Platform);
					platformWorkerThreadJobComplete(Platform);
				} break;
			};
		}
	}

	std::cout << "Platform thread completed\n";
}

int platformInit(ldiApp* AppContext, ldiPlatform* Tool) {
	Tool->appContext = AppContext;

	//Tool->imageScale = 1.0f;
	//Tool->imageOffset = vec2(0.0f, 0.0f);
	//Tool->imageWidth = 1600;
	//Tool->imageHeight = 1300;

	//Tool->camera = {};
	//// NOTE: Camera location relative to machine origin: 56.0, 0, 51.0.
	//Tool->camera.position = vec3(26.0f, 51.0f, 0);
	//Tool->camera.rotation = vec3(-90, 45, 0);
	//Tool->camera.projMat = glm::perspectiveFovRH_ZO(glm::radians(42.0f), (float)Tool->imageWidth, (float)Tool->imageHeight, 0.01f, 50.0f);
	//Tool->camera.fov = 42.0f;

	Tool->mainCamera.position = vec3(-40, 60, 20);
	Tool->mainCamera.rotation = vec3(55, 40, 0);
	Tool->mainCamera.fov = 60.0f;

	horseInit(&Tool->horse);

	if (!pantherInit(AppContext, &Tool->panther)) {
		std::cout << "Could not init panther\n";
		return 1;
	}

	Tool->workerThread = std::thread(platformWorkerThread, Tool);

	hawkInit(&Tool->hawks[0], "169.254.72.101", 6969);
	hawkInit(&Tool->hawks[1], "169.254.73.101", 6969);

	// Set default camera intrinsics:
	cv::Mat* dc = &Tool->hawks[0].defaultCameraMat;
	*dc = cv::Mat::eye(3, 3, CV_64F);
	dc->at<double>(0, 0) = 2666.92;
	dc->at<double>(0, 1) = 0.0;
	dc->at<double>(0, 2) = 1704.76;
	dc->at<double>(1, 0) = 0.0;
	dc->at<double>(1, 1) = 2683.62;
	dc->at<double>(1, 2) = 1294.87;
	dc->at<double>(2, 0) = 0.0;
	dc->at<double>(2, 1) = 0.0;
	dc->at<double>(2, 2) = 1.0;

	cv::Mat* dd = &Tool->hawks[0].defaultDistMat;
	*dd = cv::Mat::zeros(8, 1, CV_64F);
	dd->at<double>(0) = 0.1937769949436188;
	dd->at<double>(1) = -0.3512980043888092;
	dd->at<double>(2) = 0.002319999970495701;
	dd->at<double>(3) = 0.00217368989251554;

	dc = &Tool->hawks[1].defaultCameraMat;
	*dc = cv::Mat::eye(3, 3, CV_64F);
	dc->at<double>(0, 0) = 2660.06;
	dc->at<double>(0, 1) = 0.0;
	dc->at<double>(0, 2) = 1671.81;
	dc->at<double>(1, 0) = 0.0;
	dc->at<double>(1, 1) = 2660.69;
	dc->at<double>(1, 2) = 1210.23;
	dc->at<double>(2, 0) = 0.0;
	dc->at<double>(2, 1) = 0.0;
	dc->at<double>(2, 2) = 1.0;

	dd = &Tool->hawks[1].defaultDistMat;
	*dd = cv::Mat::zeros(8, 1, CV_64F);
	dd->at<double>(0) = 0.206924;
	dd->at<double>(1) = -0.377569;
	dd->at<double>(2) = 0.000113651;
	dd->at<double>(3) = -0.000941601;

	return 0;
}

void platformDestroy(ldiPlatform* Platform) {
	Platform->workerThreadRunning = false;
	Platform->workerThread.join();
}

bool platformPrepareForNewJob(ldiPlatform* Platform) {
	if (Platform->jobExecuting) {
		std::cout << "Platform is already executing a job\n";
		return false;
	}

	return true;
}

void platformCancelJob(ldiPlatform* Platform) {
	Platform->jobCancel = true;

	// TODO: Cancel remaining queued jobs.
}

void platformQueueJob(ldiPlatform* Platform, ldiPlatformJobHeader* Job) {
	Platform->job = Job;
	Platform->jobState = 1;
	Platform->jobCancel = false;
	Platform->jobExecuting = true;
	Platform->jobAvailableCondVar.notify_all();
}

bool platformQueueJobMoveAxis(ldiPlatform* Platform, ldiPantherAxis AxisId, int Steps, float Velocity, bool Relative) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobMoveAxis* job = new ldiPlatformJobMoveAxis();
	job->header.type = PJT_MOVE_AXIS;
	job->axisId = AxisId;
	job->steps = Steps;
	job->velocity = Velocity;
	job->relative = Relative;

	platformQueueJob(Platform, &job->header);

	return true;
}

bool platformQueueJobHome(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_HOME;

	platformQueueJob(Platform, job);

	return true;
}

bool platformQueueJobCaptureCalibration(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_CAPTURE_CALIBRATION;

	platformQueueJob(Platform, job);

	return true;
}

void platformMainRender(ldiPlatform* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->mainViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
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
	// Calibration stuff.
	//----------------------------------------------------------------------------------------------------
	{
		ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;
		std::vector<vec3>* modelPoints = &calibContext->bundleAdjustResult.modelPoints;

		for (size_t i = 0; i < modelPoints->size(); ++i) {
			vec3 point = (*modelPoints)[i];
			pushDebugSphere(&appContext->defaultDebug, point, 0.1, vec3(1, 0, 0), 8);

			displayTextAtPoint(&Tool->mainCamera, point, std::to_string(i), vec4(1, 1, 1, 1), TextBuffer);
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

		for (size_t i = 0; i < calibContext->bundleAdjustResult.samples.size(); ++i) {
			ldiCameraCalibSample* sample = &calibContext->bundleAdjustResult.samples[i];

			mat4 invSampleMat = sample->camLocalMat;

			// NOTE: Only difference for these is Z direction.

			invSampleMat[0][0] = invSampleMat[0][0];
			invSampleMat[0][1] = invSampleMat[0][1];
			invSampleMat[0][2] = invSampleMat[0][2];

			invSampleMat[1][0] = invSampleMat[1][0];
			invSampleMat[1][1] = invSampleMat[1][1];
			invSampleMat[1][2] = invSampleMat[1][2];

			invSampleMat[2][0] = invSampleMat[2][0];
			invSampleMat[2][1] = invSampleMat[2][1];
			invSampleMat[2][2] = invSampleMat[2][2];

			invSampleMat[3][0] = invSampleMat[3][0];
			invSampleMat[3][1] = invSampleMat[3][1];
			invSampleMat[3][2] = invSampleMat[3][2];

			//invSampleMat = glm::inverse(invSampleMat);

			renderOrigin(Tool->appContext, &Tool->mainCamera, invSampleMat, std::to_string(i), TextBuffer);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Calibration job stuff.
	//----------------------------------------------------------------------------------------------------
	{
		ldiCalibrationContext* calibContext = Tool->appContext->calibrationContext;
		ldiCalibrationJob* job = &calibContext->calibJob;

		std::vector<vec3>* modelPoints = &calibContext->calibJob.massModelTriangulatedPoints;

		for (size_t i = 0; i < modelPoints->size(); ++i) {
			vec3 point = (*modelPoints)[i] * 10.0f;
			pushDebugSphere(&appContext->defaultDebug, point, 0.01, vec3(1, 0, 0), 8);

			//displayTextAtPoint(&Tool->mainCamera, point, std::to_string(i), vec4(1, 1, 1, 1), TextBuffer);
		}

		modelPoints = &calibContext->calibJob.massModelTriangulatedPointsBundleAdjust;

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

		/*for (size_t i = 0; i < calibContext->calibJob.centeredPointGroups.size(); ++i) {
			vec3 col(0, 0.5, 0);
			srand(i);
			rand();
			col = getRandomColorHighSaturation();

			for (size_t j = 0; j < calibContext->calibJob.centeredPointGroups[i].size(); ++j) {
				vec3 point = calibContext->calibJob.centeredPointGroups[i][j] * 10.0f;

				pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
			}
		}*/

		{
			ldiLineFit line = calibContext->calibJob.axisX;
			pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(1, 0, 0));
		}

		{
			ldiLineFit line = calibContext->calibJob.axisY;
			pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(0, 1, 0));
		}

		{
			ldiLineFit line = calibContext->calibJob.axisZ;
			pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(0, 0, 1));
		}

		{
			vec3 line = calibContext->calibJob.basisX;
			pushDebugLine(&appContext->defaultDebug, line * -10.0f, line * 10.0f, vec3(0.5, 0, 0));
		}

		{
			vec3 line = calibContext->calibJob.basisY;
			pushDebugLine(&appContext->defaultDebug, line * -10.0f, line * 10.0f, vec3(0, 0.5, 0));
		}

		{
			vec3 line = calibContext->calibJob.basisZ;
			pushDebugLine(&appContext->defaultDebug, line * -10.0f, line * 10.0f, vec3(0, 0, 0.5));
		}

		{
			mat4 invSampleMat = calibContext->calibJob.camVolumeMat[0];
			renderOrigin(Tool->appContext, &Tool->mainCamera, invSampleMat, "Hawk W0", TextBuffer);
			invSampleMat = calibContext->calibJob.camVolumeMat[1];
			renderOrigin(Tool->appContext, &Tool->mainCamera, invSampleMat, "Hawk W1", TextBuffer);
		}

		{
			vec3 offset = glm::f64vec3(Tool->testPosX + 16000.0, Tool->testPosY + 16000.0, Tool->testPosZ + 16000.0) * job->stepsToCm;

			for (size_t i = 0; i < job->cubePointCentroids.size(); ++i) {
				//if (job->cubePointCounts[i] == 0) {
					//continue;
				//}

				vec3 point = job->cubePointCentroids[i];
				point += offset.x * job->axisX.direction;
				point += offset.y * job->axisY.direction;
				point += offset.z * -job->axisZ.direction;

				pushDebugSphere(&appContext->defaultDebug, point, 0.02, vec3(0, 1, 1), 8);

				int id = i;
				int sampleId = id / (9 * 6);
				int globalId = id % (9 * 6);
				int boardId = globalId / 9;
				int pointId = globalId % 9;

				displayTextAtPoint(&Tool->mainCamera, point, std::to_string(globalId), vec4(1, 1, 1, 1), TextBuffer);
			}
		}

		{
			std::vector<vec3>* modelPoints = &calibContext->calibJob.baIndvCubePoints;

			for (size_t i = 0; i < modelPoints->size(); ++i) {
				vec3 point = (*modelPoints)[i];

				pushDebugSphere(&appContext->defaultDebug, point, 0.02, vec3(0, 1, 1), 8);
				displayTextAtPoint(&Tool->mainCamera, point, std::to_string(i), vec4(1, 1, 1, 0.5), TextBuffer);
			}

			//for (size_t i = 0; i < job->baViews.size(); ++i) {
			//	ldiTransform boardTransform = {};
			//	boardTransform.world = glm::inverse(job->baViews[i]);

			//	int viewId = job->baViewIds[i] % 1000;

			//	float dist = job->baViewDist[viewId];

			//	if (dist == 0.0f) {
			//		continue;
			//	}

			//	if (job->baViewIds[i] < 1000) {
			//		boardTransform.world[3] = vec4(10, 0, 0, 1.0);
			//		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "0", TextBuffer);
			//	} else {
			//		boardTransform.world[3] = vec4(10, 0, dist, 1.0);
			//		renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "1", TextBuffer);
			//	}

			//	//pushDebugBox(&appContext->defaultDebug, worldOrigin, vec3(0.05f, 0.05f, 0.05f), targetFaceColor[point->boardId]);
			//}
			for (size_t i = 0; i < job->baStereoPairs.size(); ++i) {
				ldiStereoPair* pair = &job->baStereoPairs[i];

				{
					ldiTransform boardTransform = {};
					boardTransform.world = pair->cam0world;
					renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, std::to_string(pair->sampleId), TextBuffer);
					pushDebugSphere(&appContext->defaultDebug, pair->cam0world[3], 0.1, vec3(1, 0, 0), 8);

					boardTransform.world = pair->cam1world;
					renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "", TextBuffer);
					pushDebugSphere(&appContext->defaultDebug, pair->cam1world[3], 0.1, vec3(0, 1, 0), 8);
				}

				//pushDebugBox(&appContext->defaultDebug, worldOrigin, vec3(0.05f, 0.05f, 0.05f), targetFaceColor[point->boardId]);
			}

			/*{
				ldiTransform boardTransform = {};
				boardTransform.world = job->baStereoCamWorld[0];
				renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "Hawk 0", TextBuffer);
				pushDebugSphere(&appContext->defaultDebug, boardTransform.world[3], 0.1, vec3(1, 0, 1), 8);
				
				boardTransform.world = job->baStereoCamWorld[1];
				renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &boardTransform, "Hawk 1", TextBuffer);
				pushDebugSphere(&appContext->defaultDebug, boardTransform.world[3], 0.1, vec3(0, 0, 1), 8);

			}*/
		}

		{	
			std::vector<vec3>* modelPoints = &job->stCubePoints;
			for (size_t cubeIter = 0; cubeIter < job->stCubeWorlds.size(); ++cubeIter) {
				srand(cubeIter);
				rand();
				vec3 col = getRandomColorHighSaturation();

				for (size_t i = 0; i < modelPoints->size(); ++i) {
					vec3 point = job->stCubeWorlds[cubeIter] * vec4((*modelPoints)[i], 1.0f);

					pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
					//displayTextAtPoint(&Tool->mainCamera, point, std::to_string(i), vec4(1, 1, 1, 0.5), TextBuffer);
				}
			}

			for (size_t i = 0; i < calibContext->calibJob.stBasisXPoints.size(); ++i) {
				vec3 col(0, 0.5, 0);
				srand(i);
				rand();
				col = getRandomColorHighSaturation();

				vec3 point = calibContext->calibJob.stBasisXPoints[i];
				pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
			}

			for (size_t i = 0; i < calibContext->calibJob.stBasisYPoints.size(); ++i) {
				vec3 col(0, 0.5, 0);
				srand(i);
				rand();
				col = getRandomColorHighSaturation();

				vec3 point = calibContext->calibJob.stBasisYPoints[i];
				pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
			}

			for (size_t i = 0; i < calibContext->calibJob.stBasisZPoints.size(); ++i) {
				vec3 col(0, 0.5, 0);
				srand(i);
				rand();
				col = getRandomColorHighSaturation();

				vec3 point = calibContext->calibJob.stBasisZPoints[i];
				pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
			}
		}
	}

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

		//mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		//viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		//mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		//ldiTransform mvCameraTransform = {};
		//mvCameraTransform.world = glm::inverse(camViewMat);
		//renderTransformOrigin(Tool->appContext, &Tool->mainCamera, &mvCameraTransform, "Camera", TextBuffer);

		//// Render camera frustum.
		//mat4 projViewMat = Tool->camera.projMat * camViewMat;
		//mat4 invProjViewMat = glm::inverse(projViewMat);

		//vec4 f0 = invProjViewMat * vec4(-1, -1, 0, 1); f0 /= f0.w;
		//vec4 f1 = invProjViewMat * vec4(1, -1, 0, 1); f1 /= f1.w;
		//vec4 f2 = invProjViewMat * vec4(1, 1, 0, 1); f2 /= f2.w;
		//vec4 f3 = invProjViewMat * vec4(-1, 1, 0, 1); f3 /= f3.w;

		//vec4 f4 = invProjViewMat * vec4(-1, -1, 1, 1); f4 /= f4.w;
		//vec4 f5 = invProjViewMat * vec4(1, -1, 1, 1); f5 /= f5.w;
		//vec4 f6 = invProjViewMat * vec4(1, 1, 1, 1); f6 /= f6.w;
		//vec4 f7 = invProjViewMat * vec4(-1, 1, 1, 1); f7 /= f7.w;

		//pushDebugLine(&appContext->defaultDebug, f0, f4, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f1, f5, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f2, f6, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f3, f7, vec3(1, 0, 1));

		//pushDebugLine(&appContext->defaultDebug, f0, f1, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f1, f2, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f2, f3, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f3, f0, vec3(1, 0, 1));

		//pushDebugLine(&appContext->defaultDebug, f4, f5, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f5, f6, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f6, f7, vec3(1, 0, 1));
		//pushDebugLine(&appContext->defaultDebug, f7, f4, vec3(1, 0, 1));
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

void platformShowUi(ldiPlatform* Tool) {
	ldiApp* appContext = Tool->appContext;
	static float f = 0.0f;
	static int counter = 0;

	// TODO: Move update somewhere sensible.
	// Copy values from panther.
	Tool->panther.dataLockMutex.lock();
	Tool->positionX = Tool->panther.positionX;
	Tool->positionY = Tool->panther.positionY;
	Tool->positionZ = Tool->panther.positionZ;
	Tool->positionC = Tool->panther.positionC;
	Tool->positionA = Tool->panther.positionA;
	Tool->homed = Tool->panther.homed;
	Tool->panther.dataLockMutex.unlock();

	Tool->horse.x = Tool->positionX / 10.0f - 7.5f;
	Tool->horse.y = Tool->positionY / 10.0f - 0.0f;
	Tool->horse.z = Tool->positionZ / 10.0f - 0.0f;
	horseUpdate(&Tool->horse);

	//ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->WorkSize.y));
	//ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x, ImGui::GetMainViewport()->WorkPos.y), 0, ImVec2(1, 0));

	//ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
	ImGui::Begin("Platform controls", 0, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Test thing");
	ImGui::SliderInt("TextPosX", &Tool->testPosX, -48000, 48000);
	ImGui::SliderInt("TextPosY", &Tool->testPosY, -48000, 48000);
	ImGui::SliderInt("TextPosZ", &Tool->testPosZ, -48000, 48000);
	ImGui::Separator();

	ImGui::Text("Connection");
	ImGui::BeginDisabled(Tool->panther.serialPortConnected);
	/*char ipBuff[] = "192.168.0.50";
	int port = 5000;
	ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
	ImGui::InputInt("Port", &port);*/

	char comPortPath[7] = "COM4";
	ImGui::InputText("COM path", comPortPath, sizeof(comPortPath));

	ImGui::EndDisabled();

	if (Tool->panther.serialPortConnected) {
		if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
			pantherDisconnect(&Tool->panther);
		};
	} else {
		if (ImGui::Button("Connect", ImVec2(-1, 0))) {
			if (pantherConnect(&Tool->panther, "\\\\.\\COM4")) {
				pantherSendDiagCommand(&Tool->panther);
			}
		}
	}

	ImGui::Separator();

	ImGui::BeginDisabled(!Tool->panther.serialPortConnected);
	ImGui::Text("Platform status");
	ImGui::PushFont(Tool->appContext->fontBig);

	const char* labelStrs[] = {
		"Disconnected",
		"Connected",
		"Idle",
		"Cancelling job",
		"Running job",
		"Idle - Not homed",
	};

	ImColor labelColors[] = {
		ImColor(0, 0, 0, 50),
		ImColor(184, 179, 55, 255),
		ImColor(61, 184, 55, 255),
		ImColor(186, 28, 28, 255),
		ImColor(179, 72, 27, 255),
		ImColor(184, 179, 55, 255),
	};

	int labelIndex = 0;

	if (Tool->panther.serialPortConnected) {
		labelIndex = 1;

		if (Tool->jobExecuting) {
			if (Tool->jobCancel) {
				labelIndex = 3;
			} else {
				labelIndex = 4;
			}
		} else {
			if (Tool->homed) {
				labelIndex = 2;
			} else {
				labelIndex = 5;
			}
		}
	}

	float startX = ImGui::GetCursorPosX();
	float availX = ImGui::GetContentRegionAvail().x;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImGuiStyle style = ImGui::GetStyle();

	{
		ImVec2 bannerCursorStart = ImGui::GetCursorPos();
		ImVec2 bannerMin = ImGui::GetCursorScreenPos() + ImVec2(0.0f, style.FramePadding.y);
		ImVec2 bannerMax = bannerMin + ImVec2(availX, 32);
		int bannerHeight = 32;

		drawList->AddRectFilled(bannerMin, bannerMax, labelColors[labelIndex]);

		ImVec2 bannerCursorEnd = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + bannerHeight + style.FramePadding.y * 2);

		ImVec2 labelSize = ImGui::CalcTextSize(labelStrs[labelIndex]);
		ImGui::SetCursorPos(bannerCursorStart + ImVec2(availX / 2.0f - labelSize.x / 2.0f, bannerHeight / 2.0f - labelSize.y / 2.0f + style.FramePadding.y));
		ImGui::Text(labelStrs[labelIndex]);
		ImGui::SetCursorPos(bannerCursorEnd);
		ImGui::PopFont();
	}

	ImGui::BeginDisabled(!Tool->jobExecuting || Tool->jobCancel);

	if (ImGui::Button("Cancel job", ImVec2(-1, 0))) {
		platformCancelJob(Tool);
	}
	
	ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Text("Position");

	/*{
		ImVec2 bannerCursorStart = ImGui::GetCursorPos();
		ImVec2 bannerMin = ImGui::GetCursorScreenPos() + ImVec2(0.0f, style.FramePadding.y);
		ImVec2 bannerMax = bannerMin + ImVec2(availX, 32);
		int bannerHeight = 32;

		drawList->AddRectFilled(bannerMin, bannerMax, labelColors[labelIndex]);

		ImVec2 bannerCursorEnd = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + bannerHeight + style.FramePadding.y * 2);

		ImVec2 labelSize = ImGui::CalcTextSize(labelStrs[labelIndex]);
		ImGui::SetCursorPos(bannerCursorStart + ImVec2(availX / 2.0f - labelSize.x / 2.0f, bannerHeight / 2.0f - labelSize.y / 2.0f + style.FramePadding.y));
		ImGui::Text(labelStrs[labelIndex]);
		ImGui::SetCursorPos(bannerCursorEnd);
		ImGui::PopFont();
	}*/

	ImGui::PushFont(Tool->appContext->fontBig);
	ImGui::SetCursorPosX(startX);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: %.2f", Tool->positionX);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3);
	ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: %.2f", Tool->positionY);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 2);
	ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: %.2f", Tool->positionZ);
	
	ImGui::SetCursorPosX(startX + availX / 3 * 0);
	ImGui::TextColored(ImVec4(0.921f, 0.921f, 0.125f, 1.0f), "A: %.2f", Tool->positionA);
	ImGui::SameLine();
	ImGui::SetCursorPosX(startX + availX / 3 * 1);
	ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.921f, 1.0f), "C: %.2f", Tool->positionC);
	ImGui::PopFont();

	ImGui::BeginDisabled(Tool->jobExecuting);
	
	ImGui::Separator();

	float distance = 1;
	ImGui::InputFloat("Distance", &distance);

	static int steps = 10000;
	ImGui::InputInt("Steps", &steps);
	static float velocity = 30.0f;
	ImGui::InputFloat("Velocity", &velocity);

	ImGui::BeginDisabled(!Tool->homed);

	if (ImGui::Button("-X", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_X, -steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("-Y", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Y, -steps, velocity, true);
	}
	ImGui::SameLine();		
	if (ImGui::Button("-Z", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Z, -steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("-C", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_C, -steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("-A", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_A, -steps, velocity, true);
	}

	if (ImGui::Button("+X", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_X, steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Y", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Y, steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("+Z", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Z, steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("+C", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_C, steps, velocity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("+A", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_A, steps, velocity, true);
	}

	static int absoluteStep = 0;
	ImGui::InputInt("Absolute step", &absoluteStep);
	if (ImGui::Button("X", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_X, absoluteStep, 0, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("Y", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Y, absoluteStep, 0, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("Z", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_Z, absoluteStep, 0, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("C", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_C, absoluteStep, 0, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("A", ImVec2(32, 32))) {
		platformQueueJobMoveAxis(Tool, PA_A, absoluteStep, 0, false);
	}

	ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Text("Jobs");

	if (ImGui::Button("Find home", ImVec2(-1, 0))) {
		platformQueueJobHome(Tool);
	}

	ImGui::BeginDisabled(!Tool->homed);

	//ImGui::Button("Go home", ImVec2(-1, 0));

	if (ImGui::Button("Capture calibration", ImVec2(-1, 0))) {
		platformQueueJobCaptureCalibration(Tool);
	}

	ImGui::EndDisabled();

	ImGui::Button("Start laser preview", ImVec2(-1, 0));

	ImGui::EndDisabled();
	ImGui::EndDisabled();

	ImGui::End();

	//----------------------------------------------------------------------------------------------------
	// Platform 3D view.
	//----------------------------------------------------------------------------------------------------
	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Platform 3D view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held

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
		platformMainRender(Tool, viewSize.x, viewSize.y, &textBuffer);
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