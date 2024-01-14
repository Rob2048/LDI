#pragma once

#include "calibCube.h"

struct ldiCharucoMarker {
	int id;
	vec2 corners[4];
};

struct ldiCharucoCorner {
	int id;
	int globalId;
	vec2 position;
};

struct ldiCharucoBoard {
	int id;
	std::vector<ldiCharucoCorner> corners;
	bool localMat;
	mat4 camLocalMat;
	bool rejected;
	mat4 camLocalCharucoEstimatedMat;
	vec2 charucoEstimatedImageCenter;
	vec2 charucoEstimatedImageNormal;
	float charucoEstimatedBoardAngle;
	std::vector<vec2> reprojectdCorners;
	std::vector<vec2> outline;
};

struct ldiCharucoResults {
	std::vector<ldiCharucoMarker> markers;
	std::vector<ldiCharucoMarker> rejectedMarkers;
	std::vector<ldiCharucoBoard> boards;
	std::vector<ldiCharucoBoard> rejectedBoards;
};

struct ldiCameraCalibSample {
	ldiImage* image;
	std::vector<cv::Point3f> worldPoints;
	std::vector<vec2> imagePoints;
	std::vector<cv::Point2f> undistortedImagePoints;
	std::vector<cv::Point2f> reprojectedImagePoints;
	mat4 camLocalMat;
	mat4 simTargetCamLocalMat;
	cv::Mat rvec;
	cv::Mat tvec;
};

struct ldiCalibSample {
	std::string path;
	int phase;
	int X;
	int Y;
	int Z;
	int C;
	int A;
	bool imageLoaded = false;
	ldiImage frame;
	ldiCharucoResults cube;
};

struct ldiCalibrationJob {
	std::vector<ldiCalibSample> samples;

	cv::Mat camMat;
	cv::Mat camDist;
	mat4 camVolumeMat;

	ldiCalibCube cube;

	std::vector<int> poseToSampleIds;
	std::vector<mat4> cubePoses;

	ldiLine axisX;
	ldiLine axisY;
	ldiLine axisZ;
	ldiLine axisC;
	ldiLine axisA;

	vec3 basisX;
	vec3 basisY;
	vec3 basisZ;

	glm::f64vec3 stepsToCm;

	std::vector<ldiCalibSample> scanSamples;
	ldiPlane scanPlane;

	bool initialEstimations;
	bool metricsCalculated;
	bool scannerCalibrated;
	bool galvoCalibrated;

	//----------------------------------------------------------------------------------------------------
	// Other data - not serialized.
	//----------------------------------------------------------------------------------------------------
	std::vector<vec3> axisXPoints;
	std::vector<vec3> axisYPoints;
	std::vector<vec3> axisZPoints;
	std::vector<vec3> axisCPoints;
	std::vector<vec3> axisAPoints;

	std::vector<std::vector<vec2>> scanPoints;
	std::vector<ldiLine> scanRays;
	std::vector<vec3> scanWorldPoints;

	std::vector<vec2> projObs;
	std::vector<cv::Point2f> projReproj;
	std::vector<double> projError;
};

void calibSaveCalibImage(ldiImage* Image, int X, int Y, int Z, int C, int A, int Phase, int ImgId, const std::string& Directory = "volume_calib") {
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

	fwrite(&Image->width, sizeof(int), 1, file);
	fwrite(&Image->height, sizeof(int), 1, file);
	fwrite(Image->data, 1, Image->width * Image->height, file);

	fclose(file);
}

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

void calibLoadCalibSampleData(ldiCalibSample* Sample) {
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

	Sample->frame.data = new uint8_t[width * height];
	Sample->frame.width = width;
	Sample->frame.height = height;

	fread(Sample->frame.data, width * height, 1, file);

	Sample->imageLoaded = true;

	fclose(file);
}

void calibFreeCalibImages(ldiCalibSample* Sample) {
	if (Sample->imageLoaded) {
		delete[] Sample->frame.data;
		Sample->imageLoaded = false;
	}
}

void calibLoadSampleImages(ldiCalibSample* Sample) {
	calibFreeCalibImages(Sample);

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

	Sample->frame.data = new uint8_t[width * height];
	Sample->frame.width = width;
	Sample->frame.height = height;

	fread(Sample->frame.data, width * height, 1, file);

	Sample->imageLoaded = true;

	fclose(file);
}

void calibClearJob(ldiCalibrationJob* Job) {
	for (size_t i = 0; i < Job->samples.size(); ++i) {
		calibFreeCalibImages(&Job->samples[i]);
	}
	Job->samples.clear();

	for (size_t i = 0; i < Job->scanSamples.size(); ++i) {
		calibFreeCalibImages(&Job->scanSamples[i]);
	}
	Job->scanSamples.clear();

	Job->initialEstimations = false;
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
void calibSaveCalibJob(const std::string& FilePath, ldiCalibrationJob* Job) {
	FILE* file;
	fopen_s(&file, FilePath.c_str(), "wb");

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

		auto chRes = &sample->cube;

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

	serializeMat(file, Job->camMat);
	serializeMat(file, Job->camDist);
	fwrite(&Job->camVolumeMat, sizeof(mat4), 1, file);

	serializeCalibCube(file, &Job->cube);

	serializeVectorPrep(file, Job->poseToSampleIds);
	for (size_t i = 0; i < Job->poseToSampleIds.size(); ++i) {
		fwrite(&Job->poseToSampleIds[i], sizeof(int), 1, file);
	}

	serializeVectorPrep(file, Job->cubePoses);
	for (size_t i = 0; i < Job->cubePoses.size(); ++i) {
		fwrite(&Job->cubePoses[i], sizeof(mat4), 1, file);
	}

	fwrite(&Job->axisX, sizeof(ldiLine), 1, file);
	fwrite(&Job->axisY, sizeof(ldiLine), 1, file);
	fwrite(&Job->axisZ, sizeof(ldiLine), 1, file);
	fwrite(&Job->axisC, sizeof(ldiLine), 1, file);
	fwrite(&Job->axisA, sizeof(ldiLine), 1, file);

	fwrite(&Job->basisX, sizeof(vec3), 1, file);
	fwrite(&Job->basisY, sizeof(vec3), 1, file);
	fwrite(&Job->basisZ, sizeof(vec3), 1, file);

	fwrite(&Job->stepsToCm, sizeof(glm::f64vec3), 1, file);

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

	fwrite(&Job->initialEstimations, sizeof(bool), 1, file);
	fwrite(&Job->metricsCalculated, sizeof(bool), 1, file);
	fwrite(&Job->scannerCalibrated, sizeof(bool), 1, file);
	fwrite(&Job->galvoCalibrated, sizeof(bool), 1, file);

	fclose(file);
}

// Loads entire state of the calibration job.
bool calibLoadCalibJob(const std::string& FilePath, ldiCalibrationJob* Job) {
	calibClearJob(Job);

	FILE* file;
	fopen_s(&file, FilePath.c_str(), "rb");

	if (file == 0) {
		std::cout << "Could not open job file: " << FilePath << "\n";
		return false;
	}

	size_t sampleCount = deserializeVectorPrep(file);
	for (size_t sampleIter = 0; sampleIter < sampleCount; ++sampleIter) {
		ldiCalibSample sample = {};

		sample.path = deserializeString(file);

		fread(&sample.phase, sizeof(int), 1, file);
		fread(&sample.X, sizeof(int), 1, file);
		fread(&sample.Y, sizeof(int), 1, file);
		fread(&sample.Z, sizeof(int), 1, file);
		fread(&sample.C, sizeof(int), 1, file);
		fread(&sample.A, sizeof(int), 1, file);

		auto chRes = &sample.cube;

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

		Job->samples.push_back(sample);
	}

	Job->camMat = deserializeMat(file);
	Job->camDist = deserializeMat(file);
	fread(&Job->camVolumeMat, sizeof(mat4), 1, file);

	deserializeCalibCube(file, &Job->cube);

	size_t count = deserializeVectorPrep(file, Job->poseToSampleIds);
	for (size_t i = 0; i < count; ++i) {
		fread(&Job->poseToSampleIds[i], sizeof(int), 1, file);
	}

	count = deserializeVectorPrep(file, Job->cubePoses);
	for (size_t i = 0; i < count; ++i) {
		fread(&Job->cubePoses[i], sizeof(mat4), 1, file);
	}

	fread(&Job->axisX, sizeof(ldiLine), 1, file);
	fread(&Job->axisY, sizeof(ldiLine), 1, file);
	fread(&Job->axisZ, sizeof(ldiLine), 1, file);
	fread(&Job->axisC, sizeof(ldiLine), 1, file);
	fread(&Job->axisA, sizeof(ldiLine), 1, file);

	fread(&Job->basisX, sizeof(vec3), 1, file);
	fread(&Job->basisY, sizeof(vec3), 1, file);
	fread(&Job->basisZ, sizeof(vec3), 1, file);

	fread(&Job->stepsToCm, sizeof(glm::f64vec3), 1, file);

	count = deserializeVectorPrep(file, Job->scanSamples);
	for (size_t sampleIter = 0; sampleIter < count; ++sampleIter) {
		ldiCalibSample sample = {};

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

	fread(&Job->initialEstimations, sizeof(bool), 1, file);
	fread(&Job->metricsCalculated, sizeof(bool), 1, file);
	fread(&Job->scannerCalibrated, sizeof(bool), 1, file);
	fread(&Job->galvoCalibrated, sizeof(bool), 1, file);

	fclose(file);

	return true;
}