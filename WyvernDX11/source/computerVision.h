#pragma once

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <fstream>

struct ldiLineFit {
	vec3 origin;
	vec3 direction;
};

struct ldiLineSegment {
	vec3 a;
	vec3 b;
};

struct ldiPlane {
	vec3 point;
	vec3 normal;
};

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
};

struct ldiCharucoResults {
	std::vector<ldiCharucoMarker> markers;
	std::vector<ldiCharucoBoard> boards;
};

struct ldiCalibCubePoint {
	int id;
	int boardId;
	int globalId;
	vec3 position;
};

struct ldiCalibCube {
	float boardTotalSizeCm;
	float boardSizeCm;
	float boardSquareSizeCm;
	int boardSquareCount;
	int boardPointXCount;
	int boardPointTotalCount;
	std::vector<ldiCalibCubePoint> points;

	ldiPlane sidePlanes[5];
};

struct ldiPhysicalCamera {
	int calibrationData;
};

struct ldiCalibSample {
	float x;
	float y;
	float z;
	float a;
	float b;
	ldiCharucoResults charucos;
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

struct ldiCalibStereoSample {
	std::string path;
	int X;
	int Y;
	int Z;
	int C;
	int A;
	bool imageLoaded = false;
	ldiImage frames[2];
	ldiCharucoResults cubes[2];
};

struct ldiStereoPair {
	mat4 cam0world;
	mat4 cam1world;

	int sampleId;
};

struct ldiCalibrationJob {
	std::vector<ldiCalibStereoSample> samples;

	cv::Mat startCamMat[2];
	cv::Mat startDistMat[2];

	std::vector<int> massModelPointIds;
	std::vector<vec3> massModelTriangulatedPoints;
	std::vector<vec3> massModelTriangulatedPointsBundleAdjust;
	std::vector<int> massModelBundleAdjustPointIds;
	std::vector<vec2> massModelImagePoints[2];
	std::vector<vec2> massModelUndistortedPoints[2];

	cv::Mat rtMat[2];

	std::vector<std::vector<vec3>> centeredPointGroups;
	
	ldiLineFit axisX;
	ldiLineFit axisY;
	ldiLineFit axisZ;

	vec3 basisX;
	vec3 basisY;
	vec3 basisZ;

	mat4 camVolumeMat[2];

	cv::Mat camMatrix[2];
	cv::Mat camDist[2];

	std::vector<vec3> cubePointCentroids;
	std::vector<int> cubePointCounts;
	vec3 cubePosSteps;

	glm::f64vec3 stepsToCm;

	std::vector<vec3> baIndvCubePoints;
	std::vector<mat4> baViews;
	std::vector<int> baViewIds;
	std::vector<ldiStereoPair> baStereoPairs;
	mat4 baStereoCamWorld[2];

	// Stereo calibrate test.
	std::vector<vec3> stCubePoints;
	std::vector<mat4> stCubeWorlds;
	mat4 stStereoCamWorld[2];
	std::vector<vec3> stBasisXPoints;
	std::vector<vec3> stBasisYPoints;
	std::vector<vec3> stBasisZPoints;
};

struct ldiCameraCalibrationContext {
	ldiImage* calibrationTargetImage;
};

struct ldiCalibTargetInfo {
	ldiPlane plane;
	std::vector<ldiLineSegment> fits;
	vec3 basisX;
	vec3 basisY;
	vec3 basisZ;

	vec3 basisXortho;
	vec3 basisYortho;
};

struct ldiBundleAdjustResult {
	cv::Mat cameraMatrix;
	cv::Mat distCoeffs;
	std::vector<vec3> modelPoints;
	std::vector<ldiCameraCalibSample> samples;
	
	ldiCalibTargetInfo targetInfo;
};

struct ldiCalibrationContext {
	ldiBundleAdjustResult bundleAdjustResult;
	
	ldiCalibrationJob calibJob;
};

auto _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

ldiImage _calibrationTargetImage;
float _calibrationTargetFullWidthCm;
float _calibrationTargetFullHeightCm;
std::vector<vec3> _calibrationTargetLocalPoints;

void computerVisionFitPlane(std::vector<vec3>& Points, ldiPlane* ResultPlane);

mat4 cameraCreateProjectionFromOpenCvCamera(float Width, float Height, cv::Mat& CameraMatrix, float Near, float Far) {
	mat4 result = glm::identity<mat4>();

	float Fx = CameraMatrix.at<double>(0, 0);
	float Fy = CameraMatrix.at<double>(1, 1);
	float Cx = CameraMatrix.at<double>(0, 2);
	float Cy = CameraMatrix.at<double>(1, 2);

	result[0][0] = 2 * (Fx / Width);
	result[0][1] = 0;
	result[0][2] = 0;
	result[0][3] = 0;

	result[1][0] = 0;
	result[1][1] = -2 * (Fy / Height);
	result[1][2] = 0;
	result[1][3] = 0;

	result[2][0] = (Width - 2.0f * Cx) / Width;
	result[2][1] = (Height - 2.0f * Cy) / -Height;
	result[2][2] = Far / (Near - Far);
	result[2][3] = -1;

	result[3][0] = 0;
	result[3][1] = 0;
	result[3][2] = -(Far * Near) / (Far - Near);
	result[3][3] = 0;

	return result;
}

mat4 cameraProjectionToVirtualViewport(mat4 ProjMat, vec2 ViewPortTopLeft, vec2 ViewPortSize, vec2 WindowSize) {
	float scaleX = ViewPortSize.x / WindowSize.x;
	float scaleY = ViewPortSize.y / WindowSize.y;

	// Scale existing FOV.
	ProjMat[0][0] *= scaleX;
	ProjMat[1][1] *= scaleY;

	// Scale existing shift.
	ProjMat[2][0] *= scaleX;
	ProjMat[2][1] *= scaleY;

	// Add new shift so that middle of virtual viewport is Top left: 1, -1, Bottom right: -1, 1
	ProjMat[2][0] -= ((ViewPortTopLeft.x + ViewPortSize.x * 0.5f) / ViewPortSize.x) * 2 * scaleX - 1;
	ProjMat[2][1] += ((ViewPortTopLeft.y + ViewPortSize.y * 0.5f) / ViewPortSize.y) * 2 * scaleY - 1;

	return ProjMat;
}

void calibCubeInit(ldiCalibCube* Cube, float SquareSizeCm, int SquareCount, float TotalSideSizeCm) {
	Cube->boardSquareSizeCm = SquareSizeCm;
	Cube->boardSquareCount = SquareCount;
	Cube->boardTotalSizeCm = TotalSideSizeCm;
	Cube->boardSizeCm = SquareSizeCm * SquareCount;
	Cube->boardPointXCount = SquareCount - 1;
	Cube->boardPointTotalCount = Cube->boardPointXCount * Cube->boardPointXCount;
	Cube->points.clear();
}

void calibCubeCalculatePlanes(ldiCalibCube* Cube) {
	for (int i = 0; i < 5; ++i) {
		std::vector<vec3> points;

		for (int j = 0; j < 9; ++j) {
			points.push_back(Cube->points[i * 9 + j].position);
		}

		computerVisionFitPlane(points, &Cube->sidePlanes[i]);
	}

	// TODO: Project points to fitted plane before getting scale?
	// Calculate world size between points.
	// 0 - 1 - 2, 3 - 4 - 5, 6 - 7 - 8
	// 0 - 3 - 6, 1 - 4 - 7, 2 - 5 - 8

	double rms = 0.0f;
	double mean = 0.0f;
	int meanCount = 0;

	for (int boardIter = 0; boardIter < 5; ++boardIter) {
		for (int iY = 0; iY < Cube->boardPointXCount; ++iY) {
			for (int iX = 1; iX < Cube->boardPointXCount; ++iX) {
				{
					ldiCalibCubePoint* point0 = &Cube->points[boardIter * Cube->boardPointTotalCount + iY * Cube->boardPointXCount + (iX - 1)];
					ldiCalibCubePoint* point1 = &Cube->points[boardIter * Cube->boardPointTotalCount + iY * Cube->boardPointXCount + iX];
					float dist = glm::distance(point0->position, point1->position);
					double error = dist - Cube->boardSquareSizeCm;
					rms += error * error;
					mean += dist;
					++meanCount;

					std::cout << "(" << boardIter << ") " << iX - 1 << "," << iY << " to " << iX << "," << iY << " :: " << point0->id << " to " << point1->id << " " << dist << "\n";
				}

				{
					ldiCalibCubePoint* point0 = &Cube->points[boardIter * Cube->boardPointTotalCount + (iX - 1) * Cube->boardPointXCount + iY];
					ldiCalibCubePoint* point1 = &Cube->points[boardIter * Cube->boardPointTotalCount + iX * Cube->boardPointXCount + iY];
					float dist = glm::distance(point0->position, point1->position);
					double error = dist - Cube->boardSquareSizeCm;
					rms += error * error;
					mean += dist;
					++meanCount;

					std::cout << "(" << boardIter << ") " << iY << "," << (iX - 1) << " to " << iY << "," << iX << " :: " << point0->id << " to " << point1->id << " " << dist << "\n";
				}
			}
		}
	}

	rms = sqrt(rms / meanCount);
	mean /= meanCount;

	std::cout << "RMS: " << rms << " Mean: " << mean << "\n";

	//cv::estimateAffine3D(
}

void cameraCalibCreateTarget(int CornerCountX, int CornerCountY, float CellSize, int CellPixelSize, bool OutputImage = false) {
	// NOTE: Border is half cellSize.
	int imageWidth = (CornerCountX + 1) * CellPixelSize + CellPixelSize;
	int imageHeight = (CornerCountY + 1) * CellPixelSize + CellPixelSize;

	uint8_t *imageData = new uint8_t[imageWidth * imageHeight];
	memset(imageData, 255, imageWidth * imageHeight);

	for (int iY = 0; iY <= CornerCountY; ++iY) {
		for (int iX = 0; iX <= CornerCountX; ++iX) {
			int solidity = (iX + iY * (CornerCountX)) % 2;

			if (solidity == 0) {
				int x = CellPixelSize * 0.5f + iX * CellPixelSize;
				int y = CellPixelSize * 0.5f + iY * CellPixelSize;

				for (int sY = 0; sY < CellPixelSize; ++sY) {
					for (int sX = 0; sX < CellPixelSize; ++sX) {
						int pY = y + sY;
						int pX = x + sX;
						imageData[pX + pY * imageWidth] = 0;
					}
				}
			}
		}
	}

	if (OutputImage) {
		imageWrite("../cache/calib_target.png", imageWidth, imageHeight, 1, imageWidth, imageData);
	}

	_calibrationTargetImage.data = imageData;
	_calibrationTargetImage.width = imageWidth;
	_calibrationTargetImage.height = imageHeight;

	_calibrationTargetFullWidthCm = (CornerCountX + 2) * CellSize;
	_calibrationTargetFullHeightCm = (CornerCountY + 2) * CellSize;

	for (int iY = 0; iY < CornerCountY; ++iY) {
		for (int iX = 0; iX < CornerCountX; ++iX) {
			_calibrationTargetLocalPoints.push_back(vec3(iX * CellSize, 0.0f, iY * CellSize));
		}
	}
}

void cameraCalibProcessImage() {
	//cv::findChessboardCorners();
	//cv::estimateChessboardSharpness();
	//cv::find4QuadCornerSubpix(); Needed with findChessboardCorners()?
	//cv::drawChessboardCorners();
}

bool cameraCalibFindChessboard(ldiApp* AppContext, ldiImage Image, ldiCameraCalibSample* Sample) {
	int offset = 1;

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<cv::Point2f> corners;

		double t0 = _getTime(AppContext);
		bool foundCorners = cv::findChessboardCorners(image, cv::Size(9, 6), corners);
		t0 = _getTime(AppContext) - t0;
		std::cout << "Detect chessboard: " << (t0 * 1000.0) << " ms\n";

		if (!foundCorners) {
			return false;
		}

		// TODO: Find better window sizes.
		cv::cornerSubPix(image, corners, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON));

		for (size_t i = 0; i < corners.size(); ++i) {
			//std::cout << corners[i].x << ", " << corners[i].y << "\n";
			Sample->imagePoints.push_back(vec2(corners[i].x, corners[i].y));
		}
	} catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}

	return true;
}

bool cameraCalibFindCirclesBoard(ldiApp* AppContext, ldiImage Image, ldiCameraCalibSample* Sample) {
	int offset = 1;

	Sample->imagePoints.clear();

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<cv::Point2f> corners;

		cv::SimpleBlobDetector::Params params = cv::SimpleBlobDetector::Params();
		std::cout << params.minArea << " " << params.maxArea << "\n";
		params.minArea = 70;
		params.maxArea = 20000;

		cv::Ptr<cv::SimpleBlobDetector> blobDetector = cv::SimpleBlobDetector::create(params);

		double t0 = _getTime(AppContext);
		bool foundCorners = cv::findCirclesGrid(image, cv::Size(4, 11), corners, cv::CALIB_CB_ASYMMETRIC_GRID, blobDetector);
		t0 = _getTime(AppContext) - t0;
		std::cout << "Detect circles: " << (t0 * 1000.0) << " ms " << foundCorners << "\n";

		if (!foundCorners) {
			return false;
		}

		for (size_t i = 0; i < corners.size(); ++i) {
			//std::cout << corners[i].x << ", " << corners[i].y << "\n";
			Sample->imagePoints.push_back(vec2(corners[i].x, corners[i].y));
		}
	} catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}

	return true;
}

void cameraCalibRunCalibration(ldiApp* AppContext, std::vector<ldiCameraCalibSample>& Samples, int ImageWidth, int ImageHeight) {
	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> imagePoints;

	std::vector<cv::Point3f> objectPointsTemp;
	for (size_t i = 0; i < _calibrationTargetLocalPoints.size(); ++i) {
		objectPointsTemp.push_back(cv::Point3f(_calibrationTargetLocalPoints[i].x, _calibrationTargetLocalPoints[i].y, _calibrationTargetLocalPoints[i].z));
	}

	for (size_t sampleIter = 0; sampleIter < Samples.size(); ++sampleIter) {
		std::vector<cv::Point2f> imagePointsTemp;

		std::vector<vec2>* samplePoints = &Samples[sampleIter].imagePoints;
		for (size_t i = 0; i < samplePoints->size(); ++i) {
			imagePointsTemp.push_back(cv::Point2f((*samplePoints)[i].x, (*samplePoints)[i].y));
		}

		objectPoints.push_back(objectPointsTemp);
		imagePoints.push_back(imagePointsTemp);
	}

	// NOTE: 1600x1300 with 42deg vertical FOV.
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1693.30789;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 800;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1693.30789;
	cameraMatrix.at<double>(1, 2) = 650;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	cv::Mat stdDevIntrinsics;
	cv::Mat stdDevExtrinsics;
	cv::Mat perViewErrors;

	double t0 = _getTime(AppContext);
	// cv::CALIB_USE_INTRINSIC_GUESS
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

	double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, 0, criteria);
	t0 = _getTime(AppContext) - t0;
	std::cout << t0 * 1000.0f << " ms\n";

	std::cout << stdDevIntrinsics.size << "\n";
	std::cout << stdDevExtrinsics.size << "\n";
	std::cout << perViewErrors.size << "\n";
	std::cout << stdDevIntrinsics << "\n\n";
	std::cout << stdDevExtrinsics << "\n\n";
	std::cout << perViewErrors << "\n\n";

	std::cout << "Calibration error: " << rms << "\n";

	std::cout << "Camera matrix: " << cameraMatrix << "\n";
	std::cout << "Dist coefs: " << distCoeffs << "\n";

	for (int i = 0; i < tvecs.size(); ++i) {
		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rvecs[i], cvRotMat);

		//std::cout << "TVec: " << tvecs[i] << "\n";
		//std::cout << "cvRotMat: " << cvRotMat << "\n";

		// NOTE: OpenCV coords are flipped on Y and Z relative to our cam transform.

		mat4 rotMat(1.0f);
		rotMat[0][0] = cvRotMat.at<double>(0, 0);
		rotMat[0][1] = -cvRotMat.at<double>(1, 0);
		rotMat[0][2] = -cvRotMat.at<double>(2, 0);

		rotMat[1][0] = cvRotMat.at<double>(0, 1);
		rotMat[1][1] = -cvRotMat.at<double>(1, 1);
		rotMat[1][2] = -cvRotMat.at<double>(2, 1);

		rotMat[2][0] = cvRotMat.at<double>(0, 2);
		rotMat[2][1] = -cvRotMat.at<double>(1, 2);
		rotMat[2][2] = -cvRotMat.at<double>(2, 2);

		mat4 transMat = glm::translate(mat4(1.0f), vec3(tvecs[i].at<double>(0), -tvecs[i].at<double>(1), -tvecs[i].at<double>(2)));
		Samples[i].camLocalMat = transMat * rotMat;
	}
}

void cameraCalibRunCalibrationCircles(ldiApp* AppContext, std::vector<ldiCameraCalibSample>& Samples, int ImageWidth, int ImageHeight, cv::Mat* CameraMatrix, cv::Mat* CameraDist) {
	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> imagePoints;

	/*std::vector<cv::Point3f> objectPointsTemp;
	for (size_t i = 0; i < _calibrationTargetLocalPoints.size(); ++i) {
		objectPointsTemp.push_back(cv::Point3f(_calibrationTargetLocalPoints[i].x, _calibrationTargetLocalPoints[i].y, _calibrationTargetLocalPoints[i].z));
	}*/

	float squareSize = 1.5;
	std::vector<cv::Point3f> objectPointsTemp;
	for (int i = 0; i < 11; i++) {
		for (int j = 0; j < 4; j++) {
			objectPointsTemp.push_back(cv::Point3f((2 * j + i % 2) * squareSize, i * squareSize, 0));
		}
	}

	std::cout << "Calibrating camera with " << Samples.size() << " samples.\n";

	for (size_t sampleIter = 0; sampleIter < Samples.size(); ++sampleIter) {
		std::vector<cv::Point2f> imagePointsTemp;

		std::vector<vec2>* samplePoints = &Samples[sampleIter].imagePoints;
		for (size_t i = 0; i < samplePoints->size(); ++i) {
			imagePointsTemp.push_back(cv::Point2f((*samplePoints)[i].x, (*samplePoints)[i].y));
		}

		objectPoints.push_back(objectPointsTemp);
		imagePoints.push_back(imagePointsTemp);
	}

	/*
	Calibration error : 2.91036
	Camera matrix : [2863.329432212108, 0, 1438.568488985049;
					0, 2842.653495273561, 1269.90720871305;
					0, 0, 1]
	Dist coefs : [0.2058971840172417;
				-0.3454802257828885;
				0.0006148403759009931;
				-0.02636839714078372;
				0.2207806178260863]

	Calibration error: 5.13806
	Camera matrix: [3028.042700481885, 0, 1595.244363036006;
					0, 3022.336716826929, 1110.067203702069;
					0, 0, 1]
	Dist coefs: [0.3706319835851833;
				 -1.100170196545013;
				 -0.01871208309159954;
				 -0.007059212403853174;
				 1.109365459830671]

	Calibration error: 3.93091
Camera matrix: [3114.35830267407, 0, 1247.315798137534;
 0, 3070.139064342456, 1092.434452796077;
 0, 0, 1]
Dist coefs: [0.2314308745264201;
 -0.2385008053719591;
 -0.008067924218661829;
 -0.0578993718936298;
 0.03453225870608038]

	 Calibration error: 1.39292
	Camera matrix: [2593.351333334043, 0, 1658.710562585348;
	 0, 2590.751756763483, 1253.560922844007;
	 0, 0, 1]
	Dist coefs: [0.2259068314218333;
	 -0.5624260956431792;
	 -0.003177729613407228;
	 -0.0005562032199261654;
	 0.3713010480709695]

	 Calibration error: 0.991625
Camera matrix: [2581.728739379068, 0, 1687.942480875822;
 0, 2598.671908051138, 1242.538168269621;
 0, 0, 1]
Dist coefs: [0.2349581871368413;
 -0.6549985792513259;
 -0.002393947805394209;
 0.002195147154093995;
 0.5387109848236522]
	*/

	// NOTE: 1600x1300 with 42deg vertical FOV.
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1693.30789;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 800;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1693.30789;
	cameraMatrix.at<double>(1, 2) = 650;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	cv::Mat stdDevIntrinsics;
	cv::Mat stdDevExtrinsics;
	cv::Mat perViewErrors;

	double t0 = _getTime(AppContext);

	//cv::CALIB_USE_INTRINSIC_GUESS
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 1000, DBL_EPSILON);
	//double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, 0, criteria);
	double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_FIX_K3, criteria);
	//double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS);
	t0 = _getTime(AppContext) - t0;
	std::cout << t0 * 1000.0f << " ms\n";

	std::cout << stdDevIntrinsics.size << "\n";
	std::cout << stdDevExtrinsics.size << "\n";
	std::cout << perViewErrors.size << "\n";
	std::cout << stdDevIntrinsics << "\n\n";
	std::cout << stdDevExtrinsics << "\n\n";
	std::cout << perViewErrors << "\n\n";

	std::cout << "Calibration error: " << rms << "\n";

	std::cout << "Camera matrix: " << cameraMatrix << "\n";
	std::cout << "Dist coefs: " << distCoeffs << "\n";

	*CameraMatrix = cameraMatrix;
	*CameraDist = distCoeffs;

	for (int i = 0; i < tvecs.size(); ++i) {
		//cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		//cv::Rodrigues(rvecs[i], cvRotMat);

		////std::cout << "TVec: " << tvecs[i] << "\n";
		////std::cout << "cvRotMat: " << cvRotMat << "\n";

		//// NOTE: OpenCV coords are flipped on Y and Z relative to our cam transform.

		//mat4 rotMat(1.0f);
		//rotMat[0][0] = cvRotMat.at<double>(0, 0);
		//rotMat[0][1] = -cvRotMat.at<double>(1, 0);
		//rotMat[0][2] = -cvRotMat.at<double>(2, 0);

		//rotMat[1][0] = cvRotMat.at<double>(0, 1);
		//rotMat[1][1] = -cvRotMat.at<double>(1, 1);
		//rotMat[1][2] = -cvRotMat.at<double>(2, 1);

		//rotMat[2][0] = cvRotMat.at<double>(0, 2);
		//rotMat[2][1] = -cvRotMat.at<double>(1, 2);
		//rotMat[2][2] = -cvRotMat.at<double>(2, 2);

		//mat4 transMat = glm::translate(mat4(1.0f), vec3(tvecs[i].at<double>(0), -tvecs[i].at<double>(1), -tvecs[i].at<double>(2)));
		//Samples[i].camLocalMat = transMat * rotMat;

		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rvecs[i], cvRotMat);

		mat4 rotMat(1.0f);
		rotMat[0][0] = cvRotMat.at<double>(0, 0);
		rotMat[0][1] = cvRotMat.at<double>(1, 0);
		rotMat[0][2] = cvRotMat.at<double>(2, 0);

		rotMat[1][0] = cvRotMat.at<double>(0, 1);
		rotMat[1][1] = cvRotMat.at<double>(1, 1);
		rotMat[1][2] = cvRotMat.at<double>(2, 1);

		rotMat[2][0] = cvRotMat.at<double>(0, 2);
		rotMat[2][1] = cvRotMat.at<double>(1, 2);
		rotMat[2][2] = cvRotMat.at<double>(2, 2);

		// NOTE: This builds the camera RT matrix.
		mat4 transMat = glm::translate(mat4(1.0f), vec3(tvecs[i].at<double>(0), tvecs[i].at<double>(1), tvecs[i].at<double>(2)));
		Samples[i].camLocalMat = transMat * rotMat;
		//*Pose = transMat * rotMat;

		Samples[i].tvec = tvecs[i];
		Samples[i].rvec = rvecs[i];
		Samples[i].worldPoints = objectPointsTemp;

		cv::Mat empty;
		cv::projectPoints(Samples[i].worldPoints, Samples[i].rvec, Samples[i].tvec, cameraMatrix, empty, Samples[i].undistortedImagePoints);
		cv::projectPoints(Samples[i].worldPoints, Samples[i].rvec, Samples[i].tvec, cameraMatrix, distCoeffs, Samples[i].reprojectedImagePoints);

		//std::cout << "Converting rvecs and tvecs\n";
		//std::cout << "R: " << rvecs[i] << "\n";
		//std::cout << "T: " << tvecs[i] << "\n";
		//std::cout << "cvRotMat: " << cvRotMat << "\n";
		//std::cout << "rotMat: " << GetMat4DebugString(&rotMat);
		//std::cout << "camLocalMat: " << GetMat4DebugString(&Samples[i].camLocalMat);

		//// Convert cam mat to rvec, tvec.
		//cv::Mat reTvec = cv::Mat::zeros(3, 1, CV_64F);
		//cv::Mat reRotMat = cv::Mat::zeros(3, 3, CV_64F);
		//reTvec.at<double>(0) = Samples[i].camLocalMat[3][0];
		//reTvec.at<double>(1) = Samples[i].camLocalMat[3][1];
		//reTvec.at<double>(2) = Samples[i].camLocalMat[3][2];
		//std::cout << "Re T: " << reTvec << "\n";

		//reRotMat.at<double>(0, 0) = Samples[i].camLocalMat[0][0];
		//reRotMat.at<double>(1, 0) = Samples[i].camLocalMat[0][1];
		//reRotMat.at<double>(2, 0) = Samples[i].camLocalMat[0][2];
		//reRotMat.at<double>(0, 1) = Samples[i].camLocalMat[1][0];
		//reRotMat.at<double>(1, 1) = Samples[i].camLocalMat[1][1];
		//reRotMat.at<double>(2, 1) = Samples[i].camLocalMat[1][2];
		//reRotMat.at<double>(0, 2) = Samples[i].camLocalMat[2][0];
		//reRotMat.at<double>(1, 2) = Samples[i].camLocalMat[2][1];
		//reRotMat.at<double>(2, 2) = Samples[i].camLocalMat[2][2];

		//std::cout << "ReRotMat: " << reRotMat << "\n";

		//cv::Mat reRvec = cv::Mat::zeros(3, 1, CV_64F);
		//cv::Rodrigues(reRotMat, reRvec);
		//std::cout << "Re R: " << reRvec << "\n";
	}
}

void createCharucos(bool Output) {
	try {
		for (int i = 0; i < 6; ++i) {
			//	cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.009f, 0.006f, _dictionary);
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.9f, 0.6f, _dictionary);
			
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 0.9f, 0.6f, _dictionary);
			//double newScale = 5.0 / 6.0;
			double newScale = 1.0f;
			// NOTE: Ideal board?
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(6, 6, 1.0 * newScale, 0.7 * newScale, _dictionary);

			// NOTE: Original board?
			// NOTE: Original printedd boards are ~9mm per square.
			cv::Ptr<cv::aruco::CharucoBoard> board = new cv::aruco::CharucoBoard(cv::Size(4, 4), 1.0 * newScale, 0.7 * newScale, _dictionary);
			board->setLegacyPattern(true);
			//cv::aruco::CharucoBoard board = cv::aruco::CharucoBoard(cv::Size(4, 4), 1.0 * newScale, 0.7 * newScale, _dictRaw);

			/*cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 1.0 * newScale, 0.7 * newScale, _dictionary);*/
			//CV_WRAP CharucoBoard(const Size & size, float squareLength, float markerLength,
				//const Dictionary & dictionary, InputArray ids = noArray());
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(6, 6, 1.0 * newScale, 0.8 * newScale, _dictionary);
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(5, 5, 1.0 * newScale, 0.65 * newScale, _dictionary);
			//_charucoBoardsRaw.push_back(board);
			/*_charucoBoards.push_back(cv::Ptr<cv::aruco::CharucoBoard>(&_charucoBoardsRaw[_charucoBoardsRaw.size() - 1]));*/
			_charucoBoards.push_back(board);

			// NOTE: Shift IDs for each board.
			int offset = i * board->getIds().size();

			for (int j = 0; j < board->getIds().size(); ++j) {
				((std::vector<int>&)board->getIds())[j] = offset + j;
			}

			// NOTE: Image output.
			if (Output) {
				cv::Mat markerImage;
				board->generateImage(cv::Size(1000, 1000), markerImage, 50, 1);

				char fileName[512];
				sprintf_s(fileName, "../cache/charuco_small_%d.png", i);
				cv::imwrite(fileName, markerImage);
			}
		}
	}
	catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}

void findCharuco(ldiImage Image, ldiApp* AppContext, ldiCharucoResults* Results, cv::Mat* CameraMatrix, cv::Mat* CameraDist) {
	int offset = 1;

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
		auto parameters = cv::aruco::DetectorParameters();
		parameters.minMarkerPerimeterRate = 0.015;
		parameters.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		//parameters.useAruco3Detection = true;
		//cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::Ptr<cv::aruco::DetectorParameters>(&rawParams);
		// TODO: Enable subpixel corner refinement. WTF is this doing?
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		//cv::aruco::RefineParameters();

		double t0 = _getTime(AppContext);
		//cv::aruco::detectMarkers(image, _dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

		cv::aruco::ArucoDetector arucoDetector(_dictionary, parameters);
		arucoDetector.detectMarkers(image, markerCorners, markerIds, rejectedCandidates);
		
		t0 = _getTime(AppContext) - t0;
		std::cout << "Detect markers: " << (t0 * 1000.0) << " ms\n";
		
		// Use refine strategy to detect more markers.
		//cv::Ptr<cv::aruco::Board> board = charucoBoard.staticCast<aruco::Board>();
		//aruco::refineDetectedMarkers(image, board, markerCorners, markerIds, rejectedCandidates);

		Results->markers.clear();
		Results->boards.clear();

		for (int i = 0; i < markerCorners.size(); ++i) {
			ldiCharucoMarker marker;
			marker.id = markerIds[i];

			for (int j = 0; j < 4; ++j) {
				marker.corners[j] = vec2(markerCorners[i][j].x, markerCorners[i][j].y);
			}

			Results->markers.push_back(marker);
		}

		// Interpolate charuco corners for each possible board.
		if (markerCorners.size() > 0) {
			//std::cout << "Markers: " << markerCorners.size() << "\n";

			for (int i = 0; i < 6; ++i) {
				std::vector<cv::Point2f> charucoCorners;
				std::vector<int> charucoIds;

				//cv::Ptr<cv::aruco::Board> ptrBoard;
				cv::aruco::CharucoDetector charucoDetector(*(_charucoBoards[i]).get(), cv::aruco::CharucoParameters(), parameters);
				//cv::aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners, charucoIds);
				charucoDetector.detectBoard(image, charucoCorners, charucoIds, markerCorners, markerIds);

				//std::cout << "    " << i << " Charucos: " << charucoIds.size() << "\n";

				// Board is valid if it has at least one corner.
				if (charucoIds.size() > 0) {
					ldiCharucoBoard board = {};
					board.id = i;
					board.localMat = false;
					board.rejected = true;

					cv::Mat rvec;
					cv::Mat tvec;

					std::cout << "Check board pose:\n";
					if (cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, _charucoBoards[i], *CameraMatrix, *CameraDist, rvec, tvec)) {
						std::cout << "Found board pose: " << i << rvec << "\n";
						board.rejected = false;

						cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
						cv::Rodrigues(rvec, cvRotMat);

						// NOTE: OpenCV coords are flipped on Y and Z relative to our cam transform.
						mat4 rotMat(1.0f);
						rotMat[0][0] = cvRotMat.at<double>(0, 0);
						rotMat[0][1] = -cvRotMat.at<double>(1, 0);
						rotMat[0][2] = -cvRotMat.at<double>(2, 0);

						rotMat[1][0] = cvRotMat.at<double>(0, 1);
						rotMat[1][1] = -cvRotMat.at<double>(1, 1);
						rotMat[1][2] = -cvRotMat.at<double>(2, 1);

						rotMat[2][0] = cvRotMat.at<double>(0, 2);
						rotMat[2][1] = -cvRotMat.at<double>(1, 2);
						rotMat[2][2] = -cvRotMat.at<double>(2, 2);

						mat4 transMat = glm::translate(mat4(1.0f), vec3(tvec.at<double>(0), -tvec.at<double>(1), -tvec.at<double>(2)));
						board.camLocalCharucoEstimatedMat = transMat * rotMat;

						cv::Mat rvecTemp = cv::Mat::zeros(3, 1, CV_64F);
						cv::Mat tvecTemp = cv::Mat::zeros(3, 1, CV_64F);
						std::vector<cv::Point3f> points;
						std::vector<cv::Point2f> imagePoints;
						float cubeHSize = 2.0f;

						vec3 centerPoint = board.camLocalCharucoEstimatedMat * vec4(cubeHSize, cubeHSize, 0.0f, 1.0f);
						points.push_back(cv::Point3f(-centerPoint.x, centerPoint.y, centerPoint.z));

						vec3 centerNormalPoint = board.camLocalCharucoEstimatedMat * vec4(cubeHSize, cubeHSize, 1.0f, 1.0f);
						points.push_back(cv::Point3f(-centerNormalPoint.x, centerNormalPoint.y, centerNormalPoint.z));

						vec3 centerNormal = glm::normalize(centerNormalPoint - centerPoint);
						vec3 camDir = glm::normalize(centerPoint);
						board.charucoEstimatedBoardAngle = glm::dot(camDir, centerNormal);

						std::cout << "Dot: " << board.charucoEstimatedBoardAngle << "\n";

						if (board.charucoEstimatedBoardAngle < 0.3f) {
							board.rejected = true;
						}

						cv::projectPoints(points, rvecTemp, tvecTemp, *CameraMatrix, *CameraDist, imagePoints);
						
						//points.push_back(cv::Point3f(cubeHSize, cubeHSize, 0.0f));
						//cv::projectPoints(points, rvec, tvec, *CameraMatrix, *CameraDist, imagePoints);

						board.charucoEstimatedImageCenter = vec2(imagePoints[0].x, imagePoints[0].y);
						board.charucoEstimatedImageNormal = vec2(imagePoints[1].x, imagePoints[1].y);
					} else {
						std::cout << "Invalid\n";
					}

					for (int j = 0; j < charucoIds.size(); ++j) {
						ldiCharucoCorner corner;
						corner.id = charucoIds[j];
						corner.globalId = board.id * 9 + corner.id;
						corner.position = vec2(charucoCorners[j].x, charucoCorners[j].y);
						board.corners.push_back(corner);
					}

					if (!board.rejected) {
						Results->boards.push_back(board);
					} else {
						std::cout << "Board rejected\n";
					}
				}
			}
		}
		
		//for (int i = 0; i < boardCount; ++i) {
		//	int markerCount = charucoIds[i].size();
		//	//std::cout << "Board " << i << " markers: " << markerCount << "\n";

		//	for (int j = 0; j < markerCount; ++j) {
		//		int cornerId = charucoIds[i][j];
		//		Results->charucoCorners.push_back(vec2(charucoCorners[i][j].x, charucoCorners[i][j].y));
		//		Results->charucoIds.push_back(cornerId);
		//		//std::cout << cornerId << ": " << charucoCorners[i][j].x << ", " << charucoCorners[i][j].y << "\n";
		//	}
		//}

		/*t0 = _getTime(AppContext) - t0;
		std::cout << "Find charuco: " << (t0 * 1000.0) << " ms\n";*/

	} catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}

void computerVisionCalibrateCameraCharuco(ldiApp* AppContext, std::vector<ldiCalibSample>* Samples, int ImageWidth, int ImageHeight, cv::Mat* CameraMatrix, cv::Mat* CameraDist) {
	std::vector<std::vector<int>> charucoIds;
	std::vector<std::vector<cv::Point2f>> charucoCorners;
	
	struct ldiSampleTracker {
		int sampleId;
		int boardId;
	};

	std::vector<ldiSampleTracker> boardMap;

	for (size_t sampleIter = 0; sampleIter < Samples->size(); ++sampleIter) {
		for (size_t boardIter = 0; boardIter < (*Samples)[sampleIter].charucos.boards.size(); ++boardIter) {
			if ((*Samples)[sampleIter].charucos.boards[boardIter].corners.size() < 6) {
				continue;
			}

			std::vector<int> cornerIds;
			std::vector<cv::Point2f> cornerPositions;

			for (size_t cornerIter = 0; cornerIter < (*Samples)[sampleIter].charucos.boards[boardIter].corners.size(); ++cornerIter) {
				ldiCharucoCorner corner = (*Samples)[sampleIter].charucos.boards[boardIter].corners[cornerIter];

				cornerIds.push_back(corner.id);
				cornerPositions.push_back(cv::Point2f(corner.position.x, corner.position.y));
				//cornerPositions.push_back(cv::Point2f(corner.position.x, corner.position.y));

				//std::cout << sampleIter << " " << boardIter << " " << corner.id << " " << corner.position.x << ", " << corner.position.y << "\n";
			}

			charucoIds.push_back(cornerIds);
			charucoCorners.push_back(cornerPositions);

			ldiSampleTracker tracker = { sampleIter, boardIter };
			boardMap.push_back(tracker);
		}

		/*if (charucoIds.size() > 40) {
			break;
		}*/
	}

	std::cout << "Calib boards: " << charucoIds.size() << "\n";

	// NOTE: 1600x1300 with 42deg vertical FOV.
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1693.30789;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 800;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1693.30789;
	cameraMatrix.at<double>(1, 2) = 650;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	cv::Mat stdDevIntrinsics;
	cv::Mat stdDevExtrinsics;
	cv::Mat perViewErrors;

	double t0 = _getTime(AppContext);
	double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6 | cv::CALIB_ZERO_TANGENT_DIST | cv::CALIB_FIX_FOCAL_LENGTH | cv::CALIB_USE_INTRINSIC_GUESS | cv::CALIB_FIX_PRINCIPAL_POINT);
	//double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS,
		//cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON));
	t0 = _getTime(AppContext) - t0;
	std::cout << t0 * 1000.0f << " ms\n";

	std::cout << stdDevIntrinsics.size << "\n";
	std::cout << stdDevExtrinsics.size << "\n";
	std::cout << perViewErrors.size << "\n";
	std::cout << stdDevIntrinsics << "\n\n";
	std::cout << perViewErrors << "\n\n";

	std::cout << "Calibration error: " << rms << "\n";
	//std::cout << "Camera matrix: " << cameraMatrix << "\n";
	//std::cout << "Dist coeffs: " << distCoeffs << "\n";

	*CameraMatrix = cameraMatrix;
	*CameraDist = distCoeffs;

	for (int iX = 0; iX <= 2; ++iX) {
		for (int iY = 0; iY <= 2; ++iY) {
			std::cout << "cameraMatrix.at<double>(" << iX << ", " << iY << ") = " << cameraMatrix.at<double>(iX, iY) << ";\n";
		}
	}

	for (int i = 0; i < 5; ++i) {
		std::cout << "distCoeffs.at<double>(" << i << ") = " << distCoeffs.at<double>(i) << ";\n";
	}

	for (int i = 0; i < tvecs.size(); ++i) {
		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rvecs[i], cvRotMat);

		//std::cout << "TVec: " << tvecs[i] << "\n";
		//std::cout << "cvRotMat: " << cvRotMat << "\n";
		
		// NOTE: OpenCV coords are flipped on Y and Z relative to our cam transform.

		mat4 rotMat(1.0f);
		rotMat[0][0] = cvRotMat.at<double>(0, 0);
		rotMat[0][1] = -cvRotMat.at<double>(1, 0);
		rotMat[0][2] = -cvRotMat.at<double>(2, 0);

		rotMat[1][0] = cvRotMat.at<double>(0, 1);
		rotMat[1][1] = -cvRotMat.at<double>(1, 1);
		rotMat[1][2] = -cvRotMat.at<double>(2, 1);

		rotMat[2][0] = cvRotMat.at<double>(0, 2);
		rotMat[2][1] = -cvRotMat.at<double>(1, 2);
		rotMat[2][2] = -cvRotMat.at<double>(2, 2);

		mat4 transMat = glm::translate(mat4(1.0f), vec3(tvecs[i].at<double>(0), -tvecs[i].at<double>(1), -tvecs[i].at<double>(2)));
		(*Samples)[boardMap[i].sampleId].charucos.boards[boardMap[i].boardId].camLocalMat = transMat * rotMat;
		(*Samples)[boardMap[i].sampleId].charucos.boards[boardMap[i].boardId].localMat = true;
	}
}

bool computerVisionFindGeneralPose(cv::Mat* CameraMatrix, cv::Mat* DistCoeffs, std::vector<cv::Point2f>* ImagePoints, std::vector<cv::Point3f>* WorldPoints, mat4* Pose) {
	//Mat rvec;
	//Mat tvec;
	//bool found = aruco::estimatePoseCharucoBoard(markerPos, markerIds, _charucoBoards[0], cameraMatrix, distCoeffs, rvec, tvec);

	// Calculate pose
	//std::vector<cv::Point2d> imagePoints;
	//bool found = solvePnP(worldPoints, markerPos, cameraMatrix, distCoeffs, rvec, tvec);

	//std::cout << "Find pose\n";

	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	std::vector<float> rms;

	int solutionCount = cv::solvePnPGeneric(*WorldPoints, *ImagePoints, *CameraMatrix, *DistCoeffs, rvecs, tvecs, false, cv::SOLVEPNP_ITERATIVE, cv::noArray(), cv::noArray(), rms);
	//std::cout << "  Solutions: " << solutionCount << "\n";
	// TODO: Why do we set solutionCount to this?
	solutionCount = rvecs.size();
	//std::cout << "  Solutions: " << solutionCount << "\n";

	for (int i = 0; i < solutionCount; ++i) {
		// TODO: How does this compare with the reprojection error from the solvePnPGeneric function?
		//std::vector<cv::Point2f> projectedImagePoints;
		//projectPoints(*WorldPoints, rvecs[i], tvecs[i], *CameraMatrix, *DistCoeffs, projectedImagePoints);

		cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
		cv::Rodrigues(rvecs[i], cvRotMat);

		mat4 rotMat(1.0f);
		rotMat[0][0] = cvRotMat.at<double>(0, 0);
		rotMat[0][1] = cvRotMat.at<double>(1, 0);
		rotMat[0][2] = cvRotMat.at<double>(2, 0);

		rotMat[1][0] = cvRotMat.at<double>(0, 1);
		rotMat[1][1] = cvRotMat.at<double>(1, 1);
		rotMat[1][2] = cvRotMat.at<double>(2, 1);

		rotMat[2][0] = cvRotMat.at<double>(0, 2);
		rotMat[2][1] = cvRotMat.at<double>(1, 2);
		rotMat[2][2] = cvRotMat.at<double>(2, 2);

		// NOTE: This builds the camera RT matrix.
		mat4 transMat = glm::translate(mat4(1.0f), vec3(tvecs[i].at<double>(0), tvecs[i].at<double>(1), tvecs[i].at<double>(2)));

		*Pose = transMat * rotMat;

		std::cout << "  Found pose (" << i << "): " << rms[i] << "\n";

		if (rms[i] > 3.0) {
			return false;
		}
	}

	if (solutionCount > 0) {
		return true;
	}

	return false;
}

bool computerVisionFindGeneralPoseRT(cv::Mat* CameraMatrix, cv::Mat* DistCoeffs, std::vector<cv::Point2f>* ImagePoints, std::vector<cv::Point3f>* WorldPoints, cv::Mat* RVec, cv::Mat* TVec) {
	//Mat rvec;
	//Mat tvec;
	//bool found = aruco::estimatePoseCharucoBoard(markerPos, markerIds, _charucoBoards[0], cameraMatrix, distCoeffs, rvec, tvec);

	// Calculate pose
	//std::vector<cv::Point2d> imagePoints;
	//bool found = solvePnP(worldPoints, markerPos, cameraMatrix, distCoeffs, rvec, tvec);

	//std::cout << "Find pose\n";

	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	std::vector<float> rms;

	int solutionCount = cv::solvePnPGeneric(*WorldPoints, *ImagePoints, *CameraMatrix, *DistCoeffs, rvecs, tvecs, false, cv::SOLVEPNP_ITERATIVE, cv::noArray(), cv::noArray(), rms);
	//std::cout << "  Solutions: " << solutionCount << "\n";
	// TODO: Why do we set solutionCount to this?
	solutionCount = rvecs.size();
	//std::cout << "  Solutions: " << solutionCount << "\n";

	for (int i = 0; i < solutionCount; ++i) {
		*RVec = rvecs[i];
		*TVec = tvecs[i];

		std::cout << "  Found pose (" << i << "): " << rms[i] << "\n";

		if (rms[i] > 3.0) {
			return false;
		}
	}

	if (solutionCount > 0) {
		return true;
	}

	return false;
}

ldiLineFit computerVisionFitLine(std::vector<vec3>& Points) {
	// TODO: Double check that the centroid is actually a valid point on the line.

	std::vector<cv::Point3f> points;

	for (size_t i = 0; i < Points.size(); ++i) {
		points.push_back(cv::Point3f(Points[i].x, Points[i].y, Points[i].z));
	}

	int pointCount = (int)Points.size();
	//std::cout << "Find line fit " << pointCount << "\n" << std::flush;

	cv::Point3f centroid(0, 0, 0);

	for (int i = 0; i < pointCount; ++i) {
		centroid += points[i];
	}

	centroid /= pointCount;
	//std::cout << "Centroid: " << centroid.x << ", " << centroid.y << ", " << centroid.z << "\n" << std::flush;

	cv::Mat mP = cv::Mat(pointCount, 3, CV_32F);

	for (int i = 0; i < pointCount; ++i) {
		points[i] -= centroid;
		mP.at<float>(i, 0) = points[i].x;
		mP.at<float>(i, 1) = points[i].y;
		mP.at<float>(i, 2) = points[i].z;
	}

	cv::Mat w, u, vt;
	cv::SVD::compute(mP, w, u, vt);

	//SVD::compute(mP, w);
	//cv::Point3f pW(w.at<float>(0, 0), w.at<float>(0, 1), w.at<float>(0, 2));
	//cv::Point3f pU(u.at<float>(0, 0), u.at<float>(0, 1), u.at<float>(0, 2));
	cv::Point3f pV(vt.at<float>(0, 0), vt.at<float>(0, 1), vt.at<float>(0, 2));

	/*std::cout << "W: " << pW.x << ", " << pW.y << ", " << pW.z << "\n" << std::flush;
	std::cout << "U: " << pU.x << ", " << pU.y << ", " << pU.z << "\n" << std::flush;
	std::cout << "V: " << pV.x << ", " << pV.y << ", " << pV.z << "\n" << std::flush;*/

	ldiLineFit result;
	result.origin = vec3(centroid.x, centroid.y, centroid.z);
	result.direction = glm::normalize(vec3(pV.x, pV.y, pV.z));

	return result;
}

// Nghia Ho: https://nghiaho.com/?page_id=671
// Get transformation between corresponding 3D point sets.
void computerVisionFitPoints(std::vector<vec3>& A, std::vector<vec3>& B, cv::Mat& OutMat) {
	if (A.size() != B.size()) {
		std::cout << "Error computerVisionFitPoints, point sets must be the same size\n";
		return;
	}

	int pointCount = (int)A.size();

	cv::Mat mA = cv::Mat(pointCount, 3, CV_32F);
	cv::Mat mB = cv::Mat(pointCount, 3, CV_32F);

	vec3 centroidA(0, 0, 0);
	vec3 centroidB(0, 0, 0);

	for (size_t i = 0; i < A.size(); ++i) {
		centroidA += A[i];
		centroidB += B[i];
	}

	centroidA /= (float)pointCount;
	centroidB /= (float)pointCount;

	for (size_t i = 0; i < A.size(); ++i) {
		mA.at<float>(i, 0) = A[i].x - centroidA.x;
		mA.at<float>(i, 1) = A[i].y - centroidA.y;
		mA.at<float>(i, 2) = A[i].z - centroidA.z;

		mB.at<float>(i, 0) = B[i].x - centroidB.x;
		mB.at<float>(i, 1) = B[i].y - centroidB.y;
		mB.at<float>(i, 2) = B[i].z - centroidB.z;
	}

	cv::Mat mP = mA * mB.t();

	cv::Mat w, u, vt;
	cv::SVD::compute(mP, w, u, vt);

	/*
	# find mean column wise
	centroid_A = np.mean(A, axis=1)
	centroid_B = np.mean(B, axis=1)

	# ensure centroids are 3x1
	centroid_A = centroid_A.reshape(-1, 1)
	centroid_B = centroid_B.reshape(-1, 1)

	# subtract mean
	Am = A - centroid_A
	Bm = B - centroid_B

	H = Am @ np.transpose(Bm)

	# sanity check
	#if linalg.matrix_rank(H) < 3:
	#    raise ValueError("rank of H = {}, expecting 3".format(linalg.matrix_rank(H)))

	# find rotation
	U, S, Vt = np.linalg.svd(H)
	R = Vt.T @ U.T

	# special reflection case
	if np.linalg.det(R) < 0:
		print("det(R) < R, reflection detected!, correcting for it ...")
		Vt[2,:] *= -1
		R = Vt.T @ U.T

	t = -R @ centroid_A + centroid_B

	return R, t
	*/
}

// Original code by Emil Ernerfeldt.
// https://www.ilikebigbits.com/2017_09_25_plane_from_points_2.html
void computerVisionFitPlane(std::vector<vec3>& Points, ldiPlane* ResultPlane) {
	float pointCount = Points.size();

	vec3 sum(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < pointCount; ++i) {
		sum += Points[i];
	}

	vec3 centroid = sum * (1.0f / pointCount);

	float xx = 0.0f;
	float xy = 0.0f;
	float xz = 0.0f;
	float yy = 0.0f;
	float yz = 0.0f;
	float zz = 0.0f;

	for (int i = 0; i < pointCount; ++i) {
		vec3 r = Points[i] - centroid;
		xx += r.x * r.x;
		xy += r.x * r.y;
		xz += r.x * r.z;
		yy += r.y * r.y;
		yz += r.y * r.z;
		zz += r.z * r.z;
	}

	xx /= pointCount;
	xy /= pointCount;
	xz /= pointCount;
	yy /= pointCount;
	yz /= pointCount;
	zz /= pointCount;

	vec3 weightedDir(0.0f, 0.0f, 0.0f);

	{
		float detX = yy * zz - yz * yz;
		vec3 axisDir = vec3(
			detX,
			xz * yz - xy * zz,
			xy * yz - xz * yy
		);

		float weight = detX * detX;

		if (glm::dot(weightedDir, axisDir) < 0.0f) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	{
		float detY = xx * zz - xz * xz;
		vec3 axisDir = vec3(
			xz * yz - xy * zz,
			detY,
			xy * xz - yz * xx
		);

		float weight = detY * detY;

		if (glm::dot(weightedDir, axisDir) < 0.0f) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	{
		float detZ = xx * yy - xy * xy;
		vec3 axisDir = vec3(
			xy * yz - xz * yy,
			xy * xz - yz * xx,
			detZ
		);

		float weight = detZ * detZ;

		if (glm::dot(weightedDir, axisDir) < 0.0f) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	vec3 normal = glm::normalize(weightedDir);

	ResultPlane->point = centroid;
	ResultPlane->normal = normal;

	if (glm::length(normal) == 0.0f) {
		ResultPlane->point = vec3Zero;
		ResultPlane->normal = vec3Up;
	}
}

void computerVisionBundleAdjust(std::vector<ldiCameraCalibSample>& InputSamples, cv::Mat* CameraMatrix, cv::Mat* CameraDist) {
	// NOTE: Really in-depth camera calibration: http://mrcal.secretsauce.net/how-to-calibrate.html

	// Each cam:
	// OpenCv calib
	// BA to refine.

	// Both cams:
	// Calibrate volume with single point model.
	// OpenCv to get basic triangualtion
	// BA varying combined single point model to get relative camera extrinsics.
	// BA varying with individual views (fixed intrinsics) to get model 3D points.
	
	int cornerSize = (int)_charucoBoards[0]->getChessboardCorners().size();

	// Bundle adjust needs:
	// Pose estimation for each sample.
	// Image points for each sample.
	// Corner global IDs for each sample.
	// Sample ID.

	std::vector<mat4> poses;
	std::vector<int> viewImageIds;
	std::vector<std::vector<cv::Point2f>> viewImagePoints;
	std::vector<std::vector<int>> viewImagePointIds;

	float squareSize = 1.5;
	std::vector<vec3> targetPointsTemp;
	for (int i = 0; i < 11; i++) {
		for (int j = 0; j < 4; j++) {
			targetPointsTemp.push_back(vec3((2 * j + i % 2) * squareSize, i * squareSize, 0));
		}
	}

	int targetPointCount = (int)targetPointsTemp.size();
	int totalImagePointCount = 0;

	// Find a pose for each calibration sample.
	for (size_t sampleIter = 0; sampleIter < InputSamples.size(); ++sampleIter) {
		//std::cout << "Sample " << sampleIter << ":\n";
		ldiCameraCalibSample* sample = &InputSamples[sampleIter];

		// Combine all boards into a set of image points and 3d target local points.
		std::vector<cv::Point2f> imagePoints;
		std::vector<int> imagePointIds;

		for (size_t pointIter = 0; pointIter < sample->imagePoints.size(); ++pointIter) {
			// NOTE: SSBA considers 0,0 to be pixel start. Shift Opencv points.
			imagePoints.push_back(cv::Point2f(sample->imagePoints[pointIter].x, sample->imagePoints[pointIter].y));
			imagePointIds.push_back(pointIter);
		}

		//std::cout << "Pose:\n" << GetMat4DebugString(&pose);
		poses.push_back(sample->camLocalMat);
		viewImageIds.push_back(sampleIter);
		viewImagePoints.push_back(imagePoints);
		viewImagePointIds.push_back(imagePointIds);
		totalImagePointCount += (int)imagePoints.size();
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
	//for (size_t poseIter = 0; poseIter < poses.size(); ++poseIter) {
		fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
			CameraMatrix->at<double>(0), CameraMatrix->at<double>(1), CameraMatrix->at<double>(2), CameraMatrix->at<double>(4), CameraMatrix->at<double>(5),
			CameraDist->at<double>(0), CameraDist->at<double>(1), CameraDist->at<double>(2), CameraDist->at<double>(3));
	//}

	// 3D points.
	for (size_t pointIter = 0; pointIter < targetPointsTemp.size(); ++pointIter) {
		vec3 point = targetPointsTemp[pointIter];
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
		fprintf(f, "%f", camRt[3][2]);

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

	// Calibration mode: metric, focal, principal, radial, tangential.
	char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt tangential";
	//char args[] = "\"BundleVarying.exe\" ../../bin/cache/ssba_input.txt metric";

	CreateProcessA(
		//"../../assets/bin/BundleVarying.exe",
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

void computerVisionLoadBundleAdjust(std::vector<ldiCameraCalibSample>& InputSamples, ldiBundleAdjustResult* BundleResult) {
	ldiBundleAdjustResult result = {};

	std::ifstream file("../cache/refined.txt");
	if (!file) {
		std::cout << "Could not open bundle adjust results\n";
		return;
	}

	float cp[9];
	file >> cp[0] >> cp[1] >> cp[2] >> cp[3] >> cp[4] >> cp[5] >> cp[6] >> cp[7] >> cp[8];
	std::cout << cp[0] << ", ";

	result.cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	result.cameraMatrix.at<double>(0, 0) = cp[0];
	result.cameraMatrix.at<double>(0, 1) = cp[1];
	result.cameraMatrix.at<double>(0, 2) = cp[2];
	result.cameraMatrix.at<double>(1, 0) = 0.0;
	result.cameraMatrix.at<double>(1, 1) = cp[3];
	result.cameraMatrix.at<double>(1, 2) = cp[4];
	result.cameraMatrix.at<double>(2, 0) = 0.0;
	result.cameraMatrix.at<double>(2, 1) = 0.0;
	result.cameraMatrix.at<double>(2, 2) = 1.0;

	std::cout << result.cameraMatrix << "\n";

	result.distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	result.distCoeffs.at<double>(0) = cp[5];
	result.distCoeffs.at<double>(1) = cp[6];
	result.distCoeffs.at<double>(2) = cp[7];
	result.distCoeffs.at<double>(3) = cp[8];

	std::cout << result.distCoeffs << "\n";

	int worldPointCount = 0;
	int viewCount = 0;
	int imagePointCount = 0;
	file >> worldPointCount >> viewCount >> imagePointCount;
	std::cout << "World points: " << worldPointCount << " Views: " << viewCount << " Image points: " << imagePointCount << "\n";

	std::vector<cv::Point3f> modelPoints;

	for (int i = 0; i < worldPointCount; ++i) {
		int id = 0;
		float x, y, z;

		file >> id >> x >> y >> z;

		if (id != i) {
			std::cout << "Bad model point id\n";
			return;
		}

		result.modelPoints.push_back(vec3(x, y, z));
		modelPoints.push_back(cv::Point3f(x, y, z));
	}

	for (int i = 0; i < viewCount; ++i) {
		int id;
		float p[12];
		mat4 camLocalMat = glm::identity<mat4>();

		file >> id;

		if (id != i) {
			std::cout << "Bad view id\n";
			return;
		}

		for (int j = 0; j < 12; ++j) {
			file >> p[j];
		}

		camLocalMat[0][0] = p[0];
		camLocalMat[1][0] = p[1];
		camLocalMat[2][0] = p[2];
		camLocalMat[3][0] = p[3];
		camLocalMat[0][1] = p[4];
		camLocalMat[1][1] = p[5];
		camLocalMat[2][1] = p[6];
		camLocalMat[3][1] = p[7];
		camLocalMat[0][2] = p[8];
		camLocalMat[1][2] = p[9];
		camLocalMat[2][2] = p[10];
		camLocalMat[3][2] = p[11];

		// Convert cam mat to rvec, tvec.
		cv::Mat reTvec = cv::Mat::zeros(3, 1, CV_64F);
		cv::Mat reRotMat = cv::Mat::zeros(3, 3, CV_64F);
		reTvec.at<double>(0) = camLocalMat[3][0];
		reTvec.at<double>(1) = camLocalMat[3][1];
		reTvec.at<double>(2) = camLocalMat[3][2];
		std::cout << "Re T: " << reTvec << "\n";

		reRotMat.at<double>(0, 0) = camLocalMat[0][0];
		reRotMat.at<double>(1, 0) = camLocalMat[0][1];
		reRotMat.at<double>(2, 0) = camLocalMat[0][2];
		reRotMat.at<double>(0, 1) = camLocalMat[1][0];
		reRotMat.at<double>(1, 1) = camLocalMat[1][1];
		reRotMat.at<double>(2, 1) = camLocalMat[1][2];
		reRotMat.at<double>(0, 2) = camLocalMat[2][0];
		reRotMat.at<double>(1, 2) = camLocalMat[2][1];
		reRotMat.at<double>(2, 2) = camLocalMat[2][2];
		std::cout << "ReRotMat: " << reRotMat << "\n";

		cv::Mat reRvec = cv::Mat::zeros(3, 1, CV_64F);
		cv::Rodrigues(reRotMat, reRvec);
		std::cout << "Re R: " << reRvec << "\n";

		ldiCameraCalibSample sample = {};
		sample.camLocalMat = camLocalMat;

		// NOTE: Adjust view matrix for our coord system.
		sample.camLocalMat[0][0] = sample.camLocalMat[0][0];
		sample.camLocalMat[0][1] = sample.camLocalMat[0][1];
		sample.camLocalMat[0][2] = -sample.camLocalMat[0][2];

		sample.camLocalMat[1][0] = sample.camLocalMat[1][0];
		sample.camLocalMat[1][1] = sample.camLocalMat[1][1];
		sample.camLocalMat[1][2] = -sample.camLocalMat[1][2];

		sample.camLocalMat[2][0] = sample.camLocalMat[2][0];
		sample.camLocalMat[2][1] = sample.camLocalMat[2][1];
		sample.camLocalMat[2][2] = -sample.camLocalMat[2][2];

		sample.camLocalMat[3][0] = sample.camLocalMat[3][0];
		sample.camLocalMat[3][1] = sample.camLocalMat[3][1];
		sample.camLocalMat[3][2] = -sample.camLocalMat[3][2];

		cv::Mat empty;
		cv::projectPoints(modelPoints, reRvec, reTvec, result.cameraMatrix, empty, sample.undistortedImagePoints);
		cv::projectPoints(modelPoints, reRvec, reTvec, result.cameraMatrix, result.distCoeffs, sample.reprojectedImagePoints);

		result.samples.push_back(sample);
	}

	// Calculate calibration target basis.
	computerVisionFitPlane(result.modelPoints, &result.targetInfo.plane);

	int targetRows = 11;
	int targetColumns = 8;

	vec3 basisU = {};
	vec3 basisV = {};

	for (int i = 0; i < targetRows; ++i) {
		int startPoint = i * (targetColumns / 2);

		std::vector<vec3> fitPoints;
		for (int j = 0; j < targetColumns / 2; ++j) {
			fitPoints.push_back(result.modelPoints[j + startPoint]);
		}

		ldiLineFit lineFit = computerVisionFitLine(fitPoints);
		ldiLineSegment lineSeg;
		lineSeg.a = lineFit.origin - lineFit.direction * 10.0f;
		lineSeg.b = lineFit.origin + lineFit.direction * 10.0f;

		result.targetInfo.fits.push_back(lineSeg);

		basisU += lineFit.direction;
	}

	result.targetInfo.basisX = glm::normalize(basisU);
	
	for (int i = 0; i < targetColumns; ++i) {
		int startPoint = i;

		std::vector<vec3> fitPoints;
		for (int j = 0; j < 5; ++j) {
			fitPoints.push_back(result.modelPoints[startPoint + j * 8]);
		}

		ldiLineFit lineFit = computerVisionFitLine(fitPoints);
		ldiLineSegment lineSeg;
		lineSeg.a = lineFit.origin - lineFit.direction * 10.0f;
		lineSeg.b = lineFit.origin + lineFit.direction * 10.0f;

		result.targetInfo.fits.push_back(lineSeg);

		basisV += lineFit.direction;
	}

	result.targetInfo.basisY = glm::normalize(basisV);
	result.targetInfo.basisZ = result.targetInfo.plane.normal;

	// TODO: normalize?
	result.targetInfo.basisXortho = glm::cross(result.targetInfo.basisY, result.targetInfo.basisZ);
	result.targetInfo.basisYortho = glm::cross(result.targetInfo.basisZ, result.targetInfo.basisXortho);

	*BundleResult = result;
}

void computerVisionBundleAdjustStereo(ldiCalibrationJob* Job) {
	int totalImagePointCount = Job->massModelImagePoints[0].size() + Job->massModelImagePoints[1].size();

	// Write SSBA input file.
	FILE* f;
	fopen_s(&f, "../cache/ssba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d %d\n", Job->massModelTriangulatedPoints.size(), 2, totalImagePointCount);

	// Camera intrinsics.
	for (int i = 0; i < 2; ++i) {
		cv::Mat calibCameraMatrix = Job->startCamMat[i];
		cv::Mat calibCameraDist = Job->startDistMat[i];
		
		fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
			calibCameraMatrix.at<double>(0), calibCameraMatrix.at<double>(1), calibCameraMatrix.at<double>(2), calibCameraMatrix.at<double>(4), calibCameraMatrix.at<double>(5),
			calibCameraDist.at<double>(0), calibCameraDist.at<double>(1), calibCameraDist.at<double>(2), calibCameraDist.at<double>(3));
	}

	// 3D points.
	for (size_t pointIter = 0; pointIter < Job->massModelTriangulatedPoints.size(); ++pointIter) {
		int pointId = Job->massModelPointIds[pointIter];
		vec3 point = Job->massModelTriangulatedPoints[pointIter];
		fprintf(f, "%d %f %f %f\n", pointId, point.x, point.y, point.z);
	}

	// View poses.
	for (int poseIter = 0; poseIter < 2; ++poseIter) {
		cv::Mat camRt = Job->rtMat[poseIter];
		fprintf(f, "%d ", poseIter);

		for (int iX = 0; iX < 3; ++iX) {
			for (int iY = 0; iY < 4; ++iY) {
				fprintf(f, " %f", camRt.at<double>(iX, iY));
			}
		}

		// ViewPointImageId 12 value camRT matrix.
		//for (int j = 0; j < 12; ++j) {
			// NOTE: Index column major, output as row major.
			//fprintf(f, " %f", camRt[(j % 4) * 4 + (j / 4)]);
		//}

		// NOTE: Index column major, output as row major.
		/*fprintf(f, "%f ", camRt[0][0]);
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
		fprintf(f, "%f", camRt[3][2]);*/

		fprintf(f, "\n");
	}

	// Image points.
	for (size_t viewIter = 0; viewIter < 2; ++viewIter) {
		std::vector<vec2>* viewPoints = &Job->massModelImagePoints[viewIter];

		for (size_t pointIter = 0; pointIter < (*viewPoints).size(); ++pointIter) {
			vec2 point = (*viewPoints)[pointIter];
			int pointId = Job->massModelPointIds[pointIter];
			
			fprintf(f, "%d %d %f %f 1\n", viewIter, pointId, point.x, point.y);
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

	// Calibration mode: metric, focal, principal, radial, tangential.
	//char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt tangential";
	char args[] = "\"BundleVarying.exe\" ../../bin/cache/ssba_input.txt principal";

	CreateProcessA(
		"../../assets/bin/BundleVarying.exe",
		//"../../assets/bin/BundleCommon.exe",
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

void computerVisionLoadBundleAdjustStereo(ldiCalibrationJob* Job) {
	std::ifstream file("../cache/refined.txt");
	if (!file) {
		std::cout << "Could not open bundle adjust results\n";
		return;
	}

	int worldPointCount = 0;
	int viewCount = 0;
	int imagePointCount = 0;
	file >> worldPointCount >> viewCount >> imagePointCount;
	std::cout << "World points: " << worldPointCount << " Views: " << viewCount << " Image points: " << imagePointCount << "\n";

	// Load each camera intrinsics.
	for (int i = 0; i < 2; ++i ) {
		float cp[9];
		file >> cp[0] >> cp[1] >> cp[2] >> cp[3] >> cp[4] >> cp[5] >> cp[6] >> cp[7] >> cp[8];
		std::cout << cp[0] << ", ";

		cv::Mat calibCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
		calibCameraMatrix.at<double>(0, 0) = cp[0];
		calibCameraMatrix.at<double>(0, 1) = cp[1];
		calibCameraMatrix.at<double>(0, 2) = cp[2];
		calibCameraMatrix.at<double>(1, 0) = 0.0;
		calibCameraMatrix.at<double>(1, 1) = cp[3];
		calibCameraMatrix.at<double>(1, 2) = cp[4];
		calibCameraMatrix.at<double>(2, 0) = 0.0;
		calibCameraMatrix.at<double>(2, 1) = 0.0;
		calibCameraMatrix.at<double>(2, 2) = 1.0;

		std::cout << calibCameraMatrix << "\n";

		cv::Mat calibCameraDist = cv::Mat::zeros(8, 1, CV_64F);
		calibCameraDist.at<double>(0) = cp[5];
		calibCameraDist.at<double>(1) = cp[6];
		calibCameraDist.at<double>(2) = cp[7];
		calibCameraDist.at<double>(3) = cp[8];

		std::cout << calibCameraDist << "\n";

		Job->camMatrix[i] = calibCameraMatrix;
		Job->camDist[i] = calibCameraDist;
	}

	Job->massModelTriangulatedPointsBundleAdjust.clear();
	Job->massModelBundleAdjustPointIds.clear();

	
	//std::vector<ldiCubeCentroid> cubeCentroids;
	//cubeCentroids.resize(100);
	//for (size_t i = 0; i < cubeCentroids.size(); ++i) {
		//cubeCentroids[i] = ldiCubeCentroid{};
	//}

	std::vector<vec3> originPoints;
	std::vector<int> originPointIds;

	int lastSampleId = 0;
	std::vector<vec3> matchSets[2];
	std::vector<vec3> sampleMat;
	sampleMat.resize(100);

	for (int i = 0; i < worldPointCount; ++i) {
		int id = 0;
		float x, y, z;

		file >> id >> x >> y >> z;

		int sampleId = id / (9 * 6);
		int globalId = id % (9 * 6);
		int boardId = globalId / 9;
		int pointId = globalId % 9;

		Job->massModelTriangulatedPointsBundleAdjust.push_back(vec3(x, y, z));
		Job->massModelBundleAdjustPointIds.push_back(id);

		if (sampleId == 0) {
			originPoints.push_back(vec3(x, y, z));
			originPointIds.push_back(globalId);
		} else {
			if (i == worldPointCount - 1) {
				for (size_t originIter = 0; originIter < originPointIds.size(); ++originIter) {
					if (originPointIds[originIter] == globalId) {
						matchSets[0].push_back(originPoints[originIter]);
						matchSets[1].push_back(vec3(x, y, z));
						break;
					}
				}
			}

			if (lastSampleId != sampleId || i == worldPointCount - 1) {
				if (lastSampleId != 0) {
					cv::Mat inliers;
					//std::cout << "Estimate sample " << lastSampleId << "\n";
					//cv::estimateAffine3D(matchSets[0], matchSets[1], sampleMat[lastSampleId], inliers, 3.0, 0.999);
					//computerVisionFitPoints(matchSets[0], matchSets[1], sampleMat[lastSampleId]);

					vec3 centroidA(0, 0, 0);
					vec3 centroidB(0, 0, 0);

					for (size_t j = 0; j < matchSets[0].size(); ++j) {
						centroidA += matchSets[0][j];
						centroidB += matchSets[1][j];
					}

					centroidA /= (float)matchSets[0].size();
					centroidB /= (float)matchSets[0].size();

					sampleMat[lastSampleId] = centroidA - centroidB;
				}

				lastSampleId = sampleId;

				matchSets[0].clear();
				matchSets[1].clear();
			}

			if (i < worldPointCount - 1) {
				for (size_t originIter = 0; originIter < originPointIds.size(); ++originIter) {
					if (originPointIds[originIter] == globalId) {
						matchSets[0].push_back(originPoints[originIter]);
						matchSets[1].push_back(vec3(x, y, z));
						break;
					}
				}
			}
		}
	}

	for (int i = 0; i < 2; ++i) {
		int id;
		float p[12];
		mat4 camLocalMat = glm::identity<mat4>();

		file >> id;

		if (id != i) {
			std::cout << "Bad view id\n";
			return;
		}

		for (int j = 0; j < 12; ++j) {
			file >> p[j];
		}

		camLocalMat[0][0] = p[0];
		camLocalMat[1][0] = p[1];
		camLocalMat[2][0] = p[2];
		camLocalMat[3][0] = p[3];
		camLocalMat[0][1] = p[4];
		camLocalMat[1][1] = p[5];
		camLocalMat[2][1] = p[6];
		camLocalMat[3][1] = p[7];
		camLocalMat[0][2] = p[8];
		camLocalMat[1][2] = p[9];
		camLocalMat[2][2] = p[10];
		camLocalMat[3][2] = p[11];

		// NOTE: Adjust view matrix for our coord system.
		camLocalMat[0][0] = camLocalMat[0][0];
		camLocalMat[0][1] = camLocalMat[0][1];
		camLocalMat[0][2] = -camLocalMat[0][2];

		camLocalMat[1][0] = camLocalMat[1][0];
		camLocalMat[1][1] = camLocalMat[1][1];
		camLocalMat[1][2] = -camLocalMat[1][2];

		camLocalMat[2][0] = camLocalMat[2][0];
		camLocalMat[2][1] = camLocalMat[2][1];
		camLocalMat[2][2] = -camLocalMat[2][2];

		camLocalMat[3][0] = camLocalMat[3][0];
		camLocalMat[3][1] = camLocalMat[3][1];
		camLocalMat[3][2] = -camLocalMat[3][2];

		Job->camVolumeMat[i] = camLocalMat;
	}

	// Find transformation basis.
	// Build lines for all X change points
	// 16 columns

	struct ldiTbPoint {
		vec3 pos;
		int id;
	};

	struct ldiTbCube {
		int x;
		int y;
		int z;
		std::vector<ldiTbPoint> points;
	};

	struct ldiTbColumn {
		int x;
		int y;
		int z;
		std::vector<ldiTbCube> cubes;

		std::vector<std::vector<vec3>> pointGroups;
	};

	std::vector<ldiTbColumn> columnX;
	std::vector<ldiTbColumn> columnY;
	std::vector<ldiTbColumn> columnZ;

	for (size_t i = 0; i < Job->massModelBundleAdjustPointIds.size(); ++i) {
		int id = Job->massModelBundleAdjustPointIds[i];
		int sampleId = id / (9 * 6);
		int globalId = id % (9 * 6);
		vec3 pos = Job->massModelTriangulatedPointsBundleAdjust[i];

		int x = Job->samples[sampleId].X;
		int y = Job->samples[sampleId].Y;
		int z = Job->samples[sampleId].Z;

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
				column.pointGroups.resize(9 * 6);

				columnX.push_back(column);
			}

			columnX[columnId].pointGroups[globalId].push_back(pos);

			size_t cubeId = -1;
			for (size_t i = 0; i < columnX[columnId].cubes.size(); ++i) {
				ldiTbCube* cube = &columnX[columnId].cubes[i];

				if (cube->x == x && cube->y == y && cube->z == z) {
					cubeId = i;
					break;
				}
			}

			if (cubeId == -1) {
				cubeId = columnX[columnId].cubes.size();

				ldiTbCube cube;
				cube.x = x;
				cube.y = y;
				cube.z = z;

				columnX[columnId].cubes.push_back(cube);
			}

			ldiTbPoint point;
			point.pos = pos;
			point.id = globalId;

			columnX[columnId].cubes[cubeId].points.push_back(point);
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
				column.pointGroups.resize(9 * 6);

				columnY.push_back(column);
			}

			columnY[columnId].pointGroups[globalId].push_back(pos);

			size_t cubeId = -1;
			for (size_t i = 0; i < columnY[columnId].cubes.size(); ++i) {
				ldiTbCube* cube = &columnY[columnId].cubes[i];

				if (cube->x == x && cube->y == y && cube->z == z) {
					cubeId = i;
					break;
				}
			}

			if (cubeId == -1) {
				cubeId = columnY[columnId].cubes.size();

				ldiTbCube cube;
				cube.x = x;
				cube.y = y;
				cube.z = z;

				columnY[columnId].cubes.push_back(cube);
			}

			ldiTbPoint point;
			point.pos = pos;
			point.id = globalId;

			columnY[columnId].cubes[cubeId].points.push_back(point);
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
				column.pointGroups.resize(9 * 6);

				columnZ.push_back(column);
			}

			columnZ[columnId].pointGroups[globalId].push_back(pos);

			size_t cubeId = -1;
			for (size_t i = 0; i < columnZ[columnId].cubes.size(); ++i) {
				ldiTbCube* cube = &columnZ[columnId].cubes[i];

				if (cube->x == x && cube->y == y && cube->z == z) {
					cubeId = i;
					break;
				}
			}

			if (cubeId == -1) {
				cubeId = columnZ[columnId].cubes.size();

				ldiTbCube cube;
				cube.x = x;
				cube.y = y;
				cube.z = z;

				columnZ[columnId].cubes.push_back(cube);
			}

			ldiTbPoint point;
			point.pos = pos;
			point.id = globalId;

			columnZ[columnId].cubes[cubeId].points.push_back(point);
		}
	}

	// Get distance between corresponding points in each column.
	
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
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].cubes.size(); ++cubeIter0) {
				ldiTbCube* cube0 = &(*column)[colIter].cubes[cubeIter0];
				
				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].cubes.size(); ++cubeIter1) {
					ldiTbCube* cube1 = &(*column)[colIter].cubes[cubeIter1];

					// Compare all points.
					double cubeMechDist = glm::distance(vec3(cube0->x, cube0->y, cube0->z), vec3(cube1->x, cube1->y, cube1->z));

					for (size_t pointIter0 = 0; pointIter0 < cube0->points.size(); ++pointIter0) {
						ldiTbPoint point0 = cube0->points[pointIter0];

						for (size_t pointIter1 = 0; pointIter1 < cube1->points.size(); ++pointIter1) {
							ldiTbPoint point1 = cube1->points[pointIter1];

							if (point0.id == point1.id) {
								// Match point.
								double distVolSpace = glm::distance(point0.pos, point1.pos);
								double distNorm = distVolSpace / cubeMechDist;

								distAccum += distNorm;
								distAccumCount += 1;

								//std::cout << "Cube point dist: " << distNorm << "\n";

								break;
							}
						}
					}
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
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].cubes.size(); ++cubeIter0) {
				ldiTbCube* cube0 = &(*column)[colIter].cubes[cubeIter0];

				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].cubes.size(); ++cubeIter1) {
					ldiTbCube* cube1 = &(*column)[colIter].cubes[cubeIter1];

					// Compare all points.
					double cubeMechDist = glm::distance(vec3(cube0->x, cube0->y, cube0->z), vec3(cube1->x, cube1->y, cube1->z));

					for (size_t pointIter0 = 0; pointIter0 < cube0->points.size(); ++pointIter0) {
						ldiTbPoint point0 = cube0->points[pointIter0];

						for (size_t pointIter1 = 0; pointIter1 < cube1->points.size(); ++pointIter1) {
							ldiTbPoint point1 = cube1->points[pointIter1];

							if (point0.id == point1.id) {
								// Match point.
								double distVolSpace = glm::distance(point0.pos, point1.pos);
								double distNorm = distVolSpace / cubeMechDist;

								distAccum += distNorm;
								distAccumCount += 1;

								//std::cout << "Cube point dist: " << distNorm << "\n";

								break;
							}
						}
					}
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
			for (size_t cubeIter0 = 0; cubeIter0 < (*column)[colIter].cubes.size(); ++cubeIter0) {
				ldiTbCube* cube0 = &(*column)[colIter].cubes[cubeIter0];

				for (size_t cubeIter1 = cubeIter0 + 1; cubeIter1 < (*column)[colIter].cubes.size(); ++cubeIter1) {
					ldiTbCube* cube1 = &(*column)[colIter].cubes[cubeIter1];

					// Compare all points.
					double cubeMechDist = glm::distance(vec3(cube0->x, cube0->y, cube0->z), vec3(cube1->x, cube1->y, cube1->z));

					for (size_t pointIter0 = 0; pointIter0 < cube0->points.size(); ++pointIter0) {
						ldiTbPoint point0 = cube0->points[pointIter0];

						for (size_t pointIter1 = 0; pointIter1 < cube1->points.size(); ++pointIter1) {
							ldiTbPoint point1 = cube1->points[pointIter1];

							if (point0.id == point1.id) {
								// Match point.
								double distVolSpace = glm::distance(point0.pos, point1.pos);
								double distNorm = distVolSpace / cubeMechDist;

								distAccum += distNorm;
								distAccumCount += 1;

								std::cout << "Cube point dist: " << distNorm << "\n";

								break;
							}
						}
					}
				}
			}
		}

		distAvgZ = distAccum / (double)distAccumCount;
	}


	std::cout << "Dist avg X: " << distAvgX << "\n";
	std::cout << "Dist avg Y: " << distAvgY << "\n";
	std::cout << "Dist avg Z: " << distAvgZ << "\n";

	// Get basis lines.
	Job->centeredPointGroups.resize(9 * 6);

	std::vector<vec3> pointsX;
	std::vector<vec3> pointsY;
	std::vector<vec3> pointsZ;

	for (size_t i = 0; i < columnX.size(); ++i) {
		for (size_t j = 0; j < columnX[i].pointGroups.size(); ++j) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnX[i].pointGroups[j].size(); ++pointIter) {
				centroid += columnX[i].pointGroups[j][pointIter];
			}

			centroid /= (float)columnX[i].pointGroups[j].size();

			for (size_t pointIter = 0; pointIter < columnX[i].pointGroups[j].size(); ++pointIter) {
				vec3 transPoint = columnX[i].pointGroups[j][pointIter] - centroid;
				Job->centeredPointGroups[j].push_back(transPoint);

				pointsX.push_back(transPoint);
			}
		}
	}

	for (size_t i = 0; i < columnY.size(); ++i) {
		for (size_t j = 0; j < columnY[i].pointGroups.size(); ++j) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnY[i].pointGroups[j].size(); ++pointIter) {
				centroid += columnY[i].pointGroups[j][pointIter];
			}

			centroid /= (float)columnY[i].pointGroups[j].size();

			for (size_t pointIter = 0; pointIter < columnY[i].pointGroups[j].size(); ++pointIter) {
				vec3 transPoint = columnY[i].pointGroups[j][pointIter] - centroid;
				Job->centeredPointGroups[j].push_back(transPoint);

				pointsY.push_back(transPoint);
			}
		}
	}

	for (size_t i = 0; i < columnZ.size(); ++i) {
		for (size_t j = 0; j < columnZ[i].pointGroups.size(); ++j) {
			vec3 centroid(0, 0, 0);

			for (size_t pointIter = 0; pointIter < columnZ[i].pointGroups[j].size(); ++pointIter) {
				centroid += columnZ[i].pointGroups[j][pointIter];
			}

			centroid /= (float)columnZ[i].pointGroups[j].size();

			for (size_t pointIter = 0; pointIter < columnZ[i].pointGroups[j].size(); ++pointIter) {
				vec3 transPoint = columnZ[i].pointGroups[j][pointIter] - centroid;
				Job->centeredPointGroups[j].push_back(transPoint);

				pointsZ.push_back(transPoint);
			}
		}
	}

	Job->axisX = computerVisionFitLine(pointsX);
	Job->axisY = computerVisionFitLine(pointsY);
	Job->axisZ = computerVisionFitLine(pointsZ);

	Job->basisX = Job->axisX.direction;
	Job->basisY = glm::normalize(glm::cross(Job->basisX, Job->axisZ.direction));
	Job->basisZ = glm::normalize(glm::cross(Job->basisX, Job->basisY));

	// Find centroid of each point.
	Job->cubePointCentroids.resize(6 * 9, vec3(0, 0, 0));
	Job->cubePointCounts.resize(6 * 9, 0);

	// Compact each sample centroid to sample 0 centroid.
	for (size_t i = 0; i < Job->massModelBundleAdjustPointIds.size(); ++i) {
		int id = Job->massModelBundleAdjustPointIds[i];
		int sampleId = id / (9 * 6);
		int globalId = id % (9 * 6);
		int boardId = globalId / 9;
		int pointId = globalId % 9;

		vec3 collapsedPoint = Job->massModelTriangulatedPointsBundleAdjust[i];

		if (sampleId != 0) {
			vec3 point = Job->massModelTriangulatedPointsBundleAdjust[i];
			collapsedPoint = point + sampleMat[sampleId];
			//Job->massModelTriangulatedPointsBundleAdjust[i] = collapsedPoint;
		}

		// Compute centroid for each point.
		Job->cubePointCentroids[globalId] += collapsedPoint;
		Job->cubePointCounts[globalId] += 1;
	}

	// Average each centroid.
	for (size_t i = 0; i < Job->cubePointCentroids.size(); ++i) {
		Job->cubePointCentroids[i] /= Job->cubePointCounts[i];
	}

	Job->cubePosSteps = vec3(Job->samples[0].X, Job->samples[0].Y, Job->samples[0].Z);

	// Calculate volume to world scale:
	float scaleAccum = 0.0f;
	int scaleAccumCount = 0;

	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		int globalIdBase = boardIter * 9;

		// Match points: 0-1, 1-2, 3-4, 4-5, 6-7, 7-8,
		// 0-3, 3-6, 1-4, 4-7, 2-5, 5-8

		for (int i = 0; i < 3; ++i) {
			// Rows
			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
				int pId0 = globalIdBase + (i * 3) + pairIter + 0;
				int pId1 = globalIdBase + (i * 3) + pairIter + 1;

				if (Job->cubePointCounts[pId0] != 0 && Job->cubePointCounts[pId1]) {
					float dist = glm::distance(Job->cubePointCentroids[pId0], Job->cubePointCentroids[pId1]);
					std::cout << "Row (Board " << boardIter << ") " << pId0 << " - " << pId1 << ": " << dist << "\n";
					scaleAccum += dist;
					scaleAccumCount += 1;
				}
			}

			// Columns
			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
				int pId0 = globalIdBase + i + (pairIter * 3) + 0;
				int pId1 = globalIdBase + i + (pairIter * 3) + 3;

				if (Job->cubePointCounts[pId0] != 0 && Job->cubePointCounts[pId1]) {
					float dist = glm::distance(Job->cubePointCentroids[pId0], Job->cubePointCentroids[pId1]);
					std::cout << "Col (Board " << boardIter << ") " << pId0 << " - " << pId1 << ": " << dist << "\n";
					scaleAccum += dist;
					scaleAccumCount += 1;
				}
			}
		}
	}

	float scaleAvg = scaleAccum / (float)scaleAccumCount;
	float invScale = 0.9 / scaleAvg;
	std::cout << "Scale avg: " << scaleAvg << "\n";

	distAvgX *= invScale;
	distAvgY *= invScale;
	distAvgZ *= invScale;
	std::cout << "Dist avg X: " << distAvgX << "\n";
	std::cout << "Dist avg Y: " << distAvgY << "\n";
	std::cout << "Dist avg Z: " << distAvgZ << "\n";

	Job->stepsToCm = vec3(distAvgX, distAvgY, distAvgZ);

	mat4 worldToVolume = glm::identity<mat4>();

	worldToVolume[0] = vec4(Job->basisX, 0.0f);
	worldToVolume[1] = vec4(Job->basisY, 0.0f);
	worldToVolume[2] = vec4(Job->basisZ, 0.0f);

	mat4 volumeToWorld = glm::inverse(worldToVolume);
	mat4 volumeRealign = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
	volumeToWorld = volumeRealign * volumeToWorld;

	
	volumeRealign = glm::scale(mat4(1.0f), vec3(invScale, invScale, invScale));
	volumeToWorld = volumeRealign * volumeToWorld;

	volumeRealign = glm::translate(mat4(1.0f), vec3(0.0f, 14.0f, 0.0f));
	volumeToWorld = volumeRealign * volumeToWorld;
	
	// Move all volume elements into world space.
	{
		// Move axis directions.
		Job->axisX.direction = glm::normalize(volumeToWorld * vec4(Job->axisX.direction, 0.0f));
		Job->axisY.direction = glm::normalize(volumeToWorld * vec4(Job->axisY.direction, 0.0f));
		Job->axisZ.direction = glm::normalize(volumeToWorld * vec4(Job->axisZ.direction, 0.0f));
		
		// Move mass model.
		for (size_t i = 0; i < Job->massModelBundleAdjustPointIds.size(); ++i) {
			vec3 point = Job->massModelTriangulatedPointsBundleAdjust[i];
			point = volumeToWorld * vec4(point, 1.0f);
			Job->massModelTriangulatedPointsBundleAdjust[i] = point;
		}

		// Move cameras to world space.
		Job->camVolumeMat[0] = volumeToWorld * glm::inverse(Job->camVolumeMat[0]);
		Job->camVolumeMat[1] = volumeToWorld * glm::inverse(Job->camVolumeMat[1]);

		// Normalize scale.
		Job->camVolumeMat[0][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][0])), 0.0f);
		Job->camVolumeMat[0][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][1])), 0.0f);
		Job->camVolumeMat[0][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][2])), 0.0f);

		Job->camVolumeMat[1][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][0])), 0.0f);
		Job->camVolumeMat[1][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][1])), 0.0f);
		Job->camVolumeMat[1][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][2])), 0.0f);

		// Move cube centroids.
		for (size_t i = 0; i < Job->cubePointCentroids.size(); ++i) {
			Job->cubePointCentroids[i] = volumeToWorld * vec4(Job->cubePointCentroids[i], 1.0f);
		}
	}
}

mat4 computerVisionConvertOpenCvTransMattoGlmMat(cv::Mat& TransMat) {
	mat4 result = glm::identity<mat4>();

	for (int iY = 0; iY < 3; ++iY) {
		for (int iX = 0; iX < 4; ++iX) {
			result[iX][iY] = TransMat.at<double>(iY, iX);
		}
	}

	return result;
}

void computerVisionBundleAdjustStereoIndividual(ldiCalibrationJob* Job) {
	// Construct temp 3D cube representation.
	std::vector<ldiCalibCubePoint> cubePoints;
	std::vector<vec3> cubeGlobalPoints;
	cubeGlobalPoints.resize(9 * 6, vec3(0.0f, 0.0f, 0.0f));

	float ps = 0.9f;
	cubePoints.push_back({ 0, 0, 0, { 0.0f, ps * 0, ps * 0 } });
	cubePoints.push_back({ 0, 0, 3, { 0.0f, ps * 0, ps * 1 } });
	cubePoints.push_back({ 0, 0, 6, { 0.0f, ps * 0, ps * 2 } });

	cubePoints.push_back({ 0, 0, 1, { 0.0f, ps * 1, ps * 0 } });
	cubePoints.push_back({ 0, 0, 4, { 0.0f, ps * 1, ps * 1 } });
	cubePoints.push_back({ 0, 0, 7, { 0.0f, ps * 1, ps * 2 } });

	cubePoints.push_back({ 0, 0, 2, { 0.0f, ps * 2, ps * 0 } });
	cubePoints.push_back({ 0, 0, 5, { 0.0f, ps * 2, ps * 1 } });
	cubePoints.push_back({ 0, 0, 8, { 0.0f, ps * 2, ps * 2 } });

	// Top
	cubePoints.push_back({ 0, 0, 11, { -ps * 0 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 10, { -ps * 0 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0,  9, { -ps * 0 - 1.1f, 2.9f, ps * 2 } });

	cubePoints.push_back({ 0, 0, 14, { -ps * 1 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 13, { -ps * 1 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0, 12, { -ps * 1 - 1.1f, 2.9f, ps * 2 } });

	cubePoints.push_back({ 0, 0, 17, { -ps * 2 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 16, { -ps * 2 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0, 15, { -ps * 2 - 1.1f, 2.9f, ps * 2 } });

	for (size_t i = 0; i < cubePoints.size(); ++i) {
		cubeGlobalPoints[cubePoints[i].globalId] = cubePoints[i].position;
	}

	std::vector<mat4> poses;
	std::vector<int> viewImageIds;
	std::vector<std::vector<cv::Point2f>> viewImagePoints;
	std::vector<std::vector<int>> viewImagePointIds;
	int totalImagePointCount = 0;

	int hawkId = 0;

	// Find a pose for each calibration sample.
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		//std::cout << "Sample " << sampleIter << ":\n";

		// Combine all boards into a set of image points and 3d target local points.
		std::vector<cv::Point2f> imagePoints;
		std::vector<cv::Point3f> worldPoints;
		std::vector<int> imagePointIds;

		std::vector<ldiCharucoBoard>* boards = &Job->samples[sampleIter].cubes[hawkId].boards;

		for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
			ldiCharucoBoard* board = &(*boards)[boardIter];

			//std::cout << "    Board " << boardIter << " (" << board->id << "):\n";

			for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
				ldiCharucoCorner* corner = &board->corners[cornerIter];

				//int cornerGlobalId = (board->id * 25) + corner->id;
				int cornerGlobalId = (board->id * 9) + corner->id;
				vec3 targetPoint = cubeGlobalPoints[cornerGlobalId];

				imagePoints.push_back(cv::Point2f(corner->position.x, corner->position.y));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);

				//std::cout << "        Corner " << cornerIter << " (" << cornerGlobalId << ") " << corner->position.x << ", " << corner->position.y << " " << targetPoint.x << ", " << targetPoint.y << ", " << targetPoint.z << "\n";
			}
		}

		if (imagePoints.size() >= 6) {
			mat4 pose;

			std::cout << "Find pose - Sample: " << sampleIter << " :\n";
			if (computerVisionFindGeneralPose(&Job->startCamMat[hawkId], &Job->startDistMat[hawkId], &imagePoints, &worldPoints, &pose)) {
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
	fprintf(f, "%d %d %d\n", cubePoints.size(), poses.size(), totalImagePointCount);

	// Camera intrinsics.
	cv::Mat calibCameraMatrix = Job->startCamMat[hawkId];
	cv::Mat calibCameraDist = Job->startDistMat[hawkId];

	fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
		calibCameraMatrix.at<double>(0), calibCameraMatrix.at<double>(1), calibCameraMatrix.at<double>(2), calibCameraMatrix.at<double>(4), calibCameraMatrix.at<double>(5),
		calibCameraDist.at<double>(0), calibCameraDist.at<double>(1), calibCameraDist.at<double>(2), calibCameraDist.at<double>(3));

	// 3D points.
	for (size_t pointIter = 0; pointIter < cubePoints.size(); ++pointIter) {
		int pointId = cubePoints[pointIter].globalId;
		vec3 point = cubePoints[pointIter].position;
		fprintf(f, "%d %f %f %f\n", pointId, point.x, point.y, point.z);
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
		fprintf(f, "%f", camRt[3][2]);

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

	// Calibration mode: metric, focal, principal, radial, tangential.
	char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt metric";
	//char args[] = "\"BundleVarying.exe\" ../../bin/cache/ssba_input.txt principal";

	CreateProcessA(
		//"../../assets/bin/BundleVarying.exe",
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

bool computerVisionBundleAdjustStereoIndividualLoad(ldiCalibrationJob* Job) {
	FILE* file;
	fopen_s(&file, "../cache/refined.txt", "r");

	float ci[9];
	int modelPointCount = 0;
	int viewCount = 0;
	int imagePointCount = 0;

	fscanf_s(file, "%f %f %f %f %f %f %f %f %f\r\n", &ci[0], &ci[1], &ci[2], &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]);

	fscanf_s(file, "%d %d %d\r\n", &modelPointCount, &viewCount, &imagePointCount);
	std::cout << "Model point count: " << modelPointCount << " View count: " << viewCount << " Image point count: " << imagePointCount << "\n";

	Job->baIndvCubePoints.clear();

	for (int i = 0; i < modelPointCount; ++i) {
		vec3 position;
		int pointId;

		fscanf_s(file, "%d %f %f %f\r\n", &pointId, &position[0], &position[1], &position[2]);
		//std::cout << pointId << " " << position.x << ", " << position.y << ", " << position.z << "\n";

		Job->baIndvCubePoints.push_back(position);
	}

	Job->baViews.clear();
	for (int i = 0; i < viewCount; ++i) {
		int viewId;
		mat4 viewMat = glm::identity<mat4>();

		fscanf_s(file, "%d %f %f %f %f %f %f %f %f %f %f %f %f\r\n", &viewId,
			&viewMat[0][0], &viewMat[1][0], &viewMat[2][0], &viewMat[3][0],
			&viewMat[0][1], &viewMat[1][1], &viewMat[2][1], &viewMat[3][1],
			&viewMat[0][2], &viewMat[1][2], &viewMat[2][2], &viewMat[3][2]);

		std::cout << "View " << viewId << "\n" << GetMat4DebugString(&viewMat);

		Job->baViews.push_back(viewMat);
	}

	fclose(file);

	return true;
}

void computerVisionBundleAdjustStereoBoth(ldiCalibrationJob* Job) {
	// Construct temp 3D cube representation.
	std::vector<ldiCalibCubePoint> cubePoints;
	std::vector<vec3> cubeGlobalPoints;
	cubeGlobalPoints.resize(9 * 6, vec3(0.0f, 0.0f, 0.0f));

	float ps = 0.9f;
	cubePoints.push_back({ 0, 0, 0, { 0.0f, ps * 0, ps * 0 } });
	cubePoints.push_back({ 0, 0, 3, { 0.0f, ps * 0, ps * 1 } });
	cubePoints.push_back({ 0, 0, 6, { 0.0f, ps * 0, ps * 2 } });

	cubePoints.push_back({ 0, 0, 1, { 0.0f, ps * 1, ps * 0 } });
	cubePoints.push_back({ 0, 0, 4, { 0.0f, ps * 1, ps * 1 } });
	cubePoints.push_back({ 0, 0, 7, { 0.0f, ps * 1, ps * 2 } });

	cubePoints.push_back({ 0, 0, 2, { 0.0f, ps * 2, ps * 0 } });
	cubePoints.push_back({ 0, 0, 5, { 0.0f, ps * 2, ps * 1 } });
	cubePoints.push_back({ 0, 0, 8, { 0.0f, ps * 2, ps * 2 } });

	// Top
	cubePoints.push_back({ 0, 0, 15, { -ps * 0 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 16, { -ps * 0 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0, 17, { -ps * 0 - 1.1f, 2.9f, ps * 2 } });

	cubePoints.push_back({ 0, 0, 12, { -ps * 1 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 13, { -ps * 1 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0, 14, { -ps * 1 - 1.1f, 2.9f, ps * 2 } });

	cubePoints.push_back({ 0, 0,  9, { -ps * 2 - 1.1f, 2.9f, ps * 0 } });
	cubePoints.push_back({ 0, 0, 10, { -ps * 2 - 1.1f, 2.9f, ps * 1 } });
	cubePoints.push_back({ 0, 0, 11, { -ps * 2 - 1.1f, 2.9f, ps * 2 } });

	for (size_t i = 0; i < cubePoints.size(); ++i) {
		cubeGlobalPoints[cubePoints[i].globalId] = cubePoints[i].position;
	}

	std::vector<mat4> poses;
	std::vector<int> viewImageIds;
	std::vector<std::vector<cv::Point2f>> viewImagePoints;
	std::vector<std::vector<int>> viewImagePointIds;
	std::vector<int> poseOwner;
	int totalImagePointCount = 0;

	for (int hawkIter = 0; hawkIter < 2; ++hawkIter) {
		// Find a pose for each calibration sample.
		for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
			//std::cout << "Sample " << sampleIter << ":\n";

			// Combine all boards into a set of image points and 3d target local points.
			std::vector<cv::Point2f> imagePoints;
			std::vector<cv::Point3f> worldPoints;
			std::vector<int> imagePointIds;

			std::vector<ldiCharucoBoard>* boards = &Job->samples[sampleIter].cubes[hawkIter].boards;

			for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
				ldiCharucoBoard* board = &(*boards)[boardIter];

				//std::cout << "    Board " << boardIter << " (" << board->id << "):\n";

				for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
					ldiCharucoCorner* corner = &board->corners[cornerIter];

					//int cornerGlobalId = (board->id * 25) + corner->id;
					int cornerGlobalId = (board->id * 9) + corner->id;
					vec3 targetPoint = cubeGlobalPoints[cornerGlobalId];

					// NOTE: Basic tests have shown that the SSBA utility considers 0,0 to be corner of a pixel.
					imagePoints.push_back(cv::Point2f(corner->position.x, corner->position.y));
					worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
					imagePointIds.push_back(cornerGlobalId);

					//std::cout << "        Corner " << cornerIter << " (" << cornerGlobalId << ") " << corner->position.x << ", " << corner->position.y << " " << targetPoint.x << ", " << targetPoint.y << ", " << targetPoint.z << "\n";
				}
			}

			if (imagePoints.size() >= 6) {
				mat4 pose;

				std::cout << "Find pose - Sample: " << sampleIter << " :\n";
				if (computerVisionFindGeneralPose(&Job->startCamMat[hawkIter], &Job->startDistMat[hawkIter], &imagePoints, &worldPoints, &pose)) {
					//std::cout << "Pose:\n" << GetMat4DebugString(&pose);
					poses.push_back(pose);
					viewImageIds.push_back(sampleIter + hawkIter * 1000);
					viewImagePoints.push_back(imagePoints);
					viewImagePointIds.push_back(imagePointIds);
					totalImagePointCount += (int)imagePoints.size();
					poseOwner.push_back(hawkIter);
				}
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
	fprintf(f, "%d %d %d\n", cubePoints.size(), poses.size(), totalImagePointCount);

	// Camera intrinsics.
	for (size_t i = 0; i < poseOwner.size(); ++i) {
		int hawkId = poseOwner[i];
		cv::Mat calibCameraMatrix = Job->startCamMat[hawkId];
		cv::Mat calibCameraDist = Job->startDistMat[hawkId];

		fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
			calibCameraMatrix.at<double>(0), calibCameraMatrix.at<double>(1), calibCameraMatrix.at<double>(2), calibCameraMatrix.at<double>(4), calibCameraMatrix.at<double>(5),
			calibCameraDist.at<double>(0), calibCameraDist.at<double>(1), calibCameraDist.at<double>(2), calibCameraDist.at<double>(3));
	}

	// 3D points.
	for (size_t pointIter = 0; pointIter < cubePoints.size(); ++pointIter) {
		int pointId = cubePoints[pointIter].globalId;
		vec3 point = cubePoints[pointIter].position;
		fprintf(f, "%d %f %f %f\n", pointId, point.x, point.y, point.z);
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
		fprintf(f, "%f", camRt[3][2]);

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

	// Calibration mode: metric, focal, principal, radial, tangential.
	//char args[] = "\"BundleCommon.exe\" ../../bin/cache/ssba_input.txt metric";
	char args[] = "\"BundleVarying.exe\" ../../bin/cache/ssba_input.txt metric";

	CreateProcessA(
		"../../assets/bin/BundleVarying.exe",
		//"../../assets/bin/BundleCommon.exe",
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

bool computerVisionBundleAdjustStereoBothLoad(ldiCalibrationJob* Job) {
	FILE* file;
	fopen_s(&file, "../cache/refined.txt", "r");

	float ci[9];
	int modelPointCount = 0;
	int viewCount = 0;
	int imagePointCount = 0;

	fscanf_s(file, "%d %d %d\r\n", &modelPointCount, &viewCount, &imagePointCount);
	std::cout << "Model point count: " << modelPointCount << " View count: " << viewCount << " Image point count: " << imagePointCount << "\n";

	for (int i = 0; i < viewCount; ++i) {
		fscanf_s(file, "%f %f %f %f %f %f %f %f %f\r\n", &ci[0], &ci[1], &ci[2], &ci[3], &ci[4], &ci[5], &ci[6], &ci[7], &ci[8]);
	}

	Job->baIndvCubePoints.clear();

	for (int i = 0; i < modelPointCount; ++i) {
		vec3 position;
		int pointId;

		fscanf_s(file, "%d %f %f %f\r\n", &pointId, &position[0], &position[1], &position[2]);
		//std::cout << pointId << " " << position.x << ", " << position.y << ", " << position.z << "\n";

		Job->baIndvCubePoints.push_back(position);
	}

	Job->baViews.clear();

	int maxViewId = 0;

	for (int i = 0; i < viewCount; ++i) {
		int viewId;
		mat4 viewMat = glm::identity<mat4>();

		fscanf_s(file, "%d %f %f %f %f %f %f %f %f %f %f %f %f\r\n", &viewId,
			&viewMat[0][0], &viewMat[1][0], &viewMat[2][0], &viewMat[3][0],
			&viewMat[0][1], &viewMat[1][1], &viewMat[2][1], &viewMat[3][1],
			&viewMat[0][2], &viewMat[1][2], &viewMat[2][2], &viewMat[3][2]);


		viewMat[0][2] = -viewMat[0][2];
		viewMat[1][2] = -viewMat[1][2];
		viewMat[2][2] = -viewMat[2][2];
		viewMat[3][2] = -viewMat[3][2];

		std::cout << "View " << viewId << "\n" << GetMat4DebugString(&viewMat);

		Job->baViews.push_back(viewMat);
		Job->baViewIds.push_back(viewId);

		viewId = viewId % 1000;

		if (viewId > maxViewId) {
			maxViewId = viewId;
		}
	}

	fclose(file);

	std::vector<std::vector<int>> pairViewIds;
	pairViewIds.resize(maxViewId + 1);
	
	// Get distances between stereo cams.
	for (size_t i = 0; i < Job->baViewIds.size(); ++i) {
		int viewId = Job->baViewIds[i] % 1000;

		pairViewIds[viewId].push_back((int)i);
	}

	Job->baStereoPairs.clear();

	for (size_t i = 0; i < pairViewIds.size(); ++i) {
		if (pairViewIds[i].size() == 2) {
			mat4 v0 = glm::inverse(Job->baViews[pairViewIds[i][0]]);
			mat4 v1 = glm::inverse(Job->baViews[pairViewIds[i][1]]);

			vec3 pos0 = v0 * vec4(0, 0, 0, 1.0f);
			vec3 pos1 = v1 * vec4(0, 0, 0, 1.0f);

			float dist = glm::distance(pos0, pos1);

			std::cout << "Found match: " << pairViewIds[i][0] << " and " << pairViewIds[i][1] << " " << dist << "\n";
			//std::cout << pos0.x << ", " << pos0.y << ", " << pos0.z << " : "  << pos1.x << ", " << pos1.y << ", " << pos1.z << "\n";

			ldiStereoPair pair = {};
			pair.cam0world = v0;
			pair.cam1world = v1;
			pair.sampleId = i;

			Job->baStereoPairs.push_back(pair);
		}
	}
	
	vec3 centroid0Pos(0.0f, 0.0f, 0.0f);
	vec3 centroid0X(0.0f, 0.0f, 0.0f);
	vec3 centroid0Y(0.0f, 0.0f, 0.0f);
	vec3 centroid0Z(0.0f, 0.0f, 0.0f);

	vec3 centroid1Pos(0.0f, 0.0f, 0.0f);
	vec3 centroid1X(0.0f, 0.0f, 0.0f);
	vec3 centroid1Y(0.0f, 0.0f, 0.0f);
	vec3 centroid1Z(0.0f, 0.0f, 0.0f);

	for (size_t i = 0; i < Job->baStereoPairs.size(); ++i) {
		ldiStereoPair* pair = &Job->baStereoPairs[i];

		centroid0Pos += vec3(pair->cam0world[3]);
		centroid0X = vec3(pair->cam0world[0]);
		centroid0Y = vec3(pair->cam0world[1]);
		centroid0Z = vec3(pair->cam0world[2]);

		centroid1Pos += vec3(pair->cam1world[3]);
		centroid1X = vec3(pair->cam1world[0]);
		centroid1Y = vec3(pair->cam1world[1]);
		centroid1Z = vec3(pair->cam1world[2]);
	}

	float count = (float)Job->baStereoPairs.size();

	centroid0Pos /= count;
	centroid0X /= count;
	centroid0Y /= count;
	centroid0Z /= count;
	centroid1Pos /= count;
	centroid1X /= count;
	centroid1Y /= count;
	centroid1Z /= count;

	// Shift averaged stereo pair.
	vec3 centroidMidPos = (centroid0Pos + centroid1Pos) / 2.0f;
	vec3 pair0midpos = (vec3(Job->baStereoPairs[0].cam0world[3]) + vec3(Job->baStereoPairs[0].cam1world[3])) / 2.0f;
	vec3 deltaPos = centroidMidPos - pair0midpos;
	std::cout << "Delta pos: " << pair0midpos.x << ", " << pair0midpos.y << "\n";

	// TODO: Orthonormalize the averaged basis.
	Job->baStereoCamWorld[0] = glm::identity<mat4>();
	Job->baStereoCamWorld[0][0] = vec4(glm::normalize(centroid0X), 0.0f);
	Job->baStereoCamWorld[0][1] = vec4(glm::normalize(centroid0Y), 0.0f);
	Job->baStereoCamWorld[0][2] = vec4(glm::normalize(centroid0Z), 0.0f);
	Job->baStereoCamWorld[0][3] = vec4(centroid0Pos - deltaPos, 1.0f);

	Job->baStereoCamWorld[1] = glm::identity<mat4>();
	Job->baStereoCamWorld[1][0] = vec4(glm::normalize(centroid1X), 0.0f);
	Job->baStereoCamWorld[1][1] = vec4(glm::normalize(centroid1Y), 0.0f);
	Job->baStereoCamWorld[1][2] = vec4(glm::normalize(centroid1Z), 0.0f);
	Job->baStereoCamWorld[1][3] = vec4(centroid1Pos - deltaPos, 1.0f);

	return true;
}

void computerVisionStereoCameraCalibrate(ldiCalibrationJob* Job) {
	std::cout << "Starting stereo calibration.\n";

	std::vector<cv::Point3f> tempModelPoints(18, cv::Point3f(0, 0, 0));
	
	// TODO: This is the size reference, make sure is correct dimensions.
	// NOTE: Refined bundle adjust points. Why is Z backwards?
	tempModelPoints[0] = cv::Point3f(0.00277832, -0.000379695, 0.00827281);
	tempModelPoints[1] = cv::Point3f(0.00242818, 0.886975, -0.00139809);
	tempModelPoints[2] = cv::Point3f(-0.00199425, 1.77352, -0.00676404);
	tempModelPoints[3] = cv::Point3f(0.00680608, 0.0066398, 0.898923);
	tempModelPoints[4] = cv::Point3f(0.0043463, 0.891605, 0.894385);
	tempModelPoints[5] = cv::Point3f(0.00149548, 1.77892, 0.887999);
	tempModelPoints[6] = cv::Point3f(0.00808172, 0.0108319, 1.79622);
	tempModelPoints[7] = cv::Point3f(0.00671072, 0.896781, 1.7891);
	tempModelPoints[8] = cv::Point3f(0.00236594, 1.78382, 1.78409);
	tempModelPoints[9] = cv::Point3f(-2.88828, 2.89365, 0.00533009);
	tempModelPoints[10] = cv::Point3f(-2.89563, 2.90259, 0.897714);
	tempModelPoints[11] = cv::Point3f(-2.90182, 2.91324, 1.79047);
	tempModelPoints[12] = cv::Point3f(-1.9996, 2.89199, 0.0115555);
	tempModelPoints[13] = cv::Point3f(-2.00448, 2.90196, 0.906308);
	tempModelPoints[14] = cv::Point3f(-2.01777, 2.91062, 1.79906);
	tempModelPoints[15] = cv::Point3f(-1.11024, 2.89208, 0.0205293);
	tempModelPoints[16] = cv::Point3f(-1.11899, 2.90095, 0.911953);
	tempModelPoints[17] = cv::Point3f(-1.12496, 2.90969, 1.80808);

	for (size_t i = 0; i < tempModelPoints.size(); ++i) {
		tempModelPoints[i].z = -tempModelPoints[i].z;
		//Job->baIndvCubePoints.push_back(vec3(tempModelPoints[i].x, tempModelPoints[i].y, tempModelPoints[i].z));
	}

	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> imagePoints[2];
	std::vector<int> stereoSampleId;

	// Find matching points in both views.
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
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

		// Compare image points from boths views.
		for (size_t pointIter0 = 0; pointIter0 < tempImagePoints[0].size(); ++pointIter0) {
			int point0Id = tempGlobalIds[0][pointIter0];

			for (size_t pointIter1 = 0; pointIter1 < tempImagePoints[1].size(); ++pointIter1) {
				int point1Id = tempGlobalIds[1][pointIter1];

				if (point0Id == point1Id) {
					appendObjectPoints.push_back(tempModelPoints[point0Id]);
					appendImagePoints[0].push_back(tempImagePoints[0][pointIter0]);
					appendImagePoints[1].push_back(tempImagePoints[1][pointIter1]);

					break;
				}
			}
		}

		if (appendObjectPoints.size() > 0) {
			objectPoints.push_back(appendObjectPoints);
			imagePoints[0].push_back(appendImagePoints[0]);
			imagePoints[1].push_back(appendImagePoints[1]);
			stereoSampleId.push_back(sampleIter);
		}
	}

	cv::Mat camMat[2];
	camMat[0] = Job->startCamMat[0].clone();
	camMat[1] = Job->startCamMat[1].clone();

	cv::Mat distMat[2];
	distMat[0] = Job->startDistMat[0].clone();
	distMat[1] = Job->startDistMat[1].clone();

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
	double error = cv::stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1], camMat[0], distMat[0], camMat[1], distMat[1], cv::Size(imgWidth, imgHeight), r, t, e, f, rvecs, tvecs, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS, criteria);

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

	Job->baStereoCamWorld[0] = glm::identity<mat4>();
	Job->baStereoCamWorld[1] = glm::inverse(camRt);

	float dist = glm::distance(vec3(0, 0, 0.0f), vec3(t.at<double>(0), t.at<double>(1), t.at<double>(2)));
	std::cout << "Dist: " << dist << "\n";

	std::cout << "New cam 0 mats: \n";
	std::cout << camMat[0] << "\n";
	std::cout << distMat[0] << "\n";
	std::cout << "New cam 1 mats: \n";
	std::cout << camMat[1] << "\n";
	std::cout << distMat[1] << "\n";;

	Job->camMatrix[0] = camMat[0].clone();
	Job->camDist[0] = distMat[0].clone();
	Job->camMatrix[1] = camMat[1].clone();
	Job->camDist[1] = distMat[1].clone();

	for (size_t i = 0; i < tempModelPoints.size(); ++i) {
		Job->stCubePoints.push_back(vec3(tempModelPoints[i].x, tempModelPoints[i].y, tempModelPoints[i].z));
	}

	std::vector<mat4> poses;

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
		poses.push_back(pose);
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

	// TODO: Account for view error to decide if view should be included.
	for (size_t sampleIter = 0; sampleIter < rvecs.size(); ++sampleIter) {
		ldiCalibStereoSample* sample = &Job->samples[stereoSampleId[sampleIter]];
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
			pose.pose = poses[sampleIter];
			columnX[columnId].poses.push_back(pose);

			std::cout << "(X) View: " << sampleIter << " Sample: " << stereoSampleId[sampleIter] <<  " Mech: " << x << "," << y << "," << z << "\n";
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
			pose.pose = poses[sampleIter];
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
			pose.pose = poses[sampleIter];
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

					std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
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

					std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
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

					std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
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
	// Calculate calib volume to world basis.
	//----------------------------------------------------------------------------------------------------
	mat4 worldToVolume = glm::identity<mat4>();

	worldToVolume[0] = vec4(Job->basisX, 0.0f);
	worldToVolume[1] = vec4(Job->basisY, 0.0f);
	worldToVolume[2] = vec4(Job->basisZ, 0.0f);

	mat4 volumeToWorld = glm::inverse(worldToVolume);
	mat4 volumeRealign = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
	volumeToWorld = volumeRealign * volumeToWorld;

	volumeRealign = glm::translate(mat4(1.0f), vec3(0.0f, 14.0f, 0.0f));
	volumeToWorld = volumeRealign * volumeToWorld;

	//----------------------------------------------------------------------------------------------------
	// Move all volume elements into world space.
	//----------------------------------------------------------------------------------------------------
	{
		// Move axis directions.
		Job->axisX.direction = glm::normalize(volumeToWorld * vec4(Job->axisX.direction, 0.0f));
		Job->axisY.direction = glm::normalize(volumeToWorld * vec4(Job->axisY.direction, 0.0f));
		Job->axisZ.direction = glm::normalize(volumeToWorld * vec4(Job->axisZ.direction, 0.0f));

		// Move mass model.
		/*for (size_t i = 0; i < Job->massModelBundleAdjustPointIds.size(); ++i) {
			vec3 point = Job->massModelTriangulatedPointsBundleAdjust[i];
			point = volumeToWorld * vec4(point, 1.0f);
			Job->massModelTriangulatedPointsBundleAdjust[i] = point;
		}*/

		// Move cameras to world space.
		Job->camVolumeMat[0] = volumeToWorld * Job->baStereoCamWorld[0];
		Job->camVolumeMat[1] = volumeToWorld * Job->baStereoCamWorld[1];

		// Normalize scale.
		Job->camVolumeMat[0][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][0])), 0.0f);
		Job->camVolumeMat[0][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][1])), 0.0f);
		Job->camVolumeMat[0][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[0][2])), 0.0f);

		Job->camVolumeMat[1][0] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][0])), 0.0f);
		Job->camVolumeMat[1][1] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][1])), 0.0f);
		Job->camVolumeMat[1][2] = vec4(glm::normalize(vec3(Job->camVolumeMat[1][2])), 0.0f);

		// Move cube centroids.
		for (size_t i = 0; i < Job->stCubeWorlds.size(); ++i) {
			Job->stCubeWorlds[i] = volumeToWorld * Job->stCubeWorlds[i];

			//Job->stCubeWorlds[i][3] = vec4(0,0,0,1.0f);
		}

		Job->cubePointCentroids.clear();
		for (size_t i = 0; i < tempModelPoints.size(); ++i) {
			vec3 point = vec3(tempModelPoints[i].x, tempModelPoints[i].y, tempModelPoints[i].z);
			point = poses[0] * vec4(point.x, point.y, point.z, 1.0);

			point = volumeToWorld * vec4(point.x, point.y, point.z, 1.0);

			Job->cubePointCentroids.push_back(point);
		}
	}
}

// https://github.com/Tetragramm/opencv_contrib/blob/master/modules/mapping3d/include/opencv2/mapping3d.hpp#L131
// https://scipy-cookbook.readthedocs.io/items/bundle_adjustment.html
// https://gist.github.com/davegreenwood/4434757e97c518890c91b3d0fd9194bd
// https://github.com/mprib/pyxy3d/blob/main/pyxy3d/calibration/capture_volume/capture_volume.py