#pragma once

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

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

struct ldiCameraCalibrationContext {
	ldiImage* calibrationTargetImage;
};

cv::Ptr<cv::aruco::Dictionary> _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

ldiImage _calibrationTargetImage;
float _calibrationTargetFullWidthCm;
float _calibrationTargetFullHeightCm;
std::vector<vec3> _calibrationTargetLocalPoints;

void computerVisionFitPlane(std::vector<vec3>& Points, ldiPlane* ResultPlane);

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

	float squareSize = 15.0;
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
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);
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
			cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 1.0 * newScale, 0.7 * newScale, _dictionary);
			_charucoBoards.push_back(board);

			// NOTE: Shift ids.
			int offset = i * board->ids.size();

			for (int j = 0; j < board->ids.size(); ++j) {
				board->ids[j] = offset + j;
			}

			// NOTE: Image output.
			if (Output) {
				cv::Mat markerImage;
				board->draw(cv::Size(1000, 1000), markerImage, 50, 1);

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
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		// TODO: Enable subpixel corner refinement. WTF is this doing?
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		parameters->minMarkerPerimeterRate = 0.015;
		
		double t0 = _getTime(AppContext);
		cv::aruco::detectMarkers(image, _dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
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
			for (int i = 0; i < 6; ++i) {
				std::vector<cv::Point2f> charucoCorners;
				std::vector<int> charucoIds;
				cv::aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners, charucoIds);

				// Board is valid if it has at least one corner.
				if (charucoIds.size() > 0) {
					ldiCharucoBoard board = {};
					board.id = i;
					board.localMat = false;
					board.rejected = true;

					cv::Mat rvec;
					cv::Mat tvec;
					if (cv::aruco::estimatePoseCharucoBoard(charucoCorners, charucoIds, _charucoBoards[i], *CameraMatrix, *CameraDist, rvec, tvec)) {
						//std::cout << "Found board pose: " << i << rvec << "\n";
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
						board.charucoEstimatedBoardAngle = glm::dot(-camDir, centerNormal);

						if (board.charucoEstimatedBoardAngle < 0.3f) {
							board.rejected = true;
						}

						cv::projectPoints(points, rvecTemp, tvecTemp, *CameraMatrix, *CameraDist, imagePoints);
						
						//points.push_back(cv::Point3f(cubeHSize, cubeHSize, 0.0f));
						//cv::projectPoints(points, rvec, tvec, *CameraMatrix, *CameraDist, imagePoints);

						board.charucoEstimatedImageCenter = vec2(imagePoints[0].x, imagePoints[0].y);
						board.charucoEstimatedImageNormal = vec2(imagePoints[1].x, imagePoints[1].y);
					}

					for (int j = 0; j < charucoIds.size(); ++j) {
						ldiCharucoCorner corner;
						corner.id = charucoIds[j];
						corner.position = vec2(charucoCorners[j].x, charucoCorners[j].y);
						board.corners.push_back(corner);
					}

					if (!board.rejected) {
						Results->boards.push_back(board);
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
				// TODO: The corner positions are 0,0 at center pixel. What does the calibration algo expect? Does it matter?
				cornerPositions.push_back(cv::Point2f(corner.position.x + 0.5f, corner.position.y + 0.5f));
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
	}

	if (solutionCount > 0) {
		return true;
	}

	return false;
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

	// Debug.Log(Normal);
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
	
	int cornerSize = (int)_charucoBoards[0]->chessboardCorners.size();

	// Bundle adjust needs:
	// Pose estimation for each sample.
	// Image points for each sample.
	// Corner global IDs for each sample.
	// Sample ID.

	std::vector<mat4> poses;
	std::vector<int> viewImageIds;
	std::vector<std::vector<cv::Point2f>> viewImagePoints;
	std::vector<std::vector<int>> viewImagePointIds;

	float squareSize = 15.0;
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
			imagePoints.push_back(cv::Point2f(sample->imagePoints[pointIter].x + 0.5f, sample->imagePoints[pointIter].y + 0.5f));
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