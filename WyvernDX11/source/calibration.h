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

	fwrite(&Job->stereoCalibrated, sizeof(bool), 1, file);
	if (Job->metricsCalculated) {
		/*serializeVectorPrep(file, Job->stBasisXPoints);
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
		
		fwrite(&Job->stVolumeCenter, sizeof(vec3), 1, file);*/
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

	fread(&Job->stereoCalibrated, sizeof(bool), 1, file);
	if (Job->metricsCalculated) {
		/*count = deserializeVectorPrep(file, Job->stBasisXPoints);
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
		
		fread(&Job->stVolumeCenter, sizeof(vec3), 1, file);*/
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
void calibBuildCalibVolumeMetrics(ldiApp* AppContext, ldiCalibrationJob* Job) {
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
		for (size_t i = 0; i < Job->stCubeWorlds.size(); ++i) {
			Job->stCubeWorlds[i] = volumeToWorld * Job->stCubeWorlds[i];

			//Job->stCubeWorlds[i][3] = vec4(0,0,0,1.0f);
		}

		// Average world axes?

		Job->cubePointCentroids.clear();
		for (size_t i = 0; i < _refinedModelPoints.size(); ++i) {
			vec3 point = vec3(_refinedModelPoints[i].x, _refinedModelPoints[i].y, _refinedModelPoints[i].z);
			point = Job->stCubeWorlds[0] * vec4(point.x, point.y, point.z, 1.0);

			//point = volumeToWorld * vec4(point.x, point.y, point.z, 1.0);

			Job->cubePointCentroids.push_back(point);
		}

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
void calibCalibrateScanner(ldiApp* AppContext, ldiCalibrationJob* Job) {
	for (size_t i = 0; i < Job->scanSamples.size(); ++i) {
		calibFreeStereoCalibImages(&Job->scanSamples[i]);
	}
	Job->scanSamples.clear();

	Job->scannerCalibrated = false;

	std::vector<std::string> filePaths = listAllFilesInDirectory("../cache/scanner_calib/");

	for (int i = 0; i < filePaths.size(); ++i) {
		if (endsWith(filePaths[i], ".dat")) {
			ldiCalibStereoSample sample = {};
			sample.path = filePaths[i];

			calibLoadStereoCalibSampleData(&sample);

			// TOOD: Process here.
			/*for (int j = 0; j < 2; ++j) {
				computerVisionFindCharuco(sample.frames[j], AppContext, &sample.cubes[j], &Hawks[j].defaultCameraMat, &Hawks[j].defaultDistMat);
			}*/

			calibFreeStereoCalibImages(&sample);

			Job->samples.push_back(sample);
		}
	}
}