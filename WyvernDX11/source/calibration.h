#pragma once

void calibSaveStereoCalibImage(ldiImage* Image0, ldiImage* Image1, int X, int Y, int Z, int C, int A, int Phase, int ImgId, const std::string& Directory = "volume_calib_space") {
	char path[1024];
	sprintf_s(path, "../cache/%s/%04d_%d.dat", Directory.c_str(), ImgId, Phase);

	FILE* file;
	fopen_s(&file, path, "wb");

	fwrite(&Phase, sizeof(int), 1, file);
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

void calibLoadStereoCalibSampleData(ldiCalibStereoSample* Sample) {
	FILE* file;
	fopen_s(&file, Sample->path.c_str(), "rb");

	fread(&Sample->phase, sizeof(int), 1, file);
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

void calibFreeStereoCalibImages(ldiCalibStereoSample* Sample) {
	if (Sample->imageLoaded) {
		delete[] Sample->frames[0].data;
		delete[] Sample->frames[1].data;
		Sample->imageLoaded = false;
	}
}

void calibLoadStereoSampleImages(ldiCalibStereoSample* Sample) {
	calibFreeStereoCalibImages(Sample);

	FILE* file;
	fopen_s(&file, Sample->path.c_str(), "rb");

	int dummy;
	fread(&dummy, sizeof(int), 1, file);
	fread(&dummy, sizeof(int), 1, file);
	fread(&dummy, sizeof(int), 1, file);
	fread(&dummy, sizeof(int), 1, file);
	fread(&dummy, sizeof(int), 1, file);
	fread(&dummy, sizeof(int), 1, file);

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

void calibClearJob(ldiCalibrationJob* Job) {
	for (size_t i = 0; i < Job->samples.size(); ++i) {
		calibFreeStereoCalibImages(&Job->samples[i]);
	}
	Job->samples.clear();

	for (size_t i = 0; i < Job->scanSamples.size(); ++i) {
		calibFreeStereoCalibImages(&Job->scanSamples[i]);
	}
	Job->scanSamples.clear();

	Job->stereoCalibrated = false;
	Job->metricsCalculated = false;
	Job->scannerCalibrated = false;
	Job->galvoCalibrated = false;
}

template<class T> void serializeVectorPrep(FILE* File, std::vector<T>& Vector) {
	int size = Vector.size();
	fwrite(&size, sizeof(int), 1, File);
}

template<class T> size_t deserializeVectorPrep(FILE* File, std::vector<T>& Vector) {
	Vector.clear();

	int result = 0;
	fread(&result, sizeof(int), 1, File);

	Vector.resize(result);

	return (size_t)result;
}

size_t deserializeVectorPrep(FILE* File) {
	int result = 0;
	fread(&result, sizeof(int), 1, File);

	return (size_t)result;
}

void serializeString(FILE* File, const std::string& String) {
	int len = (int)String.length();
	const char* str = String.c_str();

	fwrite(&len, sizeof(int), 1, File);
	fwrite(str, len, 1, File);
}

std::string deserializeString(FILE* File) {
	int len;

	fread(&len, sizeof(int), 1, File);
	char* strBuf = new char[len + 1];
	fread(strBuf, sizeof(char), len, File);
	strBuf[len] = 0;

	std::string result = std::string(strBuf);

	delete[] strBuf;

	return result;
}

void serializeMat(FILE* File, const cv::Mat& Mat) {
	int type = Mat.type();
	int channels = Mat.channels();
	fwrite(&type, sizeof(int), 1, File);
	fwrite(&channels, sizeof(int), 1, File);
	fwrite(&Mat.rows, sizeof(int), 1, File);
	fwrite(&Mat.cols, sizeof(int), 1, File);

	if (!Mat.isContinuous()) {
		std::cout << "Error: Mat is not continuous!\n";
	}

	fwrite(Mat.data, Mat.cols * Mat.rows, Mat.elemSize(), File);
}

cv::Mat deserializeMat(FILE* File) {
	int type;
	int channels;
	int rows;
	int cols;
	fread(&type, sizeof(int), 1, File);
	fread(&channels, sizeof(int), 1, File);
	fread(&rows, sizeof(int), 1, File);
	fread(&cols, sizeof(int), 1, File);
	
	cv::Mat result = cv::Mat(rows, cols, type);

	fread(result.data, result.elemSize(), cols * rows, File);

	return result;
}

void serializeCharucoBoard(FILE* File, ldiCharucoBoard* Board) {
	fwrite(&Board->id, sizeof(ldiCharucoBoard::id), 1, File);

	serializeVectorPrep(File, Board->corners);
	for (size_t i = 0; i < Board->corners.size(); ++i) {
		fwrite(&Board->corners[i], sizeof(ldiCharucoCorner), 1, File);
	}

	fwrite(&Board->localMat, sizeof(ldiCharucoBoard::localMat), 1, File);
	
	fwrite(&Board->camLocalMat, sizeof(ldiCharucoBoard::camLocalMat), 1, File);
	fwrite(&Board->rejected, sizeof(ldiCharucoBoard::rejected), 1, File);
	fwrite(&Board->camLocalCharucoEstimatedMat, sizeof(ldiCharucoBoard::camLocalCharucoEstimatedMat), 1, File);
	fwrite(&Board->charucoEstimatedImageCenter, sizeof(ldiCharucoBoard::charucoEstimatedImageCenter), 1, File);
	fwrite(&Board->charucoEstimatedImageNormal, sizeof(ldiCharucoBoard::charucoEstimatedImageNormal), 1, File);
	fwrite(&Board->charucoEstimatedBoardAngle, sizeof(ldiCharucoBoard::charucoEstimatedBoardAngle), 1, File);

	serializeVectorPrep(File, Board->reprojectdCorners);
	for (size_t i = 0; i < Board->reprojectdCorners.size(); ++i) {
		fwrite(&Board->reprojectdCorners[i], sizeof(vec2), 1, File);
	}

	serializeVectorPrep(File, Board->outline);
	for (size_t i = 0; i < Board->outline.size(); ++i) {
		fwrite(&Board->outline[i], sizeof(vec2), 1, File);
	}
}

void deserializeCharucoBoard(FILE* File, ldiCharucoBoard* Board) {
	fread(&Board->id, sizeof(ldiCharucoBoard::id), 1, File);

	Board->corners.clear();
	size_t count = deserializeVectorPrep(File);
	for (size_t i = 0; i < count; ++i) {
		ldiCharucoCorner corner = {};
		fread(&corner, sizeof(ldiCharucoCorner), 1, File);
		Board->corners.push_back(corner);
	}

	fread(&Board->localMat, sizeof(ldiCharucoBoard::localMat), 1, File);

	fread(&Board->camLocalMat, sizeof(ldiCharucoBoard::camLocalMat), 1, File);
	fread(&Board->rejected, sizeof(ldiCharucoBoard::rejected), 1, File);
	fread(&Board->camLocalCharucoEstimatedMat, sizeof(ldiCharucoBoard::camLocalCharucoEstimatedMat), 1, File);
	fread(&Board->charucoEstimatedImageCenter, sizeof(ldiCharucoBoard::charucoEstimatedImageCenter), 1, File);
	fread(&Board->charucoEstimatedImageNormal, sizeof(ldiCharucoBoard::charucoEstimatedImageNormal), 1, File);
	fread(&Board->charucoEstimatedBoardAngle, sizeof(ldiCharucoBoard::charucoEstimatedBoardAngle), 1, File);

	Board->reprojectdCorners.clear();
	count = deserializeVectorPrep(File);
	for (size_t i = 0; i < count; ++i) {
		vec2 corner = {};
		fread(&corner, sizeof(vec2), 1, File);
		Board->reprojectdCorners.push_back(corner);
	}

	Board->outline.clear();
	count = deserializeVectorPrep(File);
	for (size_t i = 0; i < count; ++i) {
		vec2 corner = {};
		fread(&corner, sizeof(vec2), 1, File);
		Board->outline.push_back(corner);
	}
}

void serializeStereoSamples(FILE* File, const std::vector<ldiCalibStereoSample>& Samples) {

}

// Save the entire current state of the calibration job.
void calibSaveCalibJob(ldiCalibrationJob* Job) {
	char path[1024];
	sprintf_s(path, "../cache/volume_calib.dat");

	FILE* file;
	fopen_s(&file, path, "wb");

	serializeVectorPrep(file, Job->samples);
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		auto sample = &Job->samples[sampleIter];

		serializeString(file, sample->path.c_str());

		fwrite(&sample->phase, sizeof(int), 1, file);
		fwrite(&sample->X, sizeof(int), 1, file);
		fwrite(&sample->Y, sizeof(int), 1, file);
		fwrite(&sample->Z, sizeof(int), 1, file);
		fwrite(&sample->C, sizeof(int), 1, file);
		fwrite(&sample->A, sizeof(int), 1, file);

		for (int chResIter = 0; chResIter < 2; ++chResIter) {
			auto chRes = &sample->cubes[chResIter];

			serializeVectorPrep(file, chRes->markers);
			for (size_t markerIter = 0; markerIter < chRes->markers.size(); ++markerIter) {
				auto marker = &chRes->markers[markerIter];
				fwrite(&marker, sizeof(ldiCharucoMarker), 1, file);
			}

			serializeVectorPrep(file, chRes->rejectedMarkers);
			for (size_t markerIter = 0; markerIter < chRes->rejectedMarkers.size(); ++markerIter) {
				auto marker = &chRes->rejectedMarkers[markerIter];
				fwrite(&marker, sizeof(ldiCharucoMarker), 1, file);
			}

			serializeVectorPrep(file, chRes->boards);
			for (size_t boardIter = 0; boardIter < chRes->boards.size(); ++boardIter) {
				serializeCharucoBoard(file, &chRes->boards[boardIter]);
			}

			serializeVectorPrep(file, chRes->rejectedBoards);
			for (size_t boardIter = 0; boardIter < chRes->rejectedBoards.size(); ++boardIter) {
				serializeCharucoBoard(file, &chRes->rejectedBoards[boardIter]);
			}
		}
	}

	for (int i = 0; i < 2; ++i) {
		serializeMat(file, Job->defaultCamMat[i]);
		serializeMat(file, Job->defaultCamDist[i]);
		serializeMat(file, Job->refinedCamMat[i]);
		serializeMat(file, Job->refinedCamDist[i]);
	}

	fwrite(&Job->stereoCalibrated, sizeof(bool), 1, file);
	if (Job->stereoCalibrated) {
		serializeVectorPrep(file, Job->stPoseToSampleIds);
		for (size_t i = 0; i < Job->stPoseToSampleIds.size(); ++i) {
			fwrite(&Job->stPoseToSampleIds[i], sizeof(int), 1, file);
		}

		serializeVectorPrep(file, Job->stCubePoints);
		for (size_t i = 0; i < Job->stCubePoints.size(); ++i) {
			fwrite(&Job->stCubePoints[i], sizeof(vec3), 1, file);
		}

		serializeVectorPrep(file, Job->stCubeWorlds);
		for (size_t i = 0; i < Job->stCubeWorlds.size(); ++i) {
			fwrite(&Job->stCubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fwrite(&Job->stStereoCamWorld[i], sizeof(mat4), 1, file);
		}
	}

	fwrite(&Job->metricsCalculated, sizeof(bool), 1, file);
	if (Job->metricsCalculated) {
		serializeVectorPrep(file, Job->stBasisXPoints);
		for (size_t i = 0; i < Job->stBasisXPoints.size(); ++i) {
			fwrite(&Job->stBasisXPoints[i], sizeof(vec3), 1, file);
		}

		serializeVectorPrep(file, Job->stBasisYPoints);
		for (size_t i = 0; i < Job->stBasisYPoints.size(); ++i) {
			fwrite(&Job->stBasisYPoints[i], sizeof(vec3), 1, file);
		}

		serializeVectorPrep(file, Job->stBasisZPoints);
		for (size_t i = 0; i < Job->stBasisZPoints.size(); ++i) {
			fwrite(&Job->stBasisZPoints[i], sizeof(vec3), 1, file);
		}
		
		fwrite(&Job->stVolumeCenter, sizeof(vec3), 1, file);

		fwrite(&Job->axisX, sizeof(ldiLine), 1, file);
		fwrite(&Job->axisY, sizeof(ldiLine), 1, file);
		fwrite(&Job->axisZ, sizeof(ldiLine), 1, file);
		fwrite(&Job->axisC, sizeof(ldiLine), 1, file);
		fwrite(&Job->axisA, sizeof(ldiLine), 1, file);

		fwrite(&Job->basisX, sizeof(vec3), 1, file);
		fwrite(&Job->basisY, sizeof(vec3), 1, file);
		fwrite(&Job->basisZ, sizeof(vec3), 1, file);

		fwrite(&Job->stepsToCm, sizeof(glm::f64vec3), 1, file);

		serializeVectorPrep(file, Job->cubeWorlds);
		for (size_t i = 0; i < Job->cubeWorlds.size(); ++i) {
			fwrite(&Job->cubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fwrite(&Job->camVolumeMat[i], sizeof(mat4), 1, file);
		}
	}

	fwrite(&Job->scannerCalibrated, sizeof(bool), 1, file);
	if (Job->scannerCalibrated) {
		serializeVectorPrep(file, Job->scanSamples);
		for (size_t sampleIter = 0; sampleIter < Job->scanSamples.size(); ++sampleIter) {
			auto sample = &Job->scanSamples[sampleIter];

			serializeString(file, sample->path.c_str());

			fwrite(&sample->phase, sizeof(int), 1, file);
			fwrite(&sample->X, sizeof(int), 1, file);
			fwrite(&sample->Y, sizeof(int), 1, file);
			fwrite(&sample->Z, sizeof(int), 1, file);
			fwrite(&sample->C, sizeof(int), 1, file);
			fwrite(&sample->A, sizeof(int), 1, file);
		}

		fwrite(&Job->scanPlane, sizeof(ldiPlane), 1, file);
	}

	fwrite(&Job->galvoCalibrated, sizeof(bool), 1, file);
	if (Job->galvoCalibrated) {
	}

	fclose(file);
}

// Loads entire state of the calibration job.
void calibLoadCalibJob(ldiCalibrationJob* Job) {
	calibClearJob(Job);

	char path[1024];
	sprintf_s(path, "../cache/volume_calib.dat");

	FILE* file;
	fopen_s(&file, path, "rb");

	size_t sampleCount = deserializeVectorPrep(file);
	for (size_t sampleIter = 0; sampleIter < sampleCount; ++sampleIter) {
		ldiCalibStereoSample sample = {};

		sample.path = deserializeString(file);

		fread(&sample.phase, sizeof(int), 1, file);
		fread(&sample.X, sizeof(int), 1, file);
		fread(&sample.Y, sizeof(int), 1, file);
		fread(&sample.Z, sizeof(int), 1, file);
		fread(&sample.C, sizeof(int), 1, file);
		fread(&sample.A, sizeof(int), 1, file);

		for (int chResIter = 0; chResIter < 2; ++chResIter) {
			auto chRes = &sample.cubes[chResIter];

			size_t markerCount = deserializeVectorPrep(file);
			for (size_t markerIter = 0; markerIter < markerCount; ++markerIter) {
				ldiCharucoMarker marker = {};
				fread(&marker, sizeof(ldiCharucoMarker), 1, file);
				chRes->markers.push_back(marker);
			}

			markerCount = deserializeVectorPrep(file);
			for (size_t markerIter = 0; markerIter < markerCount; ++markerIter) {
				ldiCharucoMarker marker = {};
				fread(&marker, sizeof(ldiCharucoMarker), 1, file);
				chRes->rejectedMarkers.push_back(marker);
			}

			size_t boardCount = deserializeVectorPrep(file);
			for (size_t boardIter = 0; boardIter < boardCount; ++boardIter) {
				ldiCharucoBoard board = {};
				deserializeCharucoBoard(file, &board);
				chRes->boards.push_back(board);
			}

			boardCount = deserializeVectorPrep(file);
			for (size_t boardIter = 0; boardIter < boardCount; ++boardIter) {
				ldiCharucoBoard board = {};
				deserializeCharucoBoard(file, &board);
				chRes->rejectedBoards.push_back(board);
			}
		}

		Job->samples.push_back(sample);
	}

	for (int i = 0; i < 2; ++i) {
		Job->defaultCamMat[i] = deserializeMat(file);
		Job->defaultCamDist[i] = deserializeMat(file);
		Job->refinedCamMat[i] = deserializeMat(file);
		Job->refinedCamDist[i] = deserializeMat(file);
	}

	fread(&Job->stereoCalibrated, sizeof(bool), 1, file);
	if (Job->stereoCalibrated) {
		size_t count = deserializeVectorPrep(file, Job->stPoseToSampleIds);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stPoseToSampleIds[i], sizeof(int), 1, file);
		}

		count = deserializeVectorPrep(file, Job->stCubePoints);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stCubePoints[i], sizeof(vec3), 1, file);
		}

		count = deserializeVectorPrep(file, Job->stCubeWorlds);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stCubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fread(&Job->stStereoCamWorld[i], sizeof(mat4), 1, file);
		}
	}

	fread(&Job->metricsCalculated, sizeof(bool), 1, file);
	if (Job->metricsCalculated) {
		size_t count = deserializeVectorPrep(file, Job->stBasisXPoints);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stBasisXPoints[i], sizeof(vec3), 1, file);
		}

		count = deserializeVectorPrep(file, Job->stBasisYPoints);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stBasisYPoints[i], sizeof(vec3), 1, file);
		}

		count = deserializeVectorPrep(file, Job->stBasisZPoints);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stBasisZPoints[i], sizeof(vec3), 1, file);
		}
		
		fread(&Job->stVolumeCenter, sizeof(vec3), 1, file);
		
		fread(&Job->axisX, sizeof(ldiLine), 1, file);
		fread(&Job->axisY, sizeof(ldiLine), 1, file);
		fread(&Job->axisZ, sizeof(ldiLine), 1, file);
		fread(&Job->axisC, sizeof(ldiLine), 1, file);
		fread(&Job->axisA, sizeof(ldiLine), 1, file);

		fread(&Job->basisX, sizeof(vec3), 1, file);
		fread(&Job->basisY, sizeof(vec3), 1, file);
		fread(&Job->basisZ, sizeof(vec3), 1, file);

		fread(&Job->stepsToCm, sizeof(glm::f64vec3), 1, file);

		count = deserializeVectorPrep(file, Job->cubeWorlds);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->cubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fread(&Job->camVolumeMat[i], sizeof(mat4), 1, file);
		}
	}

	fread(&Job->scannerCalibrated, sizeof(bool), 1, file);
	if (Job->scannerCalibrated) {
		size_t count = deserializeVectorPrep(file, Job->scanSamples);
		for (size_t sampleIter = 0; sampleIter < count; ++sampleIter) {
			ldiCalibStereoSample sample = {};

			sample.path = deserializeString(file);

			fread(&sample.phase, sizeof(int), 1, file);
			fread(&sample.X, sizeof(int), 1, file);
			fread(&sample.Y, sizeof(int), 1, file);
			fread(&sample.Z, sizeof(int), 1, file);
			fread(&sample.C, sizeof(int), 1, file);
			fread(&sample.A, sizeof(int), 1, file);

			Job->scanSamples[sampleIter] = sample;
		}

		fread(&Job->scanPlane, sizeof(ldiPlane), 1, file);
	}

	fclose(file);
}

// Takes stereo image sample files and generates the initial calibration samples for a job.
void calibFindInitialObservations(ldiCalibrationJob* Job, ldiHawk* Hawks) {
	calibClearJob(Job);

	std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/volume_calib_space/");

	for (int i = 0; i < filePaths.size(); ++i) {
		if (endsWith(filePaths[i], ".dat")) {
			ldiCalibStereoSample sample = {};
			sample.path = filePaths[i];

			calibLoadStereoCalibSampleData(&sample);

			for (int j = 0; j < 2; ++j) {
				computerVisionFindCharuco(sample.frames[j], &sample.cubes[j], &Hawks[j].defaultCameraMat, &Hawks[j].defaultDistMat);
			}

			calibFreeStereoCalibImages(&sample);

			Job->samples.push_back(sample);
		}
	}

	for (int i = 0; i < 2; ++i) {
		Job->defaultCamMat[i] = Hawks[i].defaultCameraMat.clone();
		Job->defaultCamDist[i] = Hawks[i].defaultDistMat.clone();

		Job->refinedCamMat[i] = Job->defaultCamMat[i].clone();
		Job->refinedCamDist[i] = Job->defaultCamDist[i].clone();
	}
}

// Calculate stereo camera intrinsics, extrinsics, and cube transforms.
void calibStereoCalibrate(ldiCalibrationJob* Job) {
	std::cout << "Starting stereo calibration: " << getTime() << "\n";

	Job->stereoCalibrated = false;
	Job->stPoseToSampleIds.clear();
	Job->stCubePoints.clear();
	Job->stCubeWorlds.clear();
	
	//----------------------------------------------------------------------------------------------------
	// Perform stereo calibration.
	//----------------------------------------------------------------------------------------------------
	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> imagePoints[2];
	
	// Find matching points in both views.
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
	//for (size_t sampleIter = 0; sampleIter < 50; ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];

		std::vector<cv::Point2f> tempImagePoints[2];
		std::vector<int> tempGlobalIds[2];

		// Get image points from both views.
		for (size_t hawkIter = 0; hawkIter < 2; ++hawkIter) {
			std::vector<ldiCharucoBoard>* boards = &sample->cubes[hawkIter].boards;

			for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
				ldiCharucoBoard* board = &(*boards)[boardIter];

				for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
					ldiCharucoCorner* corner = &board->corners[cornerIter];

					int cornerGlobalId = (board->id * 9) + corner->id;

					tempImagePoints[hawkIter].push_back(cv::Point2f(corner->position.x, corner->position.y));
					tempGlobalIds[hawkIter].push_back(cornerGlobalId);
				}
			}
		}

		std::vector<cv::Point3f> appendObjectPoints;
		std::vector<cv::Point2f> appendImagePoints[2];

		// Compare image points from both views.
		for (size_t pointIter0 = 0; pointIter0 < tempImagePoints[0].size(); ++pointIter0) {
			int point0Id = tempGlobalIds[0][pointIter0];

			for (size_t pointIter1 = 0; pointIter1 < tempImagePoints[1].size(); ++pointIter1) {
				int point1Id = tempGlobalIds[1][pointIter1];

				if (point0Id == point1Id) {
					appendObjectPoints.push_back(_refinedModelPoints[point0Id]);
					appendImagePoints[0].push_back(tempImagePoints[0][pointIter0]);
					appendImagePoints[1].push_back(tempImagePoints[1][pointIter1]);

					break;
				}
			}
		}

		if (appendObjectPoints.size() > 3) {
			objectPoints.push_back(appendObjectPoints);
			imagePoints[0].push_back(appendImagePoints[0]);
			imagePoints[1].push_back(appendImagePoints[1]);
			Job->stPoseToSampleIds.push_back(sampleIter);
		}
	}

	cv::Mat camMat[2];
	camMat[0] = Job->defaultCamMat[0].clone();
	camMat[1] = Job->defaultCamMat[1].clone();

	cv::Mat distMat[2];
	distMat[0] = Job->defaultCamDist[0].clone();
	distMat[1] = Job->defaultCamDist[1].clone();

	int imgWidth = Job->samples[0].frames[0].width;
	int imgHeight = Job->samples[0].frames[0].height;

	cv::Mat r;
	cv::Mat t;
	cv::Mat e;
	cv::Mat f;
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	cv::Mat perViewErrors;
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-6); // 100

	//cv::CALIB_FIX_INTRINSIC
	//cv::CALIB_USE_INTRINSIC_GUESS
	double error;
	try {
		error = cv::stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1], camMat[0], distMat[0], camMat[1], distMat[1], cv::Size(imgWidth, imgHeight), r, t, e, f, rvecs, tvecs, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS, criteria);
	} catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
		return;
	}

	std::cout << "Done stereo calibration: " << getTime() << "\n";

	std::cout << "Error: " << error << "\n";
	std::cout << "Per view errors: " << perViewErrors << "\n\n";

	mat4 camRt(1.0f);
	camRt[0][0] = r.at<double>(0, 0);
	camRt[0][1] = r.at<double>(1, 0);
	camRt[0][2] = r.at<double>(2, 0);
	camRt[1][0] = r.at<double>(0, 1);
	camRt[1][1] = r.at<double>(1, 1);
	camRt[1][2] = r.at<double>(2, 1);
	camRt[2][0] = r.at<double>(0, 2);
	camRt[2][1] = r.at<double>(1, 2);
	camRt[2][2] = r.at<double>(2, 2);
	camRt[3] = vec4(t.at<double>(0), t.at<double>(1), t.at<double>(2), 1.0f);

	Job->stStereoCamWorld[0] = glm::identity<mat4>();
	Job->stStereoCamWorld[1] = glm::inverse(camRt);

	float dist = glm::distance(vec3(0, 0, 0.0f), vec3(t.at<double>(0), t.at<double>(1), t.at<double>(2)));
	std::cout << "Dist: " << dist << "\n";

	std::cout << "New cam 0 mats: \n";
	std::cout << camMat[0] << "\n";
	std::cout << distMat[0] << "\n";
	std::cout << "New cam 1 mats: \n";
	std::cout << camMat[1] << "\n";
	std::cout << distMat[1] << "\n";;

	Job->refinedCamMat[0] = camMat[0].clone();
	Job->refinedCamDist[0] = distMat[0].clone();
	Job->refinedCamMat[1] = camMat[1].clone();
	Job->refinedCamDist[1] = distMat[1].clone();

	for (size_t i = 0; i < _refinedModelPoints.size(); ++i) {
		Job->stCubePoints.push_back(vec3(_refinedModelPoints[i].x, _refinedModelPoints[i].y, _refinedModelPoints[i].z));
	}

	for (size_t i = 0; i < rvecs.size(); ++i) {
		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rvecs[i], cvRotMat);

		mat4 pose(1.0f);
		pose[0][0] = cvRotMat.at<double>(0, 0);
		pose[0][1] = cvRotMat.at<double>(1, 0);
		pose[0][2] = cvRotMat.at<double>(2, 0);
		pose[1][0] = cvRotMat.at<double>(0, 1);
		pose[1][1] = cvRotMat.at<double>(1, 1);
		pose[1][2] = cvRotMat.at<double>(2, 1);
		pose[2][0] = cvRotMat.at<double>(0, 2);
		pose[2][1] = cvRotMat.at<double>(1, 2);
		pose[2][2] = cvRotMat.at<double>(2, 2);
		pose[3] = vec4(tvecs[i].at<double>(0), tvecs[i].at<double>(1), tvecs[i].at<double>(2), 1.0f);

		Job->stCubeWorlds.push_back(pose);
	}

	Job->stereoCalibrated = true;
}

// Determine metrics for the calibration volume.
void calibBuildCalibVolumeMetrics(ldiCalibrationJob* Job) {
	Job->metricsCalculated = false;

	if (!Job->stereoCalibrated) {
		std::cout << "Can't calculate metrics, no stereo calibration\n";
		return;
	}

	//----------------------------------------------------------------------------------------------------
	// Sort into columns.
	//----------------------------------------------------------------------------------------------------
	struct ldiTbPose {
		int x;
		int y;
		int z;
		mat4 pose;
	};

	struct ldiTbColumn {
		int x;
		int y;
		int z;
		std::vector<ldiTbPose> poses;
	};

	std::vector<ldiTbColumn> columnX;
	std::vector<ldiTbColumn> columnY;
	std::vector<ldiTbColumn> columnZ;

	// TODO: Use view error to decide if view should be included.
	for (size_t sampleIter = 0; sampleIter < Job->stPoseToSampleIds.size(); ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[Job->stPoseToSampleIds[sampleIter]];

		if (sample->phase != 1) {
			continue;
		}

		if (sample->A != 0 || sample->C != 13000) {
			continue;
		}

		int x = sample->X;
		int y = sample->Y;
		int z = sample->Z;

		// X.
		{
			size_t columnId = -1;
			for (size_t i = 0; i < columnX.size(); ++i) {
				if (columnX[i].y == y && columnX[i].z == z) {
					columnId = i;
					break;
				}
			}

			if (columnId == -1) {
				columnId = columnX.size();

				ldiTbColumn column;
				column.x = 0;
				column.y = y;
				column.z = z;
				columnX.push_back(column);
			}

			ldiTbPose pose;
			pose.x = x;
			pose.y = y;
			pose.z = z;
			pose.pose = Job->stCubeWorlds[sampleIter];
			columnX[columnId].poses.push_back(pose);

			//std::cout << "(X) View: " << sampleIter << " Sample: " << stereoSampleId[sampleIter] <<  " Mech: " << x << "," << y << "," << z << "\n";
		}

		// Y.
		{
			size_t columnId = -1;
			for (size_t i = 0; i < columnY.size(); ++i) {
				if (columnY[i].x == x && columnY[i].z == z) {
					columnId = i;
					break;
				}
			}

			if (columnId == -1) {
				columnId = columnY.size();

				ldiTbColumn column;
				column.x = x;
				column.y = 0;
				column.z = z;
				columnY.push_back(column);
			}

			ldiTbPose pose;
			pose.x = x;
			pose.y = y;
			pose.z = z;
			pose.pose = Job->stCubeWorlds[sampleIter];
			columnY[columnId].poses.push_back(pose);
		}

		// Z.
		{
			size_t columnId = -1;
			for (size_t i = 0; i < columnZ.size(); ++i) {
				if (columnZ[i].x == x && columnZ[i].y == y) {
					columnId = i;
					break;
				}
			}

			if (columnId == -1) {
				columnId = columnZ.size();

				ldiTbColumn column;
				column.x = x;
				column.y = y;
				column.z = 0;
				columnZ.push_back(column);
			}

			ldiTbPose pose;
			pose.x = x;
			pose.y = y;
			pose.z = z;
			pose.pose = Job->stCubeWorlds[sampleIter];
			columnZ[columnId].poses.push_back(pose);
		}
	}

	std::cout << "X columns: " << columnX.size() << "\n";
	std::cout << "Y columns: " << columnY.size() << "\n";
	std::cout << "Z columns: " << columnZ.size() << "\n";

	//----------------------------------------------------------------------------------------------------
	// Find real movement distances.
	//----------------------------------------------------------------------------------------------------
	double distAvgX = 0.0f;
	double distAvgY = 0.0f;
	double distAvgZ = 0.0f;

	// X
	{
		double distAccum = 0.0;
		int distAccumCount = 0;

		std::vector<ldiTbColumn>* column = &columnX;
		for (size_t colIter = 0; colIter < column->size(); ++colIter) {

			// Pair all cubes in column.
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].poses.size(); ++cubeIter0) {
				ldiTbPose cube0 = (*column)[colIter].poses[cubeIter0];

				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].poses.size(); ++cubeIter1) {
					ldiTbPose cube1 = (*column)[colIter].poses[cubeIter1];

					double cubeMechDist = glm::distance(vec3(cube0.x, cube0.y, cube0.z), vec3(cube1.x, cube1.y, cube1.z));
					double distVolSpace = glm::distance(vec3(cube0.pose[3]), vec3(cube1.pose[3]));
					double distNorm = distVolSpace / cubeMechDist;

					distAccum += distNorm;
					distAccumCount += 1;

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
				}
			}
		}

		distAvgX = distAccum / (double)distAccumCount;
	}

	// Y
	{
		double distAccum = 0.0;
		int distAccumCount = 0;

		std::vector<ldiTbColumn>* column = &columnY;
		for (size_t colIter = 0; colIter < column->size(); ++colIter) {

			// Pair all cubes in column.
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].poses.size(); ++cubeIter0) {
				ldiTbPose cube0 = (*column)[colIter].poses[cubeIter0];

				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].poses.size(); ++cubeIter1) {
					ldiTbPose cube1 = (*column)[colIter].poses[cubeIter1];

					double cubeMechDist = glm::distance(vec3(cube0.x, cube0.y, cube0.z), vec3(cube1.x, cube1.y, cube1.z));
					double distVolSpace = glm::distance(vec3(cube0.pose[3]), vec3(cube1.pose[3]));
					double distNorm = distVolSpace / cubeMechDist;

					distAccum += distNorm;
					distAccumCount += 1;

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
				}
			}
		}

		distAvgY = distAccum / (double)distAccumCount;
	}

	// Z
	{
		double distAccum = 0.0;
		int distAccumCount = 0;

		std::vector<ldiTbColumn>* column = &columnZ;
		for (size_t colIter = 0; colIter < column->size(); ++colIter) {

			// Pair all cubes in column.
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].poses.size(); ++cubeIter0) {
				ldiTbPose cube0 = (*column)[colIter].poses[cubeIter0];

				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].poses.size(); ++cubeIter1) {
					ldiTbPose cube1 = (*column)[colIter].poses[cubeIter1];

					double cubeMechDist = glm::distance(vec3(cube0.x, cube0.y, cube0.z), vec3(cube1.x, cube1.y, cube1.z));
					double distVolSpace = glm::distance(vec3(cube0.pose[3]), vec3(cube1.pose[3]));
					double distNorm = distVolSpace / cubeMechDist;

					distAccum += distNorm;
					distAccumCount += 1;

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
				}
			}
		}

		distAvgZ = distAccum / (double)distAccumCount;
	}

	Job->stepsToCm = vec3(distAvgX, distAvgY, distAvgZ);

	std::cout << "Dist avg X: " << distAvgX << "\n";
	std::cout << "Dist avg Y: " << distAvgY << "\n";
	std::cout << "Dist avg Z: " << distAvgZ << "\n";

	//----------------------------------------------------------------------------------------------------
	// Find basis directions.
	//----------------------------------------------------------------------------------------------------
	Job->stBasisXPoints.clear();
	Job->stBasisYPoints.clear();
	Job->stBasisZPoints.clear();

	// X.
	{
		for (size_t i = 0; i < columnX.size(); ++i) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnX[i].poses.size(); ++pointIter) {
				centroid += vec3(columnX[i].poses[pointIter].pose[3]);
			}

			centroid /= (float)columnX[i].poses.size();

			for (size_t pointIter = 0; pointIter < columnX[i].poses.size(); ++pointIter) {
				vec3 transPoint = vec3(columnX[i].poses[pointIter].pose[3]) - centroid;
				Job->stBasisXPoints.push_back(transPoint);
			}
		}
	}

	// Y.
	{
		for (size_t i = 0; i < columnY.size(); ++i) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnY[i].poses.size(); ++pointIter) {
				centroid += vec3(columnY[i].poses[pointIter].pose[3]);
			}

			centroid /= (float)columnY[i].poses.size();

			for (size_t pointIter = 0; pointIter < columnY[i].poses.size(); ++pointIter) {
				vec3 transPoint = vec3(columnY[i].poses[pointIter].pose[3]) - centroid;
				Job->stBasisYPoints.push_back(transPoint);
			}
		}
	}

	// Z.
	{
		for (size_t i = 0; i < columnZ.size(); ++i) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnZ[i].poses.size(); ++pointIter) {
				centroid += vec3(columnZ[i].poses[pointIter].pose[3]);
			}

			centroid /= (float)columnZ[i].poses.size();

			for (size_t pointIter = 0; pointIter < columnZ[i].poses.size(); ++pointIter) {
				vec3 transPoint = vec3(columnZ[i].poses[pointIter].pose[3]) - centroid;
				Job->stBasisZPoints.push_back(transPoint);
			}
		}
	}

	Job->axisX = computerVisionFitLine(Job->stBasisXPoints);
	Job->axisY = computerVisionFitLine(Job->stBasisYPoints);
	Job->axisZ = computerVisionFitLine(Job->stBasisZPoints);

	Job->basisX = Job->axisX.direction;
	Job->basisY = glm::normalize(glm::cross(Job->basisX, Job->axisZ.direction));
	Job->basisZ = glm::normalize(glm::cross(Job->basisX, Job->basisY));

	//----------------------------------------------------------------------------------------------------
	// Gather rotarty axis samples.
	//----------------------------------------------------------------------------------------------------
	// TODO: Fit to offset cube pose center point for now, but when we rebase this to cube middle, this can't work (as effectively).
	Job->axisCPoints.clear();
	Job->axisAPoints.clear();

	for (size_t poseIter = 0; poseIter < Job->stPoseToSampleIds.size(); ++poseIter) {
		ldiCalibStereoSample* sample = &Job->samples[Job->stPoseToSampleIds[poseIter]];

		if (sample->phase == 2) {
			vec3 transPoint = Job->stCubeWorlds[poseIter][3];
			Job->axisCPoints.push_back(transPoint);
		} else if (sample->phase == 3) {
			vec3 transPoint = Job->stCubeWorlds[poseIter][3];
			Job->axisAPoints.push_back(transPoint);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Calculate calib volume to world basis.
	//----------------------------------------------------------------------------------------------------
	mat4 worldToVolume = glm::identity<mat4>();

	worldToVolume[0] = vec4(Job->basisX, 0.0f);
	worldToVolume[1] = vec4(Job->basisY, 0.0f);
	worldToVolume[2] = vec4(Job->basisZ, 0.0f);

	mat4 volumeRealign = glm::rotate(mat4(1.0f), glm::radians(90.0f), Job->basisX);
	worldToVolume = volumeRealign * worldToVolume;

	// Volume center point is at 0,0,0.
	// Use sample 0 as basis for location.
	// TODO: Calculate better calib cube center.

	glm::f64vec3 samp0;
	bool foundSample0 = false;

	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];

		if (sample->phase == 0) {
			samp0 = glm::f64vec3(sample->X, sample->Y, sample->Z);
			foundSample0 = true;
			std::cout << "Zero sample: " << sampleIter << "\n";
			break;
		}
	}

	if (!foundSample0) {
		std::cout << "Could not find zero sample.\n";
		return;
	}

	samp0 *= Job->stepsToCm;
	std::cout << "Samp0: " << samp0.x << ", " << samp0.y << ", " << samp0.z << "\n";
	//samp0 -= vec3(-2.0, 0.9, 0.9);
	samp0 = (float)samp0.x * Job->axisX.direction + (float)samp0.y * Job->axisY.direction + -(float)samp0.z * Job->axisZ.direction;

	// TOOD: This always refers to the 0th sample?
	vec3 volPos = Job->stCubeWorlds[0][3];
	//volPos -= samp0;
	//volPos -= _refinedModelCentroid / 2.0f;
	std::cout << "Offset from center: " << volPos.x << ", " << volPos.y << ", " << volPos.z << "\n";

	volumeRealign = glm::translate(mat4(1.0f), volPos);
	worldToVolume = volumeRealign * worldToVolume;

	mat4 volumeToWorld = glm::inverse(worldToVolume);

	//----------------------------------------------------------------------------------------------------
	// Transform all volume elements into world space.
	//----------------------------------------------------------------------------------------------------
	{
		// Axis directions.
		Job->axisX.direction = glm::normalize(volumeToWorld * vec4(Job->axisX.direction, 0.0f));
		Job->axisY.direction = glm::normalize(volumeToWorld * vec4(Job->axisY.direction, 0.0f));
		Job->axisZ.direction = glm::normalize(volumeToWorld * vec4(Job->axisZ.direction, 0.0f));

		for (size_t i = 0; i < Job->stBasisXPoints.size(); ++i) {
			Job->stBasisXPoints[i] = volumeToWorld * vec4(Job->stBasisXPoints[i], 0.0f);
		}

		for (size_t i = 0; i < Job->stBasisYPoints.size(); ++i) {
			Job->stBasisYPoints[i] = volumeToWorld * vec4(Job->stBasisYPoints[i], 0.0f);
		}

		for (size_t i = 0; i < Job->stBasisZPoints.size(); ++i) {
			Job->stBasisZPoints[i] = volumeToWorld * vec4(Job->stBasisZPoints[i], 0.0f);
		}

		// Center point.
		Job->stVolumeCenter = volumeToWorld * vec4(volPos, 1.0);

		// Mass model.
		/*for (size_t i = 0; i < Job->massModelBundleAdjustPointIds.size(); ++i) {
			vec3 point = Job->massModelTriangulatedPointsBundleAdjust[i];
			point = volumeToWorld * vec4(point, 1.0f);
			Job->massModelTriangulatedPointsBundleAdjust[i] = point;
		}*/

		// Cameras to world space.
		Job->camVolumeMat[0] = volumeToWorld * Job->stStereoCamWorld[0];
		Job->camVolumeMat[1] = volumeToWorld * Job->stStereoCamWorld[1];

		// Normalize scale.
		Job->camVolumeMat[0][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][0])), 0.0f);
		Job->camVolumeMat[0][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][1])), 0.0f);
		Job->camVolumeMat[0][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][2])), 0.0f);

		Job->camVolumeMat[1][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][0])), 0.0f);
		Job->camVolumeMat[1][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][1])), 0.0f);
		Job->camVolumeMat[1][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][2])), 0.0f);

		// Cube poses.
		Job->cubeWorlds.resize(Job->stCubeWorlds.size());
		for (size_t i = 0; i < Job->stCubeWorlds.size(); ++i) {
			Job->cubeWorlds[i] = volumeToWorld * Job->stCubeWorlds[i];
		}

		// TODO: Average cube poses for 0th pose?

		// Axis C points.
		for (size_t i = 0; i < Job->axisCPoints.size(); ++i) {
			Job->axisCPoints[i] = volumeToWorld * vec4(Job->axisCPoints[i], 1.0f);
		}

		// Axis A points.
		for (size_t i = 0; i < Job->axisAPoints.size(); ++i) {
			Job->axisAPoints[i] = volumeToWorld * vec4(Job->axisAPoints[i], 1.0f);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Axis C measurement.
	//----------------------------------------------------------------------------------------------------
	{
		computerVisionFitPlane(Job->axisCPoints, &Job->axisCPlane);

		mat4 planeBasis = planeGetBasis(Job->axisCPlane);
		mat4 planeBasisInv = glm::inverse(planeBasis);
		std::vector<vec2> pointsOnPlane;

		// Project all points onto the plane.
		Job->axisCPointsPlaneProjected.clear();
		for (size_t i = 0; i < Job->axisCPoints.size(); ++i) {
			Job->axisCPointsPlaneProjected.push_back(projectPointToPlane(Job->axisCPoints[i], Job->axisCPlane));

			vec3 planePos = planeBasisInv * vec4(Job->axisCPointsPlaneProjected[i], 1.0f);
			pointsOnPlane.push_back(vec2(planePos.x, planePos.y));
		}

		ldiCircle circle = circleFit(pointsOnPlane);

		// Transform 2D circle back to 3D plane.
		circle.normal = planeBasis * vec4(circle.normal, 0.0f);
		circle.origin = planeBasis * vec4(circle.origin, 1.0f);

		// This axis always points positive on Y.
		if (circle.normal.y < 0.0f) {
			circle.normal = -circle.normal;
		}

		Job->axisCCircle = circle;
		Job->axisC = { circle.origin, circle.normal };
	}

	//----------------------------------------------------------------------------------------------------
	// Axis A measurement.
	//----------------------------------------------------------------------------------------------------
	{
		computerVisionFitPlane(Job->axisAPoints, &Job->axisAPlane);

		mat4 planeBasis = planeGetBasis(Job->axisAPlane);
		mat4 planeBasisInv = glm::inverse(planeBasis);
		std::vector<vec2> pointsOnPlane;

		// Project all points onto the plane.
		Job->axisAPointsPlaneProjected.clear();
		for (size_t i = 0; i < Job->axisAPoints.size(); ++i) {
			Job->axisAPointsPlaneProjected.push_back(projectPointToPlane(Job->axisAPoints[i], Job->axisAPlane));

			vec3 planePos = planeBasisInv * vec4(Job->axisAPointsPlaneProjected[i], 1.0f);
			pointsOnPlane.push_back(vec2(planePos.x, planePos.y));
		}

		ldiCircle circle = circleFit(pointsOnPlane);

		// Transform 2D circle back to 3D plane.
		circle.normal = planeBasis * vec4(circle.normal, 0.0f);
		circle.origin = planeBasis * vec4(circle.origin, 1.0f);

		// This axis always points negative on X.
		if (circle.normal.x > 0.0f) {
			circle.normal = -circle.normal;
		}

		Job->axisACircle = circle;
		Job->axisA = { circle.origin, circle.normal };
	}

	Job->metricsCalculated = true;
}

// Takes stereo image sample files and generates scanner calibration.
void calibCalibrateScanner(ldiCalibrationJob* Job) {
	for (size_t i = 0; i < Job->scanSamples.size(); ++i) {
		calibFreeStereoCalibImages(&Job->scanSamples[i]);
	}
	Job->scanSamples.clear();
	
	Job->scannerCalibrated = false;
	Job->scanPoints[0].clear();
	Job->scanPoints[1].clear();
	Job->scanRays[0].clear();
	Job->scanRays[1].clear();
	Job->scanWorldPoints[0].clear();
	Job->scanWorldPoints[1].clear();

	if (!Job->metricsCalculated) {
		std::cout << "Scanner calibration requires metrics to be calculated\n";
		return;
	}

	std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/scanner_calib/");

	for (int i = 0; i < filePaths.size(); ++i) {
		if (endsWith(filePaths[i], ".dat")) {
			ldiCalibStereoSample sample = {};
			sample.path = filePaths[i];

			calibLoadStereoCalibSampleData(&sample);

			// Find machine basis for this sample position.
			ldiHorsePosition horsePos = {};
			horsePos.x = sample.X;
			horsePos.y = sample.Y;
			horsePos.z = sample.Z;
			horsePos.c = sample.C;
			horsePos.a = sample.A;

			std::vector<vec3> cubePoints;
			std::vector<ldiCalibCubeSide> cubeSides;
			std::vector<vec3> cubeCorners;

			horseGetRefinedCubeAtPosition(Job, horsePos, cubePoints, cubeSides, cubeCorners);

			// Plane scan line bounds.
			ldiPlane boundPlanes[4];

			vec3 dir01 = glm::normalize(cubeCorners[1] - cubeCorners[0]);
			vec3 dir12 = glm::normalize(cubeCorners[2] - cubeCorners[1]);
			vec3 dir23 = glm::normalize(cubeCorners[3] - cubeCorners[2]);
			vec3 dir30 = glm::normalize(cubeCorners[0] - cubeCorners[3]);

			boundPlanes[0].normal = dir01;
			boundPlanes[0].point = cubeCorners[0] + dir01 * 0.2f;

			boundPlanes[1].normal = dir12;
			boundPlanes[1].point = cubeCorners[1] + dir12 * 0.01f;

			boundPlanes[2].normal = dir23;
			boundPlanes[2].point = cubeCorners[2] + dir23 * 0.2f;

			boundPlanes[3].normal = dir30;
			boundPlanes[3].point = cubeCorners[3] + dir30 * 3.0f;

			for (int j = 0; j < 2; ++j) {
				std::vector<vec2> points = computerVisionFindScanLine(sample.frames[j]);

				std::vector<cv::Point2f> distortedPoints;
				std::vector<cv::Point2f> undistortedPoints;

				for (size_t pIter = 0; pIter < points.size(); ++pIter) {
					distortedPoints.push_back(toPoint2f(points[pIter]));
				}

				cv::undistortPoints(distortedPoints, undistortedPoints, Job->refinedCamMat[j], Job->refinedCamDist[j], cv::noArray(), Job->refinedCamMat[j]);

				for (size_t pIter = 0; pIter < points.size(); ++pIter) {
					points[pIter] = toVec2(undistortedPoints[pIter]);
				}

				Job->scanPoints[j].push_back(points);

				// Project points against current machine basis.
				ldiCamera camera = horseGetCamera(Job, horsePos, j, 3280, 2464);

				//if (i == 0) {
				{
					for (size_t pIter = 0; pIter < points.size(); ++pIter) {
						ldiLine ray = screenToRay(&camera, points[pIter]);

						vec3 worldPoint;
						getRayPlaneIntersection(ray, cubeSides[1].plane, worldPoint);

						// Check point within bounds.
						bool pointWithinBounds = true;
						for (int bIter = 0; bIter < 4; ++bIter) {
							if (glm::dot(boundPlanes[bIter].normal, worldPoint - boundPlanes[bIter].point) <= 0.0f){
								pointWithinBounds = false;
								break;
							}
						}

						if (pointWithinBounds) {
							Job->scanWorldPoints[j].push_back(worldPoint);
							//Job->scanRays[j].push_back(ray);
						}
					}
				}
			}

			calibFreeStereoCalibImages(&sample);

			Job->scanSamples.push_back(sample);
		}
	}

	std::vector<vec3> allWorldPoints;
	allWorldPoints.reserve(Job->scanWorldPoints[0].size() + Job->scanWorldPoints[1].size());
	allWorldPoints.insert(allWorldPoints.begin(), Job->scanWorldPoints[0].begin(), Job->scanWorldPoints[0].end());
	//allWorldPoints.insert(allWorldPoints.begin() + Job->scanWorldPoints[0].size(), Job->scanWorldPoints[1].begin(), Job->scanWorldPoints[1].end());

	ldiPlane scanPlane;
	computerVisionFitPlane(allWorldPoints, &Job->scanPlane);

	Job->scannerCalibrated = true;
}

//----------------------------------------------------------------------------------------------------
// Bundle adjust experiments.
//----------------------------------------------------------------------------------------------------
void calibSaveInitialOutput(ldiCalibrationJob* Job) {
	std::vector<int> viewId;
	std::vector<int> stereoSampleId;
	std::vector<int> camId;
	std::vector<cv::Mat> camExtrinsics;
	std::vector<cv::Mat> diffExtrinsics;
	std::vector<std::vector<cv::Point2f>> observations;
	std::vector<std::vector<int>> observationPointIds;

	int totalImagePointCount = 0;

	ldiCalibCube2 initialCube;
	calibCubeInit(&initialCube);

	// Find a pose for each calibration sample.
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
	//for (size_t sampleIter = 0; sampleIter < 344; ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];

		std::vector<cv::Point2f> imagePoints[2];
		std::vector<int> imagePointIds[2];
		//cv::Mat camExts[2];
		mat4 camExts[2];

		bool foundPoseInBothEyes = true;

		for (int hawkIter = 0; hawkIter < 2; ++hawkIter) {
			// Combine all boards into a set of image points and 3d target local points.
			std::vector<ldiCharucoBoard>* boards = &sample->cubes[hawkIter].boards;

			std::vector<cv::Point3f> worldPoints;

			for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
				ldiCharucoBoard* board = &(*boards)[boardIter];

				for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
					ldiCharucoCorner* corner = &board->corners[cornerIter];
					int cornerGlobalId = (board->id * 9) + corner->id;
					vec3 targetPoint = initialCube.points[cornerGlobalId];

					imagePoints[hawkIter].push_back(cv::Point2f(corner->position.x, corner->position.y));
					worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
					imagePointIds[hawkIter].push_back(cornerGlobalId);
				}
			}

			if (imagePoints[hawkIter].size() >= 6) {
				//cv::Mat r;
				//cv::Mat t;
				mat4 camViewMat;

				std::cout << "Find pose - Sample: " << sampleIter << " Hawk: " << hawkIter << "\n";
				if (computerVisionFindGeneralPose(&Job->defaultCamMat[hawkIter], &Job->defaultCamDist[hawkIter], &imagePoints[hawkIter], &worldPoints, &camViewMat)) {
				
					//// NOTE: OpenCV coords are flipped on Y and Z.
					//{
					//	cv::Mat cvRotMat;
					//	cv::Rodrigues(r, cvRotMat);

					//	cvRotMat.at<double>(1, 0) = -cvRotMat.at<double>(1, 0);
					//	cvRotMat.at<double>(2, 0) = -cvRotMat.at<double>(2, 0);
					//	cvRotMat.at<double>(1, 1) = -cvRotMat.at<double>(1, 1);
					//	cvRotMat.at<double>(2, 1) = -cvRotMat.at<double>(2, 1);
					//	cvRotMat.at<double>(1, 2) = -cvRotMat.at<double>(1, 2);
					//	cvRotMat.at<double>(2, 2) = -cvRotMat.at<double>(2, 2);

					//	cv::Rodrigues(cvRotMat, r);
					//}

					////std::cout << "Pose:\n" << GetMat4DebugString(&pose);
					//cv::Mat cvCamExts = cv::Mat::zeros(1, 6, CV_64F);
					//cvCamExts.at<double>(0) = r.at<double>(0);
					//cvCamExts.at<double>(1) = r.at<double>(1);
					//cvCamExts.at<double>(2) = r.at<double>(2);
					//cvCamExts.at<double>(3) = t.at<double>(0);
					//cvCamExts.at<double>(4) = -t.at<double>(1);
					//cvCamExts.at<double>(5) = -t.at<double>(2);
					//
					//camExts[hawkIter] = cvCamExts;

					camExts[hawkIter] = camViewMat;
				} else {
					foundPoseInBothEyes = false;
					break;
				}
			} else {
				foundPoseInBothEyes = false;
				break;
			}
		}

		if (foundPoseInBothEyes) {
			for (int hawkIter = 0; hawkIter < 2; ++hawkIter) {
				mat4 camLocalMat = camExts[hawkIter];
				cv::Mat reRvec = cv::Mat::zeros(3, 1, CV_64F);
				cv::Mat reTvec = cv::Mat::zeros(3, 1, CV_64F);
				cv::Mat reRotMat = cv::Mat::zeros(3, 3, CV_64F);

				if (hawkIter == 1) {
					mat4 cam0 = camExts[0];
					mat4 cam1 = camExts[1];
					
					cv::Mat reRvec0 = cv::Mat::zeros(3, 1, CV_64F);
					cv::Mat reTvec0 = cv::Mat::zeros(3, 1, CV_64F);
					cv::Mat reRotMat0 = cv::Mat::zeros(3, 3, CV_64F);

					cv::Mat reRvec1 = cv::Mat::zeros(3, 1, CV_64F);
					cv::Mat reTvec1 = cv::Mat::zeros(3, 1, CV_64F);
					cv::Mat reRotMat1 = cv::Mat::zeros(3, 3, CV_64F);

					// Convert cam mat to rvec, tvec.
					reTvec0.at<double>(0) = cam0[3][0];
					reTvec0.at<double>(1) = -cam0[3][1];
					reTvec0.at<double>(2) = -cam0[3][2];
					//std::cout << "Re T: " << reTvec << "\n";

					reRotMat0.at<double>(0, 0) = cam0[0][0];
					reRotMat0.at<double>(1, 0) = -cam0[0][1];
					reRotMat0.at<double>(2, 0) = -cam0[0][2];
					reRotMat0.at<double>(0, 1) = cam0[1][0];
					reRotMat0.at<double>(1, 1) = -cam0[1][1];
					reRotMat0.at<double>(2, 1) = -cam0[1][2];
					reRotMat0.at<double>(0, 2) = cam0[2][0];
					reRotMat0.at<double>(1, 2) = -cam0[2][1];
					reRotMat0.at<double>(2, 2) = -cam0[2][2];
					//std::cout << "ReRotMat: " << reRotMat << "\n";
					cv::Rodrigues(reRotMat0, reRvec0);
					//std::cout << "Re R: " << reRvec << "\n";

					// Convert cam mat to rvec, tvec.
					reTvec1.at<double>(0) = cam1[3][0];
					reTvec1.at<double>(1) = -cam1[3][1];
					reTvec1.at<double>(2) = -cam1[3][2];
					//std::cout << "Re T: " << reTvec << "\n";

					reRotMat1.at<double>(0, 0) = cam1[0][0];
					reRotMat1.at<double>(1, 0) = -cam1[0][1];
					reRotMat1.at<double>(2, 0) = -cam1[0][2];
					reRotMat1.at<double>(0, 1) = cam1[1][0];
					reRotMat1.at<double>(1, 1) = -cam1[1][1];
					reRotMat1.at<double>(2, 1) = -cam1[1][2];
					reRotMat1.at<double>(0, 2) = cam1[2][0];
					reRotMat1.at<double>(1, 2) = -cam1[2][1];
					reRotMat1.at<double>(2, 2) = -cam1[2][2];

					cv::Rodrigues(reRotMat1, reRvec1);
					//std::cout << "Re R: " << reRvec << "\n";

					//cv::composeRT(reRvec0, reTvec0, reRvec1, reTvec1, reRvec, reTvec);
					//reTvec = reTvec1;
					//reRotMat = reRotMat1;

					std::cout << "Cam1: " << reRvec1 << " :: " << reTvec1 << "\n";
					std::cout << "Diff: " << reRvec << " :: " << reTvec << "\n";

					mat4 diffMat(1.0f);
					mat4 cam0Mat(1.0f);
					mat4 cam1Mat(1.0f);

					{
						cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
						cv::Rodrigues(reRvec0, cvRotMat);

						cam0Mat[0][0] = cvRotMat.at<double>(0, 0);
						cam0Mat[0][1] = cvRotMat.at<double>(1, 0);
						cam0Mat[0][2] = cvRotMat.at<double>(2, 0);
						cam0Mat[1][0] = cvRotMat.at<double>(0, 1);
						cam0Mat[1][1] = cvRotMat.at<double>(1, 1);
						cam0Mat[1][2] = cvRotMat.at<double>(2, 1);
						cam0Mat[2][0] = cvRotMat.at<double>(0, 2);
						cam0Mat[2][1] = cvRotMat.at<double>(1, 2);
						cam0Mat[2][2] = cvRotMat.at<double>(2, 2);
						cam0Mat[3][0] = reTvec0.at<double>(0);
						cam0Mat[3][1] = reTvec0.at<double>(1);
						cam0Mat[3][2] = reTvec0.at<double>(2);
						cam0Mat[3][3] = 1.0;

						std::cout << "Cam0 mat: " << GetMat4DebugString(&cam0Mat) << "\n";
					}
					{
						cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
						cv::Rodrigues(reRvec1, cvRotMat);

						cam1Mat[0][0] = cvRotMat.at<double>(0, 0);
						cam1Mat[0][1] = cvRotMat.at<double>(1, 0);
						cam1Mat[0][2] = cvRotMat.at<double>(2, 0);
						cam1Mat[1][0] = cvRotMat.at<double>(0, 1);
						cam1Mat[1][1] = cvRotMat.at<double>(1, 1);
						cam1Mat[1][2] = cvRotMat.at<double>(2, 1);
						cam1Mat[2][0] = cvRotMat.at<double>(0, 2);
						cam1Mat[2][1] = cvRotMat.at<double>(1, 2);
						cam1Mat[2][2] = cvRotMat.at<double>(2, 2);
						cam1Mat[3][0] = reTvec1.at<double>(0);
						cam1Mat[3][1] = reTvec1.at<double>(1);
						cam1Mat[3][2] = reTvec1.at<double>(2);
						cam1Mat[3][3] = 1.0;

						std::cout << "Cam0 mat: " << GetMat4DebugString(&cam0Mat) << "\n";
					}

					mat4 invCam0 = glm::inverse(cam0Mat);
					camLocalMat = cam1Mat * invCam0;

					// Convert cam mat to rvec, tvec.
					reTvec.at<double>(0) = camLocalMat[3][0];
					reTvec.at<double>(1) = camLocalMat[3][1];
					reTvec.at<double>(2) = camLocalMat[3][2];
					//std::cout << "Re T: " << reTvec << "\n";

					reRotMat.at<double>(0, 0) = camLocalMat[0][0];
					reRotMat.at<double>(1, 0) = camLocalMat[0][1];
					reRotMat.at<double>(2, 0) = camLocalMat[0][2];
					reRotMat.at<double>(0, 1) = camLocalMat[1][0];
					reRotMat.at<double>(1, 1) = camLocalMat[1][1];
					reRotMat.at<double>(2, 1) = camLocalMat[1][2];
					reRotMat.at<double>(0, 2) = camLocalMat[2][0];
					reRotMat.at<double>(1, 2) = camLocalMat[2][1];
					reRotMat.at<double>(2, 2) = camLocalMat[2][2];
					//std::cout << "ReRotMat: " << reRotMat << "\n";

					cv::Rodrigues(reRotMat, reRvec);
					std::cout << "Re R: " << reRvec << "\n";

					{
						cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
						cv::Rodrigues(reRvec, cvRotMat);

						diffMat[0][0] = cvRotMat.at<double>(0, 0);
						diffMat[0][1] = cvRotMat.at<double>(1, 0);
						diffMat[0][2] = cvRotMat.at<double>(2, 0);
						diffMat[1][0] = cvRotMat.at<double>(0, 1);
						diffMat[1][1] = cvRotMat.at<double>(1, 1);
						diffMat[1][2] = cvRotMat.at<double>(2, 1);
						diffMat[2][0] = cvRotMat.at<double>(0, 2);
						diffMat[2][1] = cvRotMat.at<double>(1, 2);
						diffMat[2][2] = cvRotMat.at<double>(2, 2);
						diffMat[3][0] = reTvec.at<double>(0);
						diffMat[3][1] = reTvec.at<double>(1);
						diffMat[3][2] = reTvec.at<double>(2);
						diffMat[3][3] = 1.0;

						std::cout << "Diff mat: " << GetMat4DebugString(&diffMat) << "\n";
					}

					std::cout << "Cam1 mat: " << GetMat4DebugString(&cam1Mat) << "\n";

					mat4 recompCam1 = diffMat * cam0Mat;

					std::cout << "recompCam1 mat: " << GetMat4DebugString(&recompCam1) << "\n";

				} else {
					// Convert cam mat to rvec, tvec.
					reTvec.at<double>(0) = camLocalMat[3][0];
					reTvec.at<double>(1) = -camLocalMat[3][1];
					reTvec.at<double>(2) = -camLocalMat[3][2];
					//std::cout << "Re T: " << reTvec << "\n";

					reRotMat.at<double>(0, 0) = camLocalMat[0][0];
					reRotMat.at<double>(1, 0) = -camLocalMat[0][1];
					reRotMat.at<double>(2, 0) = -camLocalMat[0][2];
					reRotMat.at<double>(0, 1) = camLocalMat[1][0];
					reRotMat.at<double>(1, 1) = -camLocalMat[1][1];
					reRotMat.at<double>(2, 1) = -camLocalMat[1][2];
					reRotMat.at<double>(0, 2) = camLocalMat[2][0];
					reRotMat.at<double>(1, 2) = -camLocalMat[2][1];
					reRotMat.at<double>(2, 2) = -camLocalMat[2][2];
					//std::cout << "ReRotMat: " << reRotMat << "\n";

					cv::Rodrigues(reRotMat, reRvec);
					//std::cout << "Re R: " << reRvec << "\n";
				}

				cv::Mat cvCamExts = cv::Mat::zeros(1, 6, CV_64F);
				cvCamExts.at<double>(0) = reRvec.at<double>(0);
				cvCamExts.at<double>(1) = reRvec.at<double>(1);
				cvCamExts.at<double>(2) = reRvec.at<double>(2);
				cvCamExts.at<double>(3) = reTvec.at<double>(0);
				cvCamExts.at<double>(4) = reTvec.at<double>(1);
				cvCamExts.at<double>(5) = reTvec.at<double>(2);

				if (hawkIter == 0) {
					stereoSampleId.push_back(sampleIter);
					camExtrinsics.push_back(cvCamExts);
				} else {
					diffExtrinsics.push_back(cvCamExts);
				}

				viewId.push_back(stereoSampleId.size() - 1);
				camId.push_back(hawkIter);
				observations.push_back(imagePoints[hawkIter]);
				observationPointIds.push_back(imagePointIds[hawkIter]);
				totalImagePointCount += (int)imagePoints[hawkIter].size();
			}
		}
	}

	// Write file.
	FILE* f;
	//fopen_s(&f, "../cache/ba_input.txt", "w");
	fopen_s(&f, "C:/Projects/LDI/PyBA/ba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d %d\n", stereoSampleId.size(), initialCube.points.size(), totalImagePointCount);

	// Starting intrinsics.
	/*cv::Mat calibCameraMatrix = Job->defaultCamMat[hawkId];
	cv::Mat calibCameraDist = Job->defaultCamDist[hawkId];
	float focal = calibCameraMatrix.at<double>(0);
	float k1 = calibCameraDist.at<double>(0);
	float k2 = calibCameraDist.at<double>(1);*/

	{
		// Starting relative stereo pose
		// 6 params. r, t
		for (int i = 0; i < 6; ++i) {
			fprintf(f, "%f ", diffExtrinsics[0].at<double>(i));
		}

		fprintf(f, "\n");
	}

	// Observations.
	for (size_t viewIter = 0; viewIter < observations.size(); ++viewIter) {
		std::vector<cv::Point2f>* viewPoints = &observations[viewIter];

		for (size_t pointIter = 0; pointIter < observations[viewIter].size(); ++pointIter) {
			cv::Point2f point = observations[viewIter][pointIter];
			int pointId = observationPointIds[viewIter][pointIter];

			fprintf(f, "%d %d %d %f %f\n", viewId[viewIter], camId[viewIter], pointId, point.x - (3280 / 2), (2464 - point.y) - (2464 / 2));
		}
	}

	// View info
	for (size_t viewIter = 0; viewIter < stereoSampleId.size(); ++viewIter) {
		fprintf(f, "%d ", stereoSampleId[viewIter]);
		
		// Camera extrinsics
		// 6 params. r, t
		for (int i = 0; i < 6; ++i) {
			fprintf(f, "%f ", camExtrinsics[viewIter].at<double>(i));
		}

		fprintf(f, "\n");
	}

	// 3D points.
	for (size_t pointIter = 0; pointIter < initialCube.points.size(); ++pointIter) {
		vec3 point = initialCube.points[pointIter];
		fprintf(f, "%f %f %f\n", point.x, point.y, point.z);
	}
	
	fclose(f);
}

void calibLoadFullBA(ldiCalibrationJob* Job) {
	FILE* f;
	fopen_s(&f, "C:/Projects/LDI/PyBA/ba_result.txt", "r");

	int basePoseCount;
	int cubePointCount;

	fscanf_s(f, "%d %d\n", &basePoseCount, &cubePointCount);
	std::cout << "Base pose count: " << basePoseCount << " Cube point count: " << cubePointCount << "\n";

	// Cam intrinsics
	for (int i = 0; i < 2; ++i) {
		int camId = 0;
		double camInts[3];
		fscanf_s(f, "%d %lf %lf %lf\n", &camId, &camInts[0], &camInts[1], &camInts[2]);
		std::cout << "Cam intrins: " << camInts[0] << ", " << camInts[1] << ", " << camInts[2] << "\n";

		cv::Mat cam = cv::Mat::eye(3, 3, CV_64F);
		cam.at<double>(0, 0) = camInts[0];
		cam.at<double>(0, 1) = 0.0;
		cam.at<double>(0, 2) = 3280.0 / 2.0;
		cam.at<double>(1, 0) = 0.0;
		cam.at<double>(1, 1) = camInts[0];
		cam.at<double>(1, 2) = 2464.0 / 2.0;
		cam.at<double>(2, 0) = 0.0;
		cam.at<double>(2, 1) = 0.0;
		cam.at<double>(2, 2) = 1.0;

		cv::Mat dist = cv::Mat::zeros(8, 1, CV_64F);
		dist.at<double>(0) = camInts[1];
		dist.at<double>(1) = camInts[2];

		Job->refinedCamMat[i] = cam;
		Job->refinedCamDist[i] = dist;
	}

	mat4 cam0 = glm::identity<mat4>();
	mat4 cam1 = glm::identity<mat4>();

	{
		// Relative pose
		double pose[6];
		fscanf_s(f, "%lf %lf %lf %lf %lf %lf\n", &pose[0], &pose[1], &pose[2], &pose[3], &pose[4], &pose[5]);

		cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64F);
		rVec.at<double>(0) = pose[0];
		rVec.at<double>(1) = pose[1];
		rVec.at<double>(2) = pose[2];

		cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64F);
		tVec.at<double>(0) = pose[3];
		tVec.at<double>(1) = pose[4];
		tVec.at<double>(2) = pose[5];

		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rVec, cvRotMat);

		mat4 worldMat = glm::identity<mat4>();
		worldMat[0][0] = cvRotMat.at<double>(0, 0);
		worldMat[0][1] = cvRotMat.at<double>(1, 0);
		worldMat[0][2] = cvRotMat.at<double>(2, 0);
		worldMat[1][0] = cvRotMat.at<double>(0, 1);
		worldMat[1][1] = cvRotMat.at<double>(1, 1);
		worldMat[1][2] = cvRotMat.at<double>(2, 1);
		worldMat[2][0] = cvRotMat.at<double>(0, 2);
		worldMat[2][1] = cvRotMat.at<double>(1, 2);
		worldMat[2][2] = cvRotMat.at<double>(2, 2);
		worldMat[3][0] = tVec.at<double>(0);
		worldMat[3][1] = tVec.at<double>(1);
		worldMat[3][2] = tVec.at<double>(2);

		worldMat = glm::inverse(worldMat);

		worldMat[0][0] = worldMat[0][0];
		worldMat[0][1] = -worldMat[0][1];
		worldMat[0][2] = -worldMat[0][2];
		worldMat[1][0] = worldMat[1][0];
		worldMat[1][1] = -worldMat[1][1];
		worldMat[1][2] = -worldMat[1][2];
		worldMat[2][0] = worldMat[2][0];
		worldMat[2][1] = -worldMat[2][1];
		worldMat[2][2] = -worldMat[2][2];
		worldMat[3][0] = worldMat[3][0];
		worldMat[3][1] = -worldMat[3][1];
		worldMat[3][2] = -worldMat[3][2];

		cam1 = worldMat;

		// Swap view for cam
		cam1[1] = -cam1[1];
		cam1[2] = -cam1[2];
	}

	Job->stStereoCamWorld[0] = cam0;
	Job->stStereoCamWorld[1] = cam1;

	Job->stCubeWorlds.clear();
	Job->stPoseToSampleIds.clear();

	for (int i = 0; i < basePoseCount; ++i) {
		int sampleId;
		double pose[6];
		fscanf_s(f, "%d %lf %lf %lf %lf %lf %lf\n", &sampleId, &pose[0], &pose[1], &pose[2], &pose[3], &pose[4], &pose[5]);

		cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64F);
		rVec.at<double>(0) = pose[0];
		rVec.at<double>(1) = pose[1];
		rVec.at<double>(2) = pose[2];

		cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64F);
		tVec.at<double>(0) = pose[3];
		tVec.at<double>(1) = pose[4];
		tVec.at<double>(2) = pose[5];

		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rVec, cvRotMat);

		mat4 worldMat = glm::identity<mat4>();
		worldMat[0][0] = cvRotMat.at<double>(0, 0);
		worldMat[0][1] = -cvRotMat.at<double>(1, 0);
		worldMat[0][2] = -cvRotMat.at<double>(2, 0);
		worldMat[1][0] = cvRotMat.at<double>(0, 1);
		worldMat[1][1] = -cvRotMat.at<double>(1, 1);
		worldMat[1][2] = -cvRotMat.at<double>(2, 1);
		worldMat[2][0] = cvRotMat.at<double>(0, 2);
		worldMat[2][1] = -cvRotMat.at<double>(1, 2);
		worldMat[2][2] = -cvRotMat.at<double>(2, 2);
		worldMat[3][0] = tVec.at<double>(0);
		worldMat[3][1] = -tVec.at<double>(1);
		worldMat[3][2] = -tVec.at<double>(2);

		Job->stPoseToSampleIds.push_back(sampleId);
		Job->stCubeWorlds.push_back(worldMat);
	}

	Job->stCubePoints.clear();
	std::vector<vec3> cubePoints;

	for (int i = 0; i < cubePointCount; ++i) {
		int pointId;
		vec3 pos;
		fscanf_s(f, "%d %f %f %f\n", &pointId, &pos.x, &pos.y, &pos.z);
		std::cout << pointId << ": " << pos.x << ", " << pos.y << ", " << pos.z << "\n";

		cubePoints.push_back(pos);
		Job->stCubePoints.push_back(pos);
	}

	calibCubeInit(&Job->opCube);
	Job->opCube.points = cubePoints;
	calibCubeCalculateMetrics(&Job->opCube);

	// TODO: Make sure to scale view positions, relative pose, etc.
	fclose(f);
}