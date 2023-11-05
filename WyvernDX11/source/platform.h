#pragma once

enum ldiPlatformJobType {
	PJT_MOVE_AXIS,
	PJT_DIAG,
	PJT_HOME,
	PJT_CAPTURE_CALIBRATION,
	PJT_CAPTURE_SCANNER_CALIBRATION,
	PJT_SET_SCAN_LASER_STATE,
	PJT_SCAN,
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

struct ldiPlatformJobSetScanLaserState {
	ldiPlatformJobHeader header;
	bool Enabled;
};

struct ldiPlatformJobScan {
	ldiPlatformJobHeader header;
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
	int							positionX;
	int							positionY;
	int							positionZ;
	int							positionC;
	int							positionA;
	bool						homed;

	float						mmPerStepX;

	// Dual machine vision cameras.
	ldiHawk						hawks[2];

	// Motion/Galvo/Laser control.
	ldiPanther					panther;

	int							testPosX;
	int							testPosY;
	int							testPosZ;
	int							testPosC;
	int							testPosA;
	
	ldiCalibCube				defaultCube;

	ldiRenderModel				cubeModel;

	bool						showDefaultCube = false;
	bool						showMachineFrame = false;
	bool						showCalibCube = true;
	bool						showCalibCubeVolume = false;
	bool						showCalibVolumeBasis = false;
	bool						showCalibCubeFaces = false;
	bool						showScanPlane = false;
	bool						liveAxisUpdate = true;

	bool						showSourceModelShaded = true;
	bool						showSourceModelWireframe = false;
	bool						showQuadMeshDebug = true;
	bool						showQuadMeshWireframe = false;
	bool						showQuadMeshWhite = false;
	vec4						quadMeshCanvasColor = { 1.0f, 1.0f, 1.0f, 1.00f };

	std::mutex					liveScanPointsMutex;
	bool						liveScanPointsUpdated;
	std::vector<vec3>			liveScanPoints;
	std::vector<vec3>			copyOfLiveScanPoints;

	ldiScan						scan = {};

	bool						scanShowPointCloud = true;
	bool						scanUseWorkTrans = true;
	float						pointWorldSize = 0.01f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	vec3						scanBoundsMin = vec3(-5, -5, -5);
	vec3						scanBoundsMax = vec3(5, 5, 5);
};

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
	
	// NOTE: Account for backlash, always approach +/CW.

	// 8000 steps per cm.

	// C: start at 13000

	// Extents:
	// -6 to 6cm = -48000 to 48000
	// 4 captures, each step is 32000
	// 7 captures, each step is 16000
	// -48000, -32000, -16000, 0, 16000, 32000, 48000
	// 4x4x4 = 64
	// 7x7x7 = 343

	// Pre arm camera.
	/*hawkClearWaitPacket(&Platform->hawks[0]);
	hawkSetMode(&Platform->hawks[0], CCM_AVERAGE);
	std::cout << "Wait for gather " << getTime() << "\n";
	hawkWaitForPacket(&Platform->hawks[0], HO_AVERAGE_GATHERED);
	std::cout << "Got gather " << getTime() << "\n";*/
	//hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
	//std::cout << "Got frame " << getTime() << "\n";

	int imgId = 0;

	int posX = 0;
	int posY = 0;
	int posZ = 0;
	int posC = 13000;
	int posA = 0;

	int capPosX = 0;
	int capPosY = 0;
	int capPosZ = 0;
	int capPosC = 0;
	int capPosA = 0;

	//----------------------------------------------------------------------------------------------------
	// Phase 0.
	//----------------------------------------------------------------------------------------------------
	{
		if (!pantherMoveAndWait(panther, PA_X, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_X, 0, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Y, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Y, 0, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Z, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Z, -0, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_A, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_A, 0, 0.0f)) { return false; }

		Sleep(1000);

		hawkClearWaitPacket(&Platform->hawks[0]);
		hawkSetMode(&Platform->hawks[0], CCM_AVERAGE);

		hawkClearWaitPacket(&Platform->hawks[1]);
		hawkSetMode(&Platform->hawks[1], CCM_AVERAGE);

		hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
		hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

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
		
			calibSaveStereoCalibImage(&frame0, &frame1, 0, 0, 0, 13000, 0, 0, imgId);
			++imgId;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Phase 1.
	//----------------------------------------------------------------------------------------------------
	if (!pantherMoveAndWait(panther, PA_X, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_X, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Y, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Y, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Z, -49000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Z, -48000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_A, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_A, 0, 0.0f)) { return false; }

	posX = -48000;
	posY = -48000;
	posZ = -48000;
	posC = 13000;
	posA = 0;

	for (int iX = 0; iX < 7; ++iX) {
		posX = -48000 + iX * 16000;
		if (!pantherMoveAndWait(panther, PA_X, posX, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Y, -49000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Y, -48000, 0.0f)) { return false; }

		Sleep(300);

		for (int iY = 0; iY < 7; ++iY) {
			posY = -48000 + iY * 16000;
			if (!pantherMoveAndWait(panther, PA_Y, posY, 0.0f)) { return false; }

			if (!pantherMoveAndWait(panther, PA_Z, -49000, 0.0f)) { return false; }
			if (!pantherMoveAndWait(panther, PA_Z, -48000, 0.0f)) { return false; }

			Sleep(300);

			for (int iZ = 0; iZ < 7 + 1; ++iZ) {
				if (iZ < 7) {
					posZ = -48000 + iZ * 16000;
					if (!pantherMoveAndWait(panther, PA_Z, posZ, 0.0f)) { return false; }
				}

				if (iZ != 0) {
					hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
					hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

					std::cout << "Got frame " << getTime() << "\n";

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

						calibSaveStereoCalibImage(&frame0, &frame1, capPosX, capPosY, capPosZ, capPosC, capPosA, 1, imgId);
						++imgId;
					}
				}

				if (iZ < 7) {
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

	//----------------------------------------------------------------------------------------------------
	// Phase 2.
	//----------------------------------------------------------------------------------------------------
	if (!pantherMoveAndWait(panther, PA_X, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_X, 0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Y, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Y, 0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Z, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Z, -0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_A, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_A, 0, 0.0f)) { return false; }

	posX = 0;
	posY = 0;
	posZ = 0;
	posC = 13000;
	posA = 0;

	for (int iC = 0; iC < 60 + 1; ++iC) {
		if (iC < 60) {
			int stepInc = (32 * 200 * 30) / 60;
			posC = 13000 + iC * stepInc;
			if (!pantherMoveAndWait(panther, PA_C, posC, 0.0f)) { return false; }
		}

		if (iC != 0) {
			hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
			hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

			std::cout << "Got frame " << getTime() << "\n";

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

				calibSaveStereoCalibImage(&frame0, &frame1, capPosX, capPosY, capPosZ, capPosC, capPosA, 2, imgId);
				++imgId;
			}
		}

		if (iC < 60) {
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

	//----------------------------------------------------------------------------------------------------
	// Phase 3.
	//----------------------------------------------------------------------------------------------------
	if (!pantherMoveAndWait(panther, PA_X, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_X, 0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Y, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Y, 0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Z, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Z, -0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_A, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_A, 0, 0.0f)) { return false; }

	posX = 0;
	posY = 0;
	posZ = 0;
	posC = 13000;
	posA = 0;

	for (int iA = 0; iA < 61 + 1; ++iA) {
		if (iA < 61) {
			//int stepInc = ((32 * 200 * 90) / 2) / 60;
			int stepInc = 4300; // Ends up at 258000. (60 steps)
			posA = iA * stepInc;
			if (!pantherMoveAndWait(panther, PA_A, posA, 0.0f)) { return false; }
		}

		if (iA != 0) {
			hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
			hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

			std::cout << "Got frame " << getTime() << "\n";

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

				calibSaveStereoCalibImage(&frame0, &frame1, capPosX, capPosY, capPosZ, capPosC, capPosA, 3, imgId);
				++imgId;
			}
		}

		if (iA < 61) {
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

	return true;
}

bool _platformCaptureScannerCalibration(ldiPlatform* Platform) {
	ldiApp* appContext = Platform->appContext;
	ldiPanther* panther = &Platform->panther;

	//----------------------------------------------------------------------------------------------------
	// Phase 0.
	//----------------------------------------------------------------------------------------------------
	{
		pantherSendScanLaserStateCommand(panther, true);
		if (!pantherWaitForExecutionComplete(panther)) { return false; }

		if (!pantherMoveAndWait(panther, PA_X, -13700, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_X, -12700, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Y, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Y, 0, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_Z, -49000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_Z, -48000, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

		if (!pantherMoveAndWait(panther, PA_A, -1000, 0.0f)) { return false; }
		if (!pantherMoveAndWait(panther, PA_A, 0, 0.0f)) { return false; }

		Sleep(300);

		int imgId = 0;

		int posX = -12700;
		int posY = 0;
		int posZ = 0;
		int posC = 13000;
		int posA = 0;

		int capPosX = 0;
		int capPosY = 0;
		int capPosZ = 0;
		int capPosC = 0;
		int capPosA = 0;

		// -48000
		// 48000
		// 96000

		for (int iZ = 0; iZ < 61 + 1; ++iZ) {
			if (iZ < 61) {
				int stepInc = (96000) / 60;
				posZ = -48000 + iZ * stepInc;
				if (!pantherMoveAndWait(panther, PA_Z, posZ, 0.0f)) { return false; }
			}

			if (iZ != 0) {
				hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);
				hawkWaitForPacket(&Platform->hawks[1], HO_FRAME);

				std::cout << "Got frame " << getTime() << "\n";

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

					calibSaveStereoCalibImage(&frame0, &frame1, capPosX, capPosY, capPosZ, capPosC, capPosA, 0, imgId, "scanner_calib");
					++imgId;
				}
			}

			if (iZ < 61) {
				hawkClearWaitPacket(&Platform->hawks[0]);
				hawkSetMode(&Platform->hawks[0], CCM_AVERAGE_NO_FLASH);

				hawkClearWaitPacket(&Platform->hawks[1]);
				hawkSetMode(&Platform->hawks[1], CCM_AVERAGE_NO_FLASH);

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

		pantherSendScanLaserStateCommand(panther, false);
		if (!pantherWaitForExecutionComplete(panther)) { return false; }
	}

	return true;
}

bool _platformScan2(ldiPlatform* Platform, ldiPlatformJobScan* Job) {
	ldiApp* appContext = Platform->appContext;
	ldiPanther* panther = &Platform->panther;

	{
		std::unique_lock<std::mutex> lock(Platform->liveScanPointsMutex);
		Platform->liveScanPoints.clear();
		Platform->liveScanPointsUpdated = true;
	}

	// Set up cameras and other machine state.
	pantherSendScanLaserStateCommand(panther, true);
	if (!pantherWaitForExecutionComplete(panther)) { return false; }

	if (!pantherMoveAndWait(panther, PA_X, -9000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_X, -8000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Y, -1000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Y, 0, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_Z, -29000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_Z, -28000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_C, 12000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_C, 13000, 0.0f)) { return false; }

	if (!pantherMoveAndWait(panther, PA_A, 103000, 0.0f)) { return false; }
	if (!pantherMoveAndWait(panther, PA_A, 104000, 0.0f)) { return false; }

	int posX = -8000;
	int posY = 0;
	int posZ = -28000;
	int posC = 13000;
	int posA = 104000;

	Sleep(300);

	int aSteps = 2;
	int xSteps = 5;
	int steps = 80;

	for (int iA = 0; iA < aSteps; ++iA) {
		int aStepInc = 50000;
		posA = 104000 - iA * aStepInc;
		if (!pantherMoveAndWait(panther, PA_A, posA, 0.0f)) { return false; }

		for (int iX = 0; iX < xSteps; ++iX) {
			// Note account for backlash reset.
			if (iX == 0) {
				posX = -9000;
				if (!pantherMoveAndWait(panther, PA_X, posX, 0.0f)) { return false; }
			}

			int xStepInc = 4000;
			posX = -8000 + iX * xStepInc;
			if (!pantherMoveAndWait(panther, PA_X, posX, 0.0f)) { return false; }

			for (int iC = 0; iC < steps; ++iC) {
				// Note account for backlash reset.
				if (iC == 0) {
					posC = 12000;
					if (!pantherMoveAndWait(panther, PA_C, posC, 0.0f)) { return false; }
				}

				int cStepInc = (32 * 200 * 30) / steps;
				posC = 13000 + iC * cStepInc;
				if (!pantherMoveAndWait(panther, PA_C, posC, 0.0f)) { return false; }

				Sleep(600);

				hawkClearWaitPacket(&Platform->hawks[0]);
				hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);

				std::vector<vec2> scanPoints;

				{
					std::unique_lock<std::mutex> lock0(Platform->hawks[0].valuesMutex);
			
					ldiImage frame0 = {};
					frame0.data = Platform->hawks[0].frameBuffer;
					frame0.width = Platform->hawks[0].imgWidth;
					frame0.height = Platform->hawks[0].imgHeight;

					scanPoints = computerVisionFindScanLine(frame0);
				}

				// Scan plane at machine position.
				ldiHorsePosition horsePos = {};
				horsePos.x = posX;
				horsePos.y = posY;
				horsePos.z = posZ;
				horsePos.c = posC;
				horsePos.a = posA;

				// TODO: Make sure calib context is static here, we are potentially accessing it across threads.

				mat4 workTrans = horseGetWorkTransform(&Platform->appContext->calibrationContext->calibJob, horsePos);
				mat4 invWorkTrans = glm::inverse(workTrans);

				ldiPlane scanPlane = horseGetScanPlane(&Platform->appContext->calibrationContext->calibJob, horsePos);
				scanPlane.normal = -scanPlane.normal;

				// Project points onto scan plane.
				ldiCamera camera = horseGetCamera(&Platform->appContext->calibrationContext->calibJob, horsePos, 0, 3280, 2464);

				for (size_t pIter = 0; pIter < scanPoints.size(); ++pIter) {
					ldiLine ray = screenToRay(&camera, scanPoints[pIter]);

					vec3 worldPoint;
					if (getRayPlaneIntersection(ray, scanPlane, worldPoint)) {
						//std::cout << "Ray failed " << pIter << "\n";

						// Bake point into work space.
						worldPoint = invWorkTrans * vec4(worldPoint, 1.0f);

						{
							std::unique_lock<std::mutex> lock(Platform->liveScanPointsMutex);
							Platform->liveScanPoints.push_back(worldPoint);
							Platform->liveScanPointsUpdated = true;
						}
					}
				}
			}
		}
	}

	return true;
}

bool _platformScan(ldiPlatform* Platform, ldiPlatformJobScan* Job) {
	ldiApp* appContext = Platform->appContext;
	ldiPanther* panther = &Platform->panther;

	{
		std::unique_lock<std::mutex> lock(Platform->liveScanPointsMutex);
		Platform->liveScanPoints.clear();
		Platform->liveScanPointsUpdated = true;
	}

	std::vector<ldiHorsePosition> positions;


	int cSteps = 45;
	for (int iC = 0; iC < cSteps; ++iC) {
		double cStepInc = (32 * 200 * 30) / (double)cSteps;
		int posC = (int)(iC * cStepInc);
	
		for (int iX = -13000; iX < 22000; iX += 500) {
			positions.push_back({ iX, 0, -28000, posC, 130000 });
		}
	}
	
	/*int cSteps = 360 * 2;
	for (int iC = 0; iC < cSteps; ++iC) {
		double cStepInc = (32 * 200 * 30) / (double)cSteps;
		int posC = 13000 + (int)(iC * cStepInc);

		positions.push_back({ 0, 0, 0, posC, 80000 });
	}*/

	// Set up cameras and other machine state.
	pantherSendScanLaserStateCommand(panther, true);
	if (!pantherWaitForExecutionComplete(panther)) { return false; }

	int backlashAmount = 1000;

	// Execute list of positions.
	for (size_t posIter = 0; posIter < positions.size(); ++posIter) {
		if (Platform->jobCancel) {
			return false;
		}

		ldiHorsePosition currentPos = pantherGetHorsePosition(panther);
		ldiHorsePosition targetPos = positions[posIter];

		if (currentPos.x > targetPos.x) {
			if (!pantherMoveAndWait(panther, PA_X, targetPos.x - backlashAmount, 0.0f)) { return false; }
		}
		if (!pantherMoveAndWait(panther, PA_X, targetPos.x, 0.0f)) { return false; }

		if (currentPos.y > targetPos.y) {
			if (!pantherMoveAndWait(panther, PA_Y, targetPos.y - backlashAmount, 0.0f)) { return false; }
		}
		if (!pantherMoveAndWait(panther, PA_Y, targetPos.y, 0.0f)) { return false; }

		if (currentPos.z > targetPos.z) {
			if (!pantherMoveAndWait(panther, PA_Z, targetPos.z - backlashAmount, 0.0f)) { return false; }
		}
		if (!pantherMoveAndWait(panther, PA_Z, targetPos.z, 0.0f)) { return false; }

		if (currentPos.c > targetPos.c) {
			if (!pantherMoveAndWait(panther, PA_C, targetPos.c - backlashAmount, 0.0f)) { return false; }
		}
		if (!pantherMoveAndWait(panther, PA_C, targetPos.c, 0.0f)) { return false; }

		if (currentPos.a > targetPos.a) {
			if (!pantherMoveAndWait(panther, PA_A, targetPos.a - backlashAmount, 0.0f)) { return false; }
		}
		if (!pantherMoveAndWait(panther, PA_A, targetPos.a, 0.0f)) { return false; }

		Sleep(600);

		hawkClearWaitPacket(&Platform->hawks[0]);
		hawkWaitForPacket(&Platform->hawks[0], HO_FRAME);

		std::vector<vec2> scanPoints;

		{
			std::unique_lock<std::mutex> lock0(Platform->hawks[0].valuesMutex);

			ldiImage frame0 = {};
			frame0.data = Platform->hawks[0].frameBuffer;
			frame0.width = Platform->hawks[0].imgWidth;
			frame0.height = Platform->hawks[0].imgHeight;

			scanPoints = computerVisionFindScanLine(frame0);
		}

		// TODO: Make sure calib context is static here, we are potentially accessing it across threads.
		ldiCalibrationJob* job = &Platform->appContext->calibrationContext->calibJob;

		mat4 workTrans = horseGetWorkTransform(job, targetPos);
		mat4 invWorkTrans = glm::inverse(workTrans);

		ldiPlane scanPlane = horseGetScanPlane(job, targetPos);
		scanPlane.normal = -scanPlane.normal;

		ldiCamera camera = horseGetCamera(job, targetPos, 0, 3280, 2464);

		computerVisionUndistortPoints(scanPoints, job->refinedCamMat[0], job->refinedCamDist[0]);

		// Project points onto scan plane.
		{
			std::unique_lock<std::mutex> lock(Platform->liveScanPointsMutex);

			for (size_t pIter = 0; pIter < scanPoints.size(); ++pIter) {
				ldiLine ray = screenToRay(&camera, scanPoints[pIter]);

				vec3 worldPoint;
				if (getRayPlaneIntersection(ray, scanPlane, worldPoint)) {
					// Bake point into work space.
					worldPoint = invWorkTrans * vec4(worldPoint, 1.0f);
					Platform->liveScanPoints.push_back(worldPoint);
				}
			}

			Platform->liveScanPointsUpdated = true;
		}
	}

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

				case PJT_CAPTURE_SCANNER_CALIBRATION: {
					_platformCaptureScannerCalibration(Platform);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_SET_SCAN_LASER_STATE: {
					ldiPlatformJobSetScanLaserState* job = (ldiPlatformJobSetScanLaserState*)Platform->job;
					pantherSendScanLaserStateCommand(panther, job->Enabled);
					pantherWaitForExecutionComplete(panther);
					platformWorkerThreadJobComplete(Platform);
				} break;

				case PJT_SCAN: {
					ldiPlatformJobScan* job = (ldiPlatformJobScan*)Platform->job;
					_platformScan(Platform, job);
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

	Tool->mainCamera.position = vec3(10, 10, 10);
	Tool->mainCamera.rotation = vec3(-50, 30, 0);
	Tool->mainCamera.fov = 60.0f;

	calibCubeInit(&Tool->defaultCube);

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

	//----------------------------------------------------------------------------------------------------
	// Cube.
	//----------------------------------------------------------------------------------------------------
	ldiModel cube = objLoadModel("../../assets/models/cube.obj");
	Tool->cubeModel = gfxCreateRenderModel(AppContext, &cube);

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

bool platformQueueJobCaptureScannerCalibration(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobHeader* job = new ldiPlatformJobHeader();
	job->type = PJT_CAPTURE_SCANNER_CALIBRATION;

	platformQueueJob(Platform, job);

	return true;
}

bool platformQueueJobSetScanLaserState(ldiPlatform* Platform, bool Enabled) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobSetScanLaserState* job = new ldiPlatformJobSetScanLaserState();
	job->header.type = PJT_SET_SCAN_LASER_STATE;
	job->Enabled = Enabled;

	platformQueueJob(Platform, &job->header);

	return true;
}

bool platformQueueJobScan(ldiPlatform* Platform) {
	if (!platformPrepareForNewJob(Platform)) {
		return false;
	};

	ldiPlatformJobScan* job = new ldiPlatformJobScan();
	job->header.type = PJT_SCAN;
	
	platformQueueJob(Platform, &job->header);

	return true;
}

void platformRender(ldiPlatform* Tool, ldiRenderViewBuffers* RenderBuffers, int Width, int Height, ldiCamera* Camera, std::vector<ldiTextInfo>* TextBuffer, bool Clear) {
	ldiApp* appContext = Tool->appContext;

	//----------------------------------------------------------------------------------------------------
	// Target setup.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &RenderBuffers->mainViewRtView, RenderBuffers->depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);

	vec4 bgCol(0, 0, 0, 0);

	if (Clear) {
		bgCol = Tool->mainBackgroundColor;
	}
		
	appContext->d3dDeviceContext->ClearRenderTargetView(RenderBuffers->mainViewRtView, (float*)&bgCol);
	appContext->d3dDeviceContext->ClearDepthStencilView(RenderBuffers->depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
	// Default cube definition.
	//----------------------------------------------------------------------------------------------------
	if (Tool->showDefaultCube) {
		for (size_t i = 0; i < Tool->defaultCube.points.size(); ++i) {
			pushDebugSphere(&appContext->defaultDebug, Tool->defaultCube.points[i], 0.02, vec3(1, 0, 0), 8);
			displayTextAtPoint(Camera, Tool->defaultCube.points[i], std::to_string(i), vec4(1.0f, 1.0f, 1.0f, 0.6f), TextBuffer);
		}
	}

	ldiCalibrationContext* calibContext = appContext->calibrationContext;
	ldiCalibrationJob* job = &calibContext->calibJob;
	mat4 workTrans = glm::identity<mat4>();
	mat4 workWorldMat = glm::identity<mat4>();

	ldiHorsePosition horsePos = {};
	horsePos.x = Tool->testPosX;
	horsePos.y = Tool->testPosY;
	horsePos.z = Tool->testPosZ;
	horsePos.c = Tool->testPosC;
	horsePos.a = Tool->testPosA;

	if (job->metricsCalculated) {
		workTrans = horseGetWorkTransform(job, horsePos);
	}

	if (Tool->scanUseWorkTrans) {
		workWorldMat = workTrans;
	}

	//----------------------------------------------------------------------------------------------------
	// Calibration job stuff.
	//----------------------------------------------------------------------------------------------------
	{	
		if (job->metricsCalculated) {
			//----------------------------------------------------------------------------------------------------
			// Camera positions.
			//----------------------------------------------------------------------------------------------------
			{
				for (int hawkIter = 0; hawkIter < 2; ++hawkIter) {
					mat4 camWorldMat = calibContext->calibJob.camVolumeMat[hawkIter];
					renderOrigin(appContext, Camera, camWorldMat, "Hawk W" + std::to_string(hawkIter), TextBuffer);

					vec3 refToAxis = job->axisA.origin - vec3(0.0f, 0.0f, 0.0f);
					float axisAngleDeg = (Tool->testPosA) * (360.0 / (32.0 * 200.0 * 90.0));
					mat4 axisRot = glm::rotate(mat4(1.0f), glm::radians(-axisAngleDeg), job->axisA.direction);

					camWorldMat[3] = vec4(vec3(camWorldMat[3]) - refToAxis, 1.0f);
					camWorldMat = axisRot * camWorldMat;
					camWorldMat[3] = vec4(vec3(camWorldMat[3]) + refToAxis, 1.0f);

					renderOrigin(appContext, Camera, camWorldMat, "Hawk T" + std::to_string(hawkIter), TextBuffer);
				}
			}

			//----------------------------------------------------------------------------------------------------
			// Estimated cube position.
			//----------------------------------------------------------------------------------------------------
			if (job->metricsCalculated && Tool->showCalibCube) {
				std::vector<vec3> cubePoints;
				std::vector<ldiCalibCubeSide> cubeSides;
				std::vector<vec3> cubeCorners;

				horseGetRefinedCubeAtPosition(job, horsePos, cubePoints, cubeSides, cubeCorners);

				for (size_t i = 0; i < cubePoints.size(); ++i) {
					if (i >= 18 && i <= 26) {
						continue;
					}

					pushDebugSphere(&appContext->defaultDebug, cubePoints[i], 0.005, vec3(0, 1, 1), 8);

					int id = i;
					int sampleId = id / (9 * 6);
					int globalId = id % (9 * 6);
					int boardId = globalId / 9;
					int pointId = globalId % 9;

					//displayTextAtPoint(Camera, point, std::to_string(globalId), vec4(1, 1, 1, 1), TextBuffer);
				}

				if (Tool->showCalibCubeFaces) {
					for (size_t i = 0; i < cubeSides.size(); ++i) {
						const vec3 sideCol[6] = {
							vec3(1, 0, 0),
							vec3(0, 1, 0),
							vec3(0, 0, 1),
							vec3(1, 1, 0),
							vec3(1, 0, 1),
							vec3(0, 1, 1),
						};

						pushDebugTri(&appContext->defaultDebug, cubeSides[i].corners[0], cubeSides[i].corners[1], cubeSides[i].corners[2], sideCol[i] * 0.5f);
						pushDebugTri(&appContext->defaultDebug, cubeSides[i].corners[2], cubeSides[i].corners[3], cubeSides[i].corners[0], sideCol[i] * 0.5f);
					}
				}

				for (int i = 0; i < 8; ++i) {
					pushDebugSphere(&appContext->defaultDebug, cubeCorners[i], 0.001, vec3(0, 1, 1), 8);
					displayTextAtPoint(Camera, cubeCorners[i], std::to_string(i), vec4(1, 1, 1, 1), TextBuffer);
				}
			}
			
			//----------------------------------------------------------------------------------------------------
			// Scan calibration.
			//----------------------------------------------------------------------------------------------------
			if (job->metricsCalculated) {
				if (Tool->showScanPlane) {
					{
						ldiPlane scanPlane = horseGetScanPlane(job, horsePos);
						pushDebugPlane(&appContext->defaultDebug, scanPlane.point, scanPlane.normal, 10, vec3(1.0f, 0.0f, 0.0f));
					}

					for (size_t i = 0; i < job->scanWorldPoints[0].size(); ++i) {
						vec3 point = job->scanWorldPoints[0][i];
						pushDebugSphere(&appContext->defaultDebug, point, 0.001, vec3(1, 0.4, 1), 6);
					}

					for (size_t i = 0; i < job->scanWorldPoints[1].size(); ++i) {
						vec3 point = job->scanWorldPoints[1][i];
						pushDebugSphere(&appContext->defaultDebug, point, 0.001, vec3(0.4, 1, 1), 6);
					}

					for (size_t i = 0; i < job->scanRays[0].size(); ++i) {
						ldiLine line = job->scanRays[0][i];
						pushDebugLine(&appContext->defaultDebug, line.origin, line.origin + line.direction * 50.0f, vec3(1.0f, 0.4f, 0.1f));
					}

					for (size_t i = 0; i < job->scanRays[1].size(); ++i) {
						ldiLine line = job->scanRays[1][i];
						pushDebugLine(&appContext->defaultDebug, line.origin, line.origin + line.direction * 50.0f, vec3(0.1f, 0.4f, 1.0f));
					}
				}
			}
			
			//----------------------------------------------------------------------------------------------------
			// Calibration volume.
			//----------------------------------------------------------------------------------------------------
			if (Tool->showCalibCubeVolume) {
				std::vector<vec3>* modelPoints = &job->opCube.points;
				for (size_t cubeIter = 0; cubeIter < job->cubeWorlds.size(); ++cubeIter) {
					srand(cubeIter);
					rand();
					vec3 col = getRandomColorHighSaturation();

					for (size_t i = 0; i < modelPoints->size(); ++i) {
						mat4 world = job->cubeWorlds[cubeIter];
						//world[3] = vec4(0, 0, 0, 1.0f);
						vec3 point = world * vec4((*modelPoints)[i], 1.0f);

						pushDebugSphere(&appContext->defaultDebug, point, 0.01, col, 8);
						//displayTextAtPoint(Camera, point, std::to_string(i), vec4(1, 1, 1, 0.5), TextBuffer);
					}
				}

				pushDebugSphere(&appContext->defaultDebug, job->stVolumeCenter, 0.1, vec3(1, 1, 1), 8);
			}

			//----------------------------------------------------------------------------------------------------
			// Calibration basis.
			//----------------------------------------------------------------------------------------------------
			if (Tool->showCalibVolumeBasis) {
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

				{
					ldiLine line = calibContext->calibJob.axisX;
					pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(1, 0, 0));
				}

				{
					ldiLine line = calibContext->calibJob.axisY;
					pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(0, 1, 0));
				}

				{
					ldiLine line = calibContext->calibJob.axisZ;
					pushDebugLine(&appContext->defaultDebug, line.origin - line.direction * 10.0f, line.origin + line.direction * 10.0f, vec3(0, 0, 1));
				}

				/*{
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
				}*/

				//----------------------------------------------------------------------------------------------------
				// C Axis.
				//----------------------------------------------------------------------------------------------------
				{
					for (size_t i = 0; i < job->axisCPoints.size(); ++i) {
						vec3 point = job->axisCPoints[i];

						pushDebugSphere(&appContext->defaultDebug, point, 0.005, vec3(1, 0, 0), 8);
					}

					pushDebugSphere(&appContext->defaultDebug, job->axisC.origin, 0.1, vec3(1, 0, 1), 32);
					pushDebugLine(&appContext->defaultDebug, job->axisC.origin, job->axisC.origin + job->axisC.direction * 10.0f, vec3(1, 0, 1));
				}

				//----------------------------------------------------------------------------------------------------
				// A Axis.
				//----------------------------------------------------------------------------------------------------
				{
					for (size_t i = 0; i < job->axisAPoints.size(); ++i) {
						vec3 point = job->axisAPoints[i];

						pushDebugSphere(&appContext->defaultDebug, point, 0.005, vec3(1, 0, 0), 8);
					}

					pushDebugSphere(&appContext->defaultDebug, job->axisA.origin, 0.1, vec3(1, 0, 1), 32);
					pushDebugLine(&appContext->defaultDebug, job->axisA.origin, job->axisA.origin + job->axisA.direction * 10.0f, vec3(1, 0, 1));
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Live scan points.
		//----------------------------------------------------------------------------------------------------
		if (job->scannerCalibrated) {
			pushDebugSphere(&appContext->defaultDebug, workTrans * vec4(0, 0, 0, 1.0f), 0.1, vec3(0, 1, 0), 6);
			pushDebugSphere(&appContext->defaultDebug, workTrans * vec4(1, 0, 0, 1.0f), 0.1, vec3(0, 0.5f, 0), 6);

			/*for (size_t i = 0; i < Tool->liveScanPoints.size(); ++i) {
				vec3 point = Tool->liveScanPoints[i];
				point = workTrans * vec4(point, 1.0f);
				pushDebugSphere(&appContext->defaultDebug, point, 0.005, vec3(1, 1, 1), 4);
			}*/

			// TODO: Move points update somewhere else?
			{
				bool update = false;
				{
					std::unique_lock<std::mutex> lock(Tool->liveScanPointsMutex);
					if (Tool->liveScanPointsUpdated) {
						Tool->liveScanPointsUpdated = false;
						Tool->copyOfLiveScanPoints = Tool->liveScanPoints;
						update = true;
					}
				}

				if (update) {
					std::cout << "Updating point cloud - Size: " << Tool->copyOfLiveScanPoints.size() << "\n";
					scanUpdatePoints(Tool->appContext, &Tool->scan, Tool->copyOfLiveScanPoints);
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Scan tools.
		//----------------------------------------------------------------------------------------------------
		if (Tool->scanShowPointCloud) {
			if (Tool->scan.pointCloudRenderModel.indexCount > 0) {
				{
					D3D11_MAPPED_SUBRESOURCE ms;
					appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
					ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
					constantBuffer->screenSize = vec4(Camera->viewWidth, Camera->viewHeight, 0, 0);
					constantBuffer->mvp = Camera->projViewMat;
					constantBuffer->color = vec4(0, 0, 0, 1);
					constantBuffer->view = Camera->viewMat * workWorldMat;
					constantBuffer->proj = Camera->projMat;
					appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
				}
				{
					D3D11_MAPPED_SUBRESOURCE ms;
					appContext->d3dDeviceContext->Map(appContext->pointcloudConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
					ldiPointCloudConstantBuffer* constantBuffer = (ldiPointCloudConstantBuffer*)ms.pData;
					//constantBuffer->size = vec4(0.1f, 0.1f, 0, 0);
					//constantBuffer->size = vec4(16, 16, 1, 0);
					constantBuffer->size = vec4(Tool->pointWorldSize, Tool->pointScreenSize, Tool->pointScreenSpaceBlend, 0);
					appContext->d3dDeviceContext->Unmap(appContext->pointcloudConstantBuffer, 0);
				}

				gfxRenderPointCloud(appContext, &Tool->scan.pointCloudRenderModel);
			}

			pushDebugBoxMinMax(&appContext->defaultDebug, Tool->scanBoundsMin, Tool->scanBoundsMax, vec3(0, 1, 0));
		}

		//----------------------------------------------------------------------------------------------------
		// Machine representation.
		//----------------------------------------------------------------------------------------------------
		if (Tool->showMachineFrame) {
			vec3 offset = glm::f64vec3(Tool->testPosX, Tool->testPosY, Tool->testPosZ) * job->stepsToCm;
			vec3 point(0.0f, 0.0f, 0.0f);
			point += offset.x * job->axisX.direction;
			point += offset.y * job->axisY.direction;
			point += offset.z * -job->axisZ.direction;

			vec3 offsetX = offset.x * job->axisX.direction;
			vec3 offsetY = offset.y * job->axisY.direction;

			// X Slab.
			{
				mat4 worldMat = glm::scale(mat4(1.0f), vec3(70.0f, 6.0f, 12.0f));
				worldMat[3] = vec4(-6.0f, -(39.0f + 3.0f), -0.5f, 1.0f);
			
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * worldMat;
				constantBuffer->world = worldMat;
				constantBuffer->color = vec4(0.3f, 0.3f, 0.3f, 1.0f);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				//gfxRenderModel(appContext, &Tool->cubeModel, false);
				gfxRenderModel(appContext, &Tool->cubeModel, appContext->defaultRasterizerState, appContext->litMeshVertexShader, appContext->litMeshPixelShader, appContext->litMeshInputLayout);
			}

			// Y Slab.
			{
				mat4 worldMat = glm::scale(mat4(1.0f), vec3(12.0f, 3.0f, 38.0f));
				worldMat[3] = vec4(vec3(-23.0f, -(32.0f + 1.5f), -2.5f) + offsetX, 1.0f);

				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * worldMat;
				constantBuffer->world = worldMat;
				constantBuffer->color = vec4(0.3f, 0.3f, 0.3f, 1.0f);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				//gfxRenderModel(appContext, &Tool->cubeModel, false);
				gfxRenderModel(appContext, &Tool->cubeModel, appContext->defaultRasterizerState, appContext->litMeshVertexShader, appContext->litMeshPixelShader, appContext->litMeshInputLayout);
			}

			// Z Slab.
			{
				mat4 worldMat = glm::scale(mat4(1.0f), vec3(12.0f, 39.0f, 3.0f));
				worldMat[3] = vec4(vec3(-23.0f, -(-10.0f + (39.0f / 2.0f)), -3.7f) + offsetX + offsetY, 1.0f);

				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * worldMat;
				constantBuffer->world = worldMat;
				constantBuffer->color = vec4(0.3f, 0.3f, 0.3f, 1.0f);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				//gfxRenderModel(appContext, &Tool->cubeModel, false);
				gfxRenderModel(appContext, &Tool->cubeModel, appContext->defaultRasterizerState, appContext->litMeshVertexShader, appContext->litMeshPixelShader, appContext->litMeshInputLayout);
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Horse.
	//----------------------------------------------------------------------------------------------------
	{
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.origin, "Origin", TextBuffer);
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.axisX, "X", TextBuffer);
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.axisY, "Y", TextBuffer);
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.axisZ, "Z", TextBuffer);
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.axisA, "A", TextBuffer);
		renderTransformOrigin(Tool->appContext, Camera, &Tool->horse.axisC, "C", TextBuffer);

		//mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
		//viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
		//mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

		//ldiTransform mvCameraTransform = {};
		//mvCameraTransform.world = glm::inverse(camViewMat);
		//renderTransformOrigin(Tool->appContext, Camera, &mvCameraTransform, "Camera", TextBuffer);

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
	// Project models.
	//----------------------------------------------------------------------------------------------------
	{
		ldiProjectContext* project = Tool->appContext->projectContext;

		if (project->sourceModelLoaded) {
			if (Tool->showSourceModelShaded) {

				/*[-0.55525757  0.05451943  0.85529738 - 0.07826866]
				[-0.02091123  1.01795051 - 0.078463    1.97768689]
				[-0.85677809 - 0.06017768 - 0.55238293  0.10937812]
				[0.          0.          0.          1.]*/

				mat4 tempMat = glm::identity<mat4>();
				tempMat[0][0] = -0.55525757;
				tempMat[0][1] = -0.02091123;
				tempMat[0][2] = -0.85677809;

				tempMat[1][0] = 0.05451943;
				tempMat[1][1] = 1.01795051;
				tempMat[1][2] = -0.06017768;

				tempMat[2][0] = 0.85529738;
				tempMat[2][1] = -0.078463;
				tempMat[2][2] = -0.55238293;

				tempMat[3][0] = -0.07826866;
				tempMat[3][1] = 1.97768689;
				tempMat[3][2] = 0.10937812;

				tempMat = glm::scale(tempMat, vec3(0.5, 0.5, 0.5));

				if (project->sourceTextureLoaded) {
					D3D11_MAPPED_SUBRESOURCE ms;
					appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
					ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
					constantBuffer->mvp = Camera->projViewMat * projectGetSourceTransformMat(project);
					constantBuffer->color = vec4(1, 1, 1, 1);
					appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

					gfxRenderModel(appContext, &project->sourceRenderModel, false, project->sourceTextureSrv, appContext->defaultPointSamplerState);
				} else {
					D3D11_MAPPED_SUBRESOURCE ms;
					appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
					ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
					mat4 worldMat = projectGetSourceTransformMat(project);
					//constantBuffer->mvp = Camera->projViewMat * worldMat;
					constantBuffer->mvp = Camera->projViewMat * tempMat;
					//constantBuffer->world = worldMat;
					constantBuffer->world = tempMat;
					constantBuffer->color = vec4(0.7f, 0.7f, 0.7f, 1.0f);
					appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

					gfxRenderModel(appContext, &project->sourceRenderModel, appContext->defaultRasterizerState, appContext->litMeshVertexShader, appContext->litMeshPixelShader, appContext->litMeshInputLayout);
				}
			}

			if (Tool->showSourceModelWireframe) {
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * projectGetSourceTransformMat(project);
				constantBuffer->color = vec4(0, 0, 0, 1);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				gfxRenderModel(appContext, &project->sourceRenderModel, true);
			}
		}

		if (project->quadModelLoaded) {
			if (Tool->showQuadMeshDebug) {
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * workWorldMat;
				constantBuffer->color = vec4(0, 0, 0, 1);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				gfxRenderDebugModel(appContext, &project->quadDebugModel);
			}

			if (Tool->showQuadMeshWireframe) {
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * workWorldMat;
				constantBuffer->color = vec4(0, 0, 0, 1);
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				gfxRenderWireModel(appContext, &project->quadMeshWire);
			}

			if (Tool->showQuadMeshWhite) {
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->mvp = Camera->projViewMat * workWorldMat;
				constantBuffer->color = Tool->quadMeshCanvasColor;
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

				gfxRenderDebugModel(appContext, &project->quadModelWhite);
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
		constantBuffer->mvp = Camera->projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
}

void platformMainRender(ldiPlatform* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(Tool->appContext, &Tool->mainViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	updateCamera3dBasicFps(&Tool->mainCamera, (float)Tool->mainViewWidth, (float)Tool->mainViewHeight);

	platformRender(Tool, &Tool->mainViewBuffers, Tool->mainViewWidth, Tool->mainViewHeight, &Tool->mainCamera, TextBuffer, true);
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

	if (Tool->liveAxisUpdate) {
		Tool->testPosX = Tool->positionX;
		Tool->testPosY = Tool->positionY;
		Tool->testPosZ = Tool->positionZ;
		Tool->testPosC = Tool->positionC;
		Tool->testPosA = Tool->positionA;
	}

	if (ImGui::Begin("Scan", 0, ImGuiWindowFlags_NoCollapse)) {
		ImGui::Checkbox("Use machine transformation", &Tool->scanUseWorkTrans);

		if (ImGui::CollapsingHeader("Point cloud rendering")) {
			ImGui::Checkbox("Show point cloud", &Tool->scanShowPointCloud);
			ImGui::SliderFloat("World size", &Tool->pointWorldSize, 0.0f, 1.0f);
			ImGui::SliderFloat("Screen size", &Tool->pointScreenSize, 0.0f, 32.0f);
			ImGui::SliderFloat("Screen blend", &Tool->pointScreenSpaceBlend, 0.0f, 1.0f);
		}

		ImGui::Separator();

		ImGui::DragFloat3("min##scanBoundsMin", (float*)&Tool->scanBoundsMin, 0.1f);
		ImGui::DragFloat3("max##scanBoundsMax", (float*)&Tool->scanBoundsMax, 0.1f);

		if (ImGui::Button("Clip points")) {
			std::vector<ldiPointCloudVertex> clippedPoints;

			for (auto& point : Tool->scan.pointCloud.points) {
				vec3 p = point.position;

				if (p.x <= Tool->scanBoundsMax.x && p.y <= Tool->scanBoundsMax.y && p.z <= Tool->scanBoundsMax.z &&
					p.x >= Tool->scanBoundsMin.x && p.y >= Tool->scanBoundsMin.y && p.z >= Tool->scanBoundsMin.z) {
					clippedPoints.push_back(point);
				}
			}

			scanUpdatePoints(Tool->appContext, &Tool->scan, clippedPoints);
		}

		ImGui::Separator();

		if (ImGui::Button("Save scan")) {
			std::string filePath;
			if (showSaveFileDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, filePath, L"Scan file", L"*.scan", L"scan")) {
				scanSaveFile(&Tool->scan, filePath);
			}
		}

		if (ImGui::Button("Load scan")) {
			std::string filePath;
			if (showOpenFileDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, filePath, L"Scan file", L"*.scan")) {
				scanLoadFile(Tool->appContext, &Tool->scan, filePath);
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Clear scan points")) {
			scanClearPoints(Tool->appContext, &Tool->scan);
		}

		if (ImGui::Button("Save point cloud")) {
			std::string filePath;
			if (showSaveFileDialog(Tool->appContext->hWnd, Tool->appContext->currentWorkingDir, filePath, L"Polygon", L"*.ply", L"ply")) {
				plySavePoints(filePath.c_str(), &Tool->scan.pointCloud);
			}
		}
	}
	ImGui::End();
		
	if (ImGui::Begin("Platform controls", 0, ImGuiWindowFlags_NoCollapse)) {
		if (ImGui::CollapsingHeader("Viewport")) {
			ImGui::SliderFloat("Camera speed", &Tool->mainCameraSpeed, 0.01f, 4.0f);

			ImGui::Separator();
			ImGui::Text("Rendering options");
			ImGui::Checkbox("Show default cube", &Tool->showDefaultCube);
			ImGui::Checkbox("Show machine frame", &Tool->showMachineFrame);
			ImGui::Checkbox("Show calib cube", &Tool->showCalibCube);
			ImGui::Checkbox("Show calib cube faces", &Tool->showCalibCubeFaces);
			ImGui::Checkbox("Show calib cube volume", &Tool->showCalibCubeVolume);
			ImGui::Checkbox("Show calib basis", &Tool->showCalibVolumeBasis);
			ImGui::Checkbox("Show scan plane", &Tool->showScanPlane);

			ImGui::Separator();
			ImGui::Checkbox("Show source model shaded", &Tool->showSourceModelShaded);
			ImGui::Checkbox("Show source model wireframe", &Tool->showSourceModelWireframe);

			ImGui::Separator();
			ImGui::Checkbox("Show quad mesh debug", &Tool->showQuadMeshDebug);
			ImGui::Checkbox("Show quad mesh wireframe", &Tool->showQuadMeshWireframe);
			ImGui::Checkbox("Show quad mesh canvas", &Tool->showQuadMeshWhite);

			ImGui::Separator();
			ImGui::Text("Simulated position");
			ImGui::Checkbox("Update live", &Tool->liveAxisUpdate);
			ImGui::SliderInt("PosX", &Tool->testPosX, -48000, 48000);
			ImGui::SliderInt("PosY", &Tool->testPosY, -48000, 48000);
			ImGui::SliderInt("PosZ", &Tool->testPosZ, -48000, 48000);
			ImGui::SliderInt("PosC", &Tool->testPosC, -192000 * 2, 192000 * 2);
			ImGui::SliderInt("PosA", &Tool->testPosA, -15000, 300000);
		}

		if (ImGui::CollapsingHeader("Control", ImGuiTreeNodeFlags_DefaultOpen)) {
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
			ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: %d", Tool->positionX);
			ImGui::SameLine();
			ImGui::SetCursorPosX(startX + availX / 3);
			ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: %d", Tool->positionY);
			ImGui::SameLine();
			ImGui::SetCursorPosX(startX + availX / 3 * 2);
			ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: %d", Tool->positionZ);
	
			ImGui::SetCursorPosX(startX + availX / 3 * 0);
			ImGui::TextColored(ImVec4(0.921f, 0.921f, 0.125f, 1.0f), "A: %d", Tool->positionA);
			ImGui::SameLine();
			ImGui::SetCursorPosX(startX + availX / 3 * 1);
			ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.921f, 1.0f), "C: %d", Tool->positionC);
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

			ImGui::Separator();

			if (ImGui::Button("Enable scan laser", ImVec2(-1, 0))) {
				platformQueueJobSetScanLaserState(Tool, true);
			}

			if (ImGui::Button("Disable scan laser", ImVec2(-1, 0))) {
				platformQueueJobSetScanLaserState(Tool, false);
			}

			ImGui::Separator();

			ImGui::BeginDisabled(!Tool->homed);

			//ImGui::Button("Go home", ImVec2(-1, 0));

			if (ImGui::Button("Capture calibration", ImVec2(-1, 0))) {
				platformQueueJobCaptureCalibration(Tool);
			}

			if (ImGui::Button("Capture scanner calibration", ImVec2(-1, 0))) {
				platformQueueJobCaptureScannerCalibration(Tool);
			}

			ImGui::Separator();
	
			if (ImGui::Button("Scan model", ImVec2(-1, 0))) {
				// Hawk 0 5000 exposure, continuous feed.
				// Hawk 1 off.
				platformQueueJobScan(Tool);
			}

			ImGui::EndDisabled();

			ImGui::Button("Start laser preview", ImVec2(-1, 0));

			ImGui::EndDisabled();
			ImGui::EndDisabled();
		}
	}

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
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 70), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("%.3f %.3f %.3f", Tool->mainCamera.position.x, Tool->mainCamera.position.y, Tool->mainCamera.position.z);
			ImGui::Text("%.3f %.3f %.3f", Tool->mainCamera.rotation.x, Tool->mainCamera.rotation.y, Tool->mainCamera.rotation.z);

			ImGui::EndChild();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}
