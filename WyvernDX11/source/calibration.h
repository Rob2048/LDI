#pragma once

// Bundle adjust and non-linear optimization:
 
// https://github.com/Tetragramm/opencv_contrib/blob/master/modules/mapping3d/include/opencv2/mapping3d.hpp#L131

// https://gist.github.com/davegreenwood/4434757e97c518890c91b3d0fd9194bd
// https://scipy-cookbook.readthedocs.io/items/bundle_adjustment.html
// https://bitbucket.org/kaess/isam/src/master/examples/stereo.cpp
// http://people.csail.mit.edu/kaess/isam/
// https://github.com/mprib/pyxy3d/blob/main/pyxy3d/calibration/capture_volume/capture_volume.py

// https://colmap.github.io/tutorial.html
// https://www.cs.cornell.edu/~snavely/bundler/
// https://users.ics.forth.gr/~lourakis/sba/
// https://www.informatik.uni-marburg.de/~thormae/paper/MIRA11_2.pdf
// https://gist.github.com/davegreenwood/4434757e97c518890c91b3d0fd9194bd
// https://pypi.org/project/pyba/

// NOTE: Really in-depth camera calibration: http://mrcal.secretsauce.net/how-to-calibrate.html

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

void serialize(FILE* File, ldiModel* Model) {
	serializeVectorPrep(File, Model->verts);
	for (size_t i = 0; i < Model->verts.size(); ++i) {
		fwrite(&Model->verts[i], sizeof(ldiMeshVertex), 1, File);
	}

	serializeVectorPrep(File, Model->indices);
	for (size_t i = 0; i < Model->indices.size(); ++i) {
		fwrite(&Model->indices[i], sizeof(uint32_t), 1, File);
	}
}

void deserialize(FILE* File, ldiModel* Model) {
	deserializeVectorPrep(File, Model->verts);
	for (size_t i = 0; i < Model->verts.size(); ++i) {
		fread(&Model->verts[i], sizeof(ldiMeshVertex), 1, File);
	}

	deserializeVectorPrep(File, Model->indices);
	for (size_t i = 0; i < Model->indices.size(); ++i) {
		fread(&Model->indices[i], sizeof(uint32_t), 1, File);
	}
}

void serialize(FILE* File, ldiImage* Image, int Channels) {
	fwrite(&Image->width, sizeof(int), 1, File);
	fwrite(&Image->height, sizeof(int), 1, File);
	fwrite(Image->data, Image->width * Image->height * Channels, 1, File);
}

void deserialize(FILE* File, ldiImage* Image, int Channels) {
	fread(&Image->width, sizeof(int), 1, File);
	fread(&Image->height, sizeof(int), 1, File);
	Image->data = new uint8_t[Image->width * Image->height * Channels];
	fread(Image->data, Image->width * Image->height * Channels, 1, File);
}

void serializeCalibCube(FILE* File, ldiCalibCube* Cube) {
	fwrite(&Cube->cellSize, sizeof(float), 1, File);
	fwrite(&Cube->gridSize, sizeof(int), 1, File);
	fwrite(&Cube->pointsPerSide, sizeof(int), 1, File);
	fwrite(&Cube->totalPoints, sizeof(int), 1, File);
	fwrite(&Cube->scaleFactor, sizeof(float), 1, File);

	serializeVectorPrep(File, Cube->points);
	for (size_t i = 0; i < Cube->points.size(); ++i) {
		fwrite(&Cube->points[i], sizeof(vec3), 1, File);
	}

	serializeVectorPrep(File, Cube->sides);
	for (size_t i = 0; i < Cube->sides.size(); ++i) {
		fwrite(&Cube->sides[i], sizeof(ldiCalibCubeSide), 1, File);
	}

	fwrite(&Cube->corners, sizeof(ldiCalibCube::corners), 1, File);
}

void deserializeCalibCube(FILE* File, ldiCalibCube* Cube) {
	fread(&Cube->cellSize, sizeof(float), 1, File);
	fread(&Cube->gridSize, sizeof(int), 1, File);
	fread(&Cube->pointsPerSide, sizeof(int), 1, File);
	fread(&Cube->totalPoints, sizeof(int), 1, File);
	fread(&Cube->scaleFactor, sizeof(float), 1, File);

	deserializeVectorPrep(File, Cube->points);
	for (size_t i = 0; i < Cube->points.size(); ++i) {
		fread(&Cube->points[i], sizeof(vec3), 1, File);
	}

	deserializeVectorPrep(File, Cube->sides);
	for (size_t i = 0; i < Cube->sides.size(); ++i) {
		fread(&Cube->sides[i], sizeof(ldiCalibCubeSide), 1, File);
	}

	fread(&Cube->corners, sizeof(ldiCalibCube::corners), 1, File);
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

		serializeVectorPrep(file, Job->stCubeWorlds);
		for (size_t i = 0; i < Job->stCubeWorlds.size(); ++i) {
			fwrite(&Job->stCubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fwrite(&Job->stStereoCamWorld[i], sizeof(mat4), 1, file);
		}

		serializeCalibCube(file, &Job->opCube);
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

		count = deserializeVectorPrep(file, Job->stCubeWorlds);
		for (size_t i = 0; i < count; ++i) {
			fread(&Job->stCubeWorlds[i], sizeof(mat4), 1, file);
		}

		for (int i = 0; i < 2; ++i) {
			fread(&Job->stStereoCamWorld[i], sizeof(mat4), 1, file);
		}

		deserializeCalibCube(file, &Job->opCube);
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
	// Gather rotary axis samples.
	//----------------------------------------------------------------------------------------------------
	Job->axisCPoints.clear();
	Job->axisAPoints.clear();

	// Just for visualization.
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

	std::vector<vec3> circOriginsC;
	std::vector<vec3> circOriginsA;

	for (size_t pointIter = 0; pointIter < Job->opCube.points.size(); ++pointIter) {
		if (pointIter / 9 == 2) {
			continue;
		}

		std::vector<vec3d> axisPointsC;
		std::vector<vec3d> axisPointsA;

		for (size_t poseIter = 0; poseIter < Job->stPoseToSampleIds.size(); ++poseIter) {
			ldiCalibStereoSample* sample = &Job->samples[Job->stPoseToSampleIds[poseIter]];

			if (sample->phase == 2) {
				vec3d transPoint = mat4d(Job->stCubeWorlds[poseIter]) * vec4d(Job->opCube.points[pointIter], 1.0);
				axisPointsC.push_back(transPoint);
			} else if (sample->phase == 3) {
				vec3d transPoint = mat4d(Job->stCubeWorlds[poseIter]) * vec4d(Job->opCube.points[pointIter], 1.0);
				axisPointsA.push_back(transPoint);
			}
		}

		circOriginsC.push_back(computerVisionFitCircle(axisPointsC).origin);
		circOriginsA.push_back(computerVisionFitCircle(axisPointsA).origin);
	}

	ldiLine fitC = computerVisionFitLine(circOriginsC);
	Job->axisC.direction = fitC.direction;
	Job->axisC.origin = fitC.origin;
	
	ldiLine fitA = computerVisionFitLine(circOriginsA);
	Job->axisA.direction = fitA.direction;
	Job->axisA.origin = fitA.origin;

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

		// Axis C points.
		for (size_t i = 0; i < Job->axisCPoints.size(); ++i) {
			Job->axisCPoints[i] = volumeToWorld * vec4(Job->axisCPoints[i], 1.0f);
		}

		// Axis A points.
		for (size_t i = 0; i < Job->axisAPoints.size(); ++i) {
			Job->axisAPoints[i] = volumeToWorld * vec4(Job->axisAPoints[i], 1.0f);
		}

		// This axis always points negative on X.
		/*if (fitC.direction.x > 0.0f) {
			circle.normal = -circle.normal;
		}*/

		Job->axisC.origin = volumeToWorld * vec4(Job->axisC.origin, 1.0f);
		Job->axisC.direction = volumeToWorld * vec4(-Job->axisC.direction, 0.0f);
		//	std::cout << "Axis A - Origin: " << GetStr(Job->axisA.origin) << " Dir: " << GetStr(Job->axisA.direction) << "\n";

		Job->axisA.origin = volumeToWorld * vec4(Job->axisA.origin, 1.0f);
		Job->axisA.direction = volumeToWorld * vec4(-Job->axisA.direction, 0.0f);
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
void calibSaveFullBA(ldiCalibrationJob* Job, const std::string& FilePath) {
	std::vector<int> viewId;
	std::vector<int> stereoSampleId;
	std::vector<int> camId;
	std::vector<cv::Mat> camExtrinsics;
	std::vector<cv::Mat> diffExtrinsics;
	std::vector<std::vector<cv::Point2f>> observations;
	std::vector<std::vector<int>> observationPointIds;

	cv::Mat relativeMat = cv::Mat::zeros(1, 6, CV_64F);
	int relativeMatCount = 0;

	int totalImagePointCount = 0;

	ldiCalibCube initialCube;
	calibCubeInit(&initialCube);

	// Find a pose for each calibration sample.
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
	//for (size_t sampleIter = 0; sampleIter < 344; ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];

		std::vector<cv::Point2f> imagePoints[2];
		std::vector<int> imagePointIds[2];
		cv::Mat camExts[2];
		//mat4 camExts[2];

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
				cv::Mat r;
				cv::Mat t;
				
				std::cout << "Find pose - Sample: " << sampleIter << " Hawk: " << hawkIter << "\n";
				if (computerVisionFindGeneralPoseRT(&Job->defaultCamMat[hawkIter], &Job->defaultCamDist[hawkIter], &imagePoints[hawkIter], &worldPoints, &r, &t)) {
					camExts[hawkIter] = cv::Mat::zeros(1, 6, CV_64F);
					camExts[hawkIter].at<double>(0) = r.at<double>(0);
					camExts[hawkIter].at<double>(1) = r.at<double>(1);
					camExts[hawkIter].at<double>(2) = r.at<double>(2);
					camExts[hawkIter].at<double>(3) = t.at<double>(0);
					camExts[hawkIter].at<double>(4) = t.at<double>(1);
					camExts[hawkIter].at<double>(5) = t.at<double>(2);
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
				
				if (hawkIter == 0) {
					stereoSampleId.push_back(sampleIter);
					camExtrinsics.push_back(camExts[hawkIter]);
				} else {
					cv::Mat cam0 = convertRvecTvec(camExts[0]);
					cv::Mat cam1 = convertRvecTvec(camExts[1]);
					cam1 = cam1 * cam0.inv();

					cv::Mat relativeRT = convertTransformToRT(cam1);

					relativeMat += relativeRT;
					relativeMatCount += 1;

					diffExtrinsics.push_back(relativeRT);
					//diffExtrinsics.push_back(camExts[hawkIter]);
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
	fopen_s(&f, FilePath.c_str(), "w");
	//fopen_s(&f, "C:/Projects/LDI/PyBA/ba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d %d\n", (int)stereoSampleId.size(), (int)initialCube.points.size(), totalImagePointCount);

	// Starting intrinsics.
	for (int i = 0; i < 2; ++i) {
		//Job->defaultCamMat[i]

		for (int j = 0; j < 9; ++j) {
			fprintf(f, "%f ", Job->defaultCamMat[i].at<double>(j));
		}

		fprintf(f, "\n");

		for (int j = 0; j < 8; ++j) {
			fprintf(f, "%f ", Job->defaultCamDist[i].at<double>(j));
		}

		fprintf(f, "\n");
	}

	// Relative pose.
	{
		relativeMat /= (float)relativeMatCount;

		// Starting relative stereo pose
		// 6 params. r, t
		for (int i = 0; i < 6; ++i) {
			fprintf(f, "%f ", relativeMat.at<double>(i));
		}

		fprintf(f, "\n");
	}

	// Observations.
	for (size_t viewIter = 0; viewIter < observations.size(); ++viewIter) {
		std::vector<cv::Point2f>* viewPoints = &observations[viewIter];

		for (size_t pointIter = 0; pointIter < observations[viewIter].size(); ++pointIter) {
			cv::Point2f point = observations[viewIter][pointIter];
			int pointId = observationPointIds[viewIter][pointIter];

			fprintf(f, "%d %d %d %f %f\n", viewId[viewIter], camId[viewIter], pointId, point.x, point.y);
		}
	}

	// Base poses.
	for (size_t viewIter = 0; viewIter < stereoSampleId.size(); ++viewIter) {
		fprintf(f, "%d ", stereoSampleId[viewIter]);
		
		// Camera extrinsics.
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

void calibLoadFullBA(ldiCalibrationJob* Job, const std::string& FilePath) {
	FILE* f;
	fopen_s(&f, FilePath.c_str(), "r");

	if (f == 0) {
		std::cout << "Could not open bundle adjust file.\n";
		return;
	}

	int basePoseCount;
	int cubePointCount;

	fscanf_s(f, "%d %d\n", &basePoseCount, &cubePointCount);
	std::cout << "Base pose count: " << basePoseCount << " Cube point count: " << cubePointCount << "\n";

	// Cam intrinsics
	for (int i = 0; i < 2; ++i) {
		double camInts[4];
		double camDist[4];
		fscanf_s(f, "%lf %lf %lf %lf\n", &camInts[0], &camInts[1], &camInts[2], &camInts[3]);
		fscanf_s(f, "%lf %lf %lf %lf\n", &camDist[0], &camDist[1], &camDist[2], &camDist[3]);
		
		cv::Mat cam = cv::Mat::eye(3, 3, CV_64F);
		cam.at<double>(0, 0) = camInts[0];
		cam.at<double>(0, 1) = 0.0;
		cam.at<double>(0, 2) = camInts[2];
		cam.at<double>(1, 0) = 0.0;
		cam.at<double>(1, 1) = camInts[1];
		cam.at<double>(1, 2) = camInts[3];
		cam.at<double>(2, 0) = 0.0;
		cam.at<double>(2, 1) = 0.0;
		cam.at<double>(2, 2) = 1.0;

		cv::Mat dist = cv::Mat::zeros(8, 1, CV_64F);
		dist.at<double>(0) = camDist[0];
		dist.at<double>(1) = camDist[1];
		dist.at<double>(2) = camDist[2];
		dist.at<double>(3) = camDist[3];

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
		cam1 = worldMat;
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

		// NOTE: Had Y/Z negated
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

		Job->stPoseToSampleIds.push_back(sampleId);
		Job->stCubeWorlds.push_back(worldMat);

		mat4d worldMatD = glm::identity<mat4>();
		worldMatD[0][0] = cvRotMat.at<double>(0, 0);
		worldMatD[0][1] = cvRotMat.at<double>(1, 0);
		worldMatD[0][2] = cvRotMat.at<double>(2, 0);
		worldMatD[1][0] = cvRotMat.at<double>(0, 1);
		worldMatD[1][1] = cvRotMat.at<double>(1, 1);
		worldMatD[1][2] = cvRotMat.at<double>(2, 1);
		worldMatD[2][0] = cvRotMat.at<double>(0, 2);
		worldMatD[2][1] = cvRotMat.at<double>(1, 2);
		worldMatD[2][2] = cvRotMat.at<double>(2, 2);
		worldMatD[3][0] = tVec.at<double>(0);
		worldMatD[3][1] = tVec.at<double>(1);
		worldMatD[3][2] = tVec.at<double>(2);
	}

	std::vector<vec3> cubePoints;

	for (int i = 0; i < cubePointCount; ++i) {
		int pointId;
		vec3 pos;
		fscanf_s(f, "%d %f %f %f\n", &pointId, &pos.x, &pos.y, &pos.z);
		std::cout << pointId << ": " << pos.x << ", " << pos.y << ", " << pos.z << "\n";

		cubePoints.push_back(pos);
	}

	calibCubeInit(&Job->opCube);
	Job->opCube.points = cubePoints;
	calibCubeCalculateMetrics(&Job->opCube);

	// TODO: Make sure to scale view positions, relative pose, etc.
	fclose(f);
}

// Calculate stereo camera intrinsics, extrinsics, and cube transforms.
void calibStereoCalibrate(ldiCalibrationJob* Job) {
	std::cout << "Starting stereo calibration: " << getTime() << "\n";

	Job->stereoCalibrated = false;
	Job->stPoseToSampleIds.clear();
	Job->stCubeWorlds.clear();

	calibSaveFullBA(Job, "../cache/ba_input.txt");

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char args[2048];
	sprintf_s(args, "python bundleAdjust.py ../../bin/cache/ba_input.txt ../../bin/cache/ba_refined.txt");

	CreateProcessA(
		NULL,
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		"../../assets/bin",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);

	calibLoadFullBA(Job, "../cache/ba_refined.txt");

	Job->stereoCalibrated = true;
}

void calibGetInitialEstimations(ldiCalibrationJob* Job, ldiHawk* Hawks) {
	// Unknown parameters (That need initial estimations):
	// - X axis direction.
	// - Y axis direction.
	// - Z axis direction.
	// - C axis direction and origin.
	// - A axis direction and origin.
	// - Cube 3D point positions.
	// - Camera intrinsics (camera matrix, lens distortion).
	// - Camera extrinsics (position, rotation).

	int hawkId = 1;

	Job->stPoseToSampleIds.clear();
	Job->stCubeWorlds.clear();

	//----------------------------------------------------------------------------------------------------
	// Find a pose for each calibration sample.
	//----------------------------------------------------------------------------------------------------
	ldiCalibCube initialCube;
	calibCubeInit(&initialCube);
	Job->opCube = initialCube;

	Job->stStereoCamWorld[0] = glm::identity<mat4>();
	Job->stStereoCamWorld[1] = glm::identity<mat4>();

	Job->refinedCamMat[0] = Job->defaultCamMat[0].clone();
	Job->refinedCamDist[0] = Job->defaultCamDist[0].clone();

	Job->refinedCamMat[1] = Job->defaultCamMat[1].clone();
	Job->refinedCamDist[1] = Job->defaultCamDist[1].clone();

	std::cout << "Find pose for each sample\n";

	std::vector<std::vector<cv::Point2f>> viewObservations;
	std::vector<std::vector<int>> viewPointIds;
	std::vector<ldiHorsePositionAbs> viewPositions;

	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[sampleIter];
		std::vector<cv::Point2f> imagePoints;
		std::vector<int> imagePointIds;
		std::vector<cv::Point3f> worldPoints;
		
		cv::Mat poseRT;

		std::vector<ldiCharucoBoard>* boards = &sample->cubes[hawkId].boards;
		for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
			ldiCharucoBoard* board = &(*boards)[boardIter];

			for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
				ldiCharucoCorner* corner = &board->corners[cornerIter];
				int cornerGlobalId = (board->id * 9) + corner->id;
				vec3 targetPoint = initialCube.points[cornerGlobalId];

				imagePoints.push_back(cv::Point2f(corner->position.x, corner->position.y));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);
			}
		}

		if (imagePoints.size() >= 6) {
			cv::Mat r;
			cv::Mat t;

			if (computerVisionFindGeneralPoseRT(&Job->defaultCamMat[hawkId], &Job->defaultCamDist[hawkId], &imagePoints, &worldPoints, &r, &t)) {
				std::cout << "Find pose - Sample: " << sampleIter << "\n";

				poseRT = cv::Mat::zeros(1, 6, CV_64F);
				poseRT.at<double>(0) = r.at<double>(0);
				poseRT.at<double>(1) = r.at<double>(1);
				poseRT.at<double>(2) = r.at<double>(2);
				poseRT.at<double>(3) = t.at<double>(0);
				poseRT.at<double>(4) = t.at<double>(1);
				poseRT.at<double>(5) = t.at<double>(2);

				cv::Mat camTransMat = convertRvecTvec(poseRT);
				//cv::Mat cubeTransMat = camTransMat;//.inv();
				mat4 cubeMat = convertOpenCvTransMatToGlmMat(camTransMat);
				Job->stCubeWorlds.push_back(cubeMat);
				Job->stPoseToSampleIds.push_back(sampleIter);

				// Add to BA output.
				if (sample->phase == 1) {
					// Save positions in absolute values.
					double stepsToCm = 1.0 / ((200 * 32) / 0.8);

					ldiHorsePositionAbs platformPos = {};
					platformPos.x = sample->X * stepsToCm;
					platformPos.y = sample->Y * stepsToCm;
					platformPos.z = sample->Z * stepsToCm;
					platformPos.a = 0;
					platformPos.c = 0;

					viewObservations.push_back(imagePoints);
					viewPointIds.push_back(imagePointIds);
					viewPositions.push_back(platformPos);
				}
			} else {
				std::cout << "Find pose - Sample: " << sampleIter << ": Not found\n";
			}
		} else {
			std::cout << "Find pose - Sample: " << sampleIter << ": Not found\n";
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Get volume metrics.
	//----------------------------------------------------------------------------------------------------
	calibBuildCalibVolumeMetrics(Job);

	//----------------------------------------------------------------------------------------------------
	// Create BA file.
	//----------------------------------------------------------------------------------------------------
	FILE* f;
	fopen_s(&f, "../cache/new_ba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d\n", (int)initialCube.points.size(), (int)viewPositions.size());

	// Axis directions.
	fprintf(f, "%f %f %f\n", Job->axisX.direction.x, Job->axisX.direction.y, Job->axisX.direction.z);
	fprintf(f, "%f %f %f\n", Job->axisY.direction.x, Job->axisY.direction.y, Job->axisY.direction.z);
	fprintf(f, "%f %f %f\n", Job->axisZ.direction.x, Job->axisZ.direction.y, Job->axisZ.direction.z);
	
	// Starting intrinsics.
	for (int j = 0; j < 9; ++j) {
		fprintf(f, "%f ", Job->defaultCamMat[hawkId].at<double>(j));
	}
	fprintf(f, "\n");

	for (int j = 0; j < 8; ++j) {
		fprintf(f, "%f ", Job->defaultCamDist[hawkId].at<double>(j));
	}
	fprintf(f, "\n");

	// Starting camera transform.
	auto invMat = glm::inverse(Job->camVolumeMat[hawkId]);
	cv::Mat camExt = convertTransformToRT(convertGlmTransMatToOpenCvMat(invMat));
	// 6 params. r, t
	for (int i = 0; i < 6; ++i) {
		fprintf(f, "%f ", camExt.at<double>(i));
	}
	fprintf(f, "\n");

	// 3D points.
	for (size_t pointIter = 0; pointIter < initialCube.points.size(); ++pointIter) {
		vec3 point = initialCube.points[pointIter];
		fprintf(f, "%f %f %f\n", point.x, point.y, point.z);
	}

	// Views.
	for (size_t viewIter = 0; viewIter < viewPositions.size(); ++viewIter) {
		ldiHorsePositionAbs pos = viewPositions[viewIter];
		fprintf(f, "%d %f %f %f %f %f\n", (int)viewObservations[viewIter].size(), pos.x, pos.y, pos.z, pos.a, pos.c);
		
		std::vector<cv::Point2f>* viewPoints = &viewObservations[viewIter];
		for (size_t pointIter = 0; pointIter < viewObservations[viewIter].size(); ++pointIter) {
			cv::Point2f point = viewObservations[viewIter][pointIter];
			int pointId = viewPointIds[viewIter][pointIter];

			fprintf(f, "%d %f %f\n", pointId, point.x, point.y);
		}
	}

	fclose(f);
}

void calibLoadNewBA(ldiCalibrationJob* Job, const std::string& FilePath) {
	FILE* f;

	fopen_s(&f, FilePath.c_str(), "r");

	if (f == 0) {
		std::cout << "Could not open bundle adjust file.\n";
		return;
	}

	int basePoseCount;
	int cubePointCount;

	fscanf_s(f, "%d\n", &cubePointCount);
	std::cout << "Cube point count: " << cubePointCount << "\n";

	// Cam intrinsics
	double camInts[4];
	double camDist[4];
	fscanf_s(f, "%lf %lf %lf %lf\n", &camInts[0], &camInts[1], &camInts[2], &camInts[3]);
	fscanf_s(f, "%lf %lf %lf %lf\n", &camDist[0], &camDist[1], &camDist[2], &camDist[3]);

	cv::Mat cam = cv::Mat::eye(3, 3, CV_64F);
	cam.at<double>(0, 0) = camInts[0];
	cam.at<double>(0, 1) = 0.0;
	cam.at<double>(0, 2) = camInts[2];
	cam.at<double>(1, 0) = 0.0;
	cam.at<double>(1, 1) = camInts[1];
	cam.at<double>(1, 2) = camInts[3];
	cam.at<double>(2, 0) = 0.0;
	cam.at<double>(2, 1) = 0.0;
	cam.at<double>(2, 2) = 1.0;

	cv::Mat dist = cv::Mat::zeros(8, 1, CV_64F);
	dist.at<double>(0) = camDist[0];
	dist.at<double>(1) = camDist[1];
	dist.at<double>(2) = camDist[2];
	dist.at<double>(3) = camDist[3];

	Job->refinedCamMat[0] = cam;
	Job->refinedCamDist[0] = dist;

	{
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
		Job->stStereoCamWorld[0] = worldMat;
		Job->camVolumeMat[0] = worldMat;
	}

	Job->cubeWorlds[0] = glm::identity<mat4>();

	Job->stCubeWorlds.clear();
	Job->stPoseToSampleIds.clear();

	// Axis directions.
	Job->axisX.origin = vec3Zero;
	Job->axisY.origin = vec3Zero;
	Job->axisZ.origin = vec3Zero;
	fscanf_s(f, "%f %f %f\n", &Job->axisX.direction.x, &Job->axisX.direction.y, &Job->axisX.direction.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisY.direction.x, &Job->axisY.direction.y, &Job->axisY.direction.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisZ.direction.x, &Job->axisZ.direction.y, &Job->axisZ.direction.z);
	
	std::vector<vec3> cubePoints;

	for (int i = 0; i < cubePointCount; ++i) {
		int pointId;
		vec3 pos;
		fscanf_s(f, "%d %f %f %f\n", &pointId, &pos.x, &pos.y, &pos.z);
		std::cout << pointId << ": " << pos.x << ", " << pos.y << ", " << pos.z << "\n";

		cubePoints.push_back(pos);
	}

	calibCubeInit(&Job->opCube);
	Job->opCube.points = cubePoints;
	calibCubeCalculateMetrics(&Job->opCube);

	fclose(f);

	double stepsToCm = 1.0 / ((200 * 32) / 0.8);
	Job->stepsToCm = glm::f64vec3(stepsToCm, stepsToCm, stepsToCm);
}