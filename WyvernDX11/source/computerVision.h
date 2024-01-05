#pragma once

#include <fstream>

void computerVisionFitPlane(std::vector<vec3>& Points, ldiPlane* ResultPlane);

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
	int phase;
	int X;
	int Y;
	int Z;
	int C;
	int A;
	bool imageLoaded = false;
	ldiImage frames[2];
	ldiCharucoResults cubes[2];
};

struct ldiCalibrationJob {
	std::vector<ldiCalibStereoSample> samples;

	// Default intrinsics.
	cv::Mat defaultCamMat[2];
	cv::Mat defaultCamDist[2];

	// Most up to date camera instrinsics.
	cv::Mat refinedCamMat[2];
	cv::Mat refinedCamDist[2];

	//----------------------------------------------------------------------------------------------------
	// Stereo calibration.
	//----------------------------------------------------------------------------------------------------
	bool stereoCalibrated;
	std::vector<int> stPoseToSampleIds;
	std::vector<mat4> stCubeWorlds;
	mat4 stStereoCamWorld[2];

	ldiCalibCube opCube;

	//----------------------------------------------------------------------------------------------------
	// Calib volume metrics.
	//----------------------------------------------------------------------------------------------------
	bool metricsCalculated;
	std::vector<vec3> stBasisXPoints;
	std::vector<vec3> stBasisYPoints;
	std::vector<vec3> stBasisZPoints;

	vec3 stVolumeCenter;

	ldiLine axisX;
	ldiLine axisY;
	ldiLine axisZ;
	ldiLine axisC;
	ldiLine axisA;

	vec3 basisX;
	vec3 basisY;
	vec3 basisZ;

	glm::f64vec3 stepsToCm;

	std::vector<mat4> cubeWorlds;
	mat4 camVolumeMat[2];

	//----------------------------------------------------------------------------------------------------
	// Scanner calibration.
	//----------------------------------------------------------------------------------------------------
	bool scannerCalibrated;
	std::vector<ldiCalibStereoSample> scanSamples;
	ldiPlane scanPlane;

	//----------------------------------------------------------------------------------------------------
	// Galvo calibration.
	//----------------------------------------------------------------------------------------------------
	bool galvoCalibrated;

	//----------------------------------------------------------------------------------------------------
	// Other data - not serialized.
	//----------------------------------------------------------------------------------------------------
	std::vector<vec3> axisCPoints;
	std::vector<vec3> axisAPoints;
	
	std::vector<std::vector<vec2>> scanPoints[2];
	std::vector<ldiLine> scanRays[2];
	std::vector<vec3> scanWorldPoints[2];
};

struct ldiCalibrationContext {
	ldiCalibrationJob calibJob;
};

auto _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

ldiImage _calibrationTargetImage;
float _calibrationTargetFullWidthCm;
float _calibrationTargetFullHeightCm;
std::vector<vec3> _calibrationTargetLocalPoints;

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

bool cameraCalibFindChessboard(ldiApp* AppContext, ldiImage Image, ldiCameraCalibSample* Sample) {
	int offset = 1;

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<cv::Point2f> corners;

		double t0 = getTime();
		bool foundCorners = cv::findChessboardCorners(image, cv::Size(9, 6), corners);
		t0 = getTime() - t0;
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

		double t0 = getTime();
		bool foundCorners = cv::findCirclesGrid(image, cv::Size(4, 11), corners, cv::CALIB_CB_ASYMMETRIC_GRID, blobDetector);
		t0 = getTime() - t0;
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

	double t0 = getTime();
	// cv::CALIB_USE_INTRINSIC_GUESS
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

	double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, 0, criteria);
	t0 = getTime() - t0;
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

	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 2660;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 3280.0 / 2.0;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 2464.0 / 2.0;
	cameraMatrix.at<double>(1, 2) = 2660;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	cv::Mat stdDevIntrinsics;
	cv::Mat stdDevExtrinsics;
	cv::Mat perViewErrors;

	double t0 = getTime();

	//cv::CALIB_USE_INTRINSIC_GUESS
	cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 1000, DBL_EPSILON);
	//double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, 0, criteria);
	double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_FIX_K3, criteria);
	//double rms = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS, criteria);
	t0 = getTime() - t0;
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
		//std::cout << "rotMat: " << GetStr(&rotMat);
		//std::cout << "camLocalMat: " << GetStr(&Samples[i].camLocalMat);

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

			// NOTE: We only use legacy patterns becuase I printed them out before they were considered "legacy".
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

void computerVisionFindCharuco(ldiImage Image, ldiCharucoResults* Results, cv::Mat* CameraMatrix, cv::Mat* CameraDist) {
	int offset = 1;

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedMarkers;
		auto parameters = cv::aruco::DetectorParameters();
		parameters.minMarkerPerimeterRate = 0.015;
		// TODO: Check if this is doing anything.
		parameters.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

		double t0 = getTime();
		
		cv::aruco::ArucoDetector arucoDetector(_dictionary, parameters);
		arucoDetector.detectMarkers(image, markerCorners, markerIds, rejectedMarkers);
		// TODO: Use refine strategy to detect more markers.
		//cv::Ptr<cv::aruco::Board> board = charucoBoard.staticCast<aruco::Board>();
		//aruco::refineDetectedMarkers(image, board, markerCorners, markerIds, rejectedCandidates);
		
		t0 = getTime() - t0;
		std::cout << "Detect markers: " << (t0 * 1000.0) << " ms\n";

		Results->markers.clear();
		Results->boards.clear();
		Results->rejectedMarkers.clear();

		for (size_t i = 0; i < markerCorners.size(); ++i) {
			ldiCharucoMarker marker;
			marker.id = markerIds[i];

			for (int j = 0; j < 4; ++j) {
				marker.corners[j] = vec2(markerCorners[i][j].x, markerCorners[i][j].y);
			}

			Results->markers.push_back(marker);
		}

		for (size_t i = 0; i < rejectedMarkers.size(); ++i) {
			ldiCharucoMarker marker;
			marker.id = -1;

			for (int j = 0; j < 4; ++j) {
				marker.corners[j] = vec2(rejectedMarkers[i][j].x, rejectedMarkers[i][j].y);
			}

			Results->rejectedMarkers.push_back(marker);
		}

		// Interpolate charuco corners for each possible board.
		if (markerCorners.size() > 0) {
			//std::cout << "Markers: " << markerCorners.size() << "\n";

			for (int i = 0; i < 6; ++i) {
				std::vector<cv::Point2f> charucoCorners;
				std::vector<int> charucoIds;

				cv::aruco::CharucoDetector charucoDetector(*(_charucoBoards[i]).get(), cv::aruco::CharucoParameters(), parameters);
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

					//std::cout << "Check board pose:\n";
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

						//cv::Mat rvecTemp = cv::Mat::zeros(3, 1, CV_64F);
						//cv::Mat tvecTemp = cv::Mat::zeros(3, 1, CV_64F);
						std::vector<cv::Point3f> points;
						std::vector<cv::Point2f> imagePoints;
												
						const float cubeOffset = 0.0f;
						const float cubeSize = 4.0f;
						const float cubeHSize = 2.0f;

						points.push_back(cv::Point3f(cubeOffset, cubeOffset, 0.0f));
						points.push_back(cv::Point3f(cubeSize + cubeOffset, cubeOffset, 0.0f));
						points.push_back(cv::Point3f(cubeSize + cubeOffset, cubeSize + cubeOffset, 0.0f));
						points.push_back(cv::Point3f(cubeOffset, cubeSize + cubeOffset, 0.0f));

						cv::projectPoints(points, rvec, tvec, *CameraMatrix, *CameraDist, imagePoints);
						for (size_t pointIter = 0; pointIter < imagePoints.size(); ++pointIter) {
							board.outline.push_back(toVec2(imagePoints[pointIter]));
						}

						points.clear();
						imagePoints.clear();

						vec3 centerPoint = board.camLocalCharucoEstimatedMat * vec4(cubeHSize, cubeHSize, 0.0f, 1.0f);
						//points.push_back(cv::Point3f(-centerPoint.x, centerPoint.y, centerPoint.z));

						vec3 centerNormalPoint = board.camLocalCharucoEstimatedMat * vec4(cubeHSize, cubeHSize, 1.0f, 1.0f);
						//points.push_back(cv::Point3f(-centerNormalPoint.x, centerNormalPoint.y, centerNormalPoint.z));

						vec3 centerNormal = glm::normalize(centerNormalPoint - centerPoint);
						vec3 camDir = glm::normalize(centerPoint);
						board.charucoEstimatedBoardAngle = glm::dot(camDir, centerNormal);

						//std::cout << "Dot: " << board.charucoEstimatedBoardAngle << "\n";

						if (board.charucoEstimatedBoardAngle < 0.38f) {
							board.rejected = true;
						}

						auto worldCorners = _charucoBoards[i]->getChessboardCorners();

						for (size_t cornerIter = 0; cornerIter < charucoIds.size(); ++cornerIter) {
							int cornerId = charucoIds[cornerIter];
							points.push_back(worldCorners[cornerId]);
						}	

						cv::projectPoints(points, rvec, tvec, *CameraMatrix, *CameraDist, imagePoints);
						
						//points.push_back(cv::Point3f(cubeHSize, cubeHSize, 0.0f));
						//cv::projectPoints(points, rvec, tvec, *CameraMatrix, *CameraDist, imagePoints);

						//board.charucoEstimatedImageCenter = vec2(imagePoints[0].x, imagePoints[0].y);
						//board.charucoEstimatedImageNormal = vec2(imagePoints[1].x, imagePoints[1].y);

						// Check for outliers.
						// ...
						for (size_t cornerIter = 0; cornerIter < imagePoints.size(); ++cornerIter) {
							board.reprojectdCorners.push_back(toVec2(imagePoints[cornerIter]));
						}
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
						Results->rejectedBoards.push_back(board);
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

		/*t0 = getTime() - t0;
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

	double t0 = getTime();
	double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6 | cv::CALIB_ZERO_TANGENT_DIST | cv::CALIB_FIX_FOCAL_LENGTH | cv::CALIB_USE_INTRINSIC_GUESS | cv::CALIB_FIX_PRINCIPAL_POINT);
	//double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS,
		//cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON));
	t0 = getTime() - t0;
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
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;
	std::vector<float> rms;

	int solutionCount = cv::solvePnPGeneric(*WorldPoints, *ImagePoints, *CameraMatrix, *DistCoeffs, rvecs, tvecs, false, cv::SOLVEPNP_ITERATIVE, cv::noArray(), cv::noArray(), rms);
	//std::cout << "  Solutions: " << solutionCount << "\n";
	// TODO: Why do we set solutionCount to this?
	solutionCount = rvecs.size();
	//std::cout << "  Solutions: " << solutionCount << "\n";

	for (int i = 0; i < solutionCount; ++i) {
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

		//std::cout << "  Found pose (" << i << "): " << rms[i] << "\n";

		if (rms[i] > 3.0) {
			return false;
		}
	}

	if (solutionCount > 0) {
		return true;
	}

	return false;
}

ldiLine computerVisionFitLine(std::vector<vec3>& Points) {
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

	ldiLine result;
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
	double pointCount = Points.size();

	vec3d sum(0.0, 0.0, 0.0);
	for (int i = 0; i < pointCount; ++i) {
		sum += Points[i];
	}

	vec3d centroid = sum * (1.0 / pointCount);

	double xx = 0.0;
	double xy = 0.0;
	double xz = 0.0;
	double yy = 0.0;
	double yz = 0.0;
	double zz = 0.0;

	for (int i = 0; i < pointCount; ++i) {
		vec3d r = vec3d(Points[i]) - centroid;
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

	vec3d weightedDir(0.0, 0.0, 0.0);

	{
		double detX = yy * zz - yz * yz;
		vec3d axisDir = vec3d(
			detX,
			xz * yz - xy * zz,
			xy * yz - xz * yy
		);

		double weight = detX * detX;

		if (glm::dot(weightedDir, axisDir) < 0.0) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	{
		double detY = xx * zz - xz * xz;
		vec3d axisDir = vec3d(
			xz * yz - xy * zz,
			detY,
			xy * xz - yz * xx
		);

		double weight = detY * detY;

		if (glm::dot(weightedDir, axisDir) < 0.0) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	{
		double detZ = xx * yy - xy * xy;
		vec3d axisDir = vec3d(
			xy * yz - xz * yy,
			xy * xz - yz * xx,
			detZ
		);

		double weight = detZ * detZ;

		if (glm::dot(weightedDir, axisDir) < 0.0) {
			weight = -weight;
		}

		weightedDir += axisDir * weight;
	}

	vec3d normal = glm::normalize(weightedDir);

	ResultPlane->point = centroid;
	ResultPlane->normal = normal;

	if (glm::length(normal) == 0.0) {
		ResultPlane->point = vec3Zero;
		ResultPlane->normal = vec3Up;
	}
}

ldiPlane computerVisionFitPlane2(std::vector<vec3d>& Points) {
	std::vector<cv::Point3d> points;

	for (size_t i = 0; i < Points.size(); ++i) {
		points.push_back(cv::Point3d(Points[i].x, Points[i].y, Points[i].z));
	}

	int pointCount = (int)Points.size();
	
	cv::Point3d centroid(0, 0, 0);

	for (int i = 0; i < pointCount; ++i) {
		centroid += points[i];
	}

	centroid /= (double)pointCount;
	
	cv::Mat mP = cv::Mat(pointCount, 3, CV_64F);

	for (int i = 0; i < pointCount; ++i) {
		points[i] -= centroid;
		mP.at<double>(i, 0) = points[i].x;
		mP.at<double>(i, 1) = points[i].y;
		mP.at<double>(i, 2) = points[i].z;
	}

	cv::Mat w, u, vt;
	cv::SVD::compute(mP, w, u, vt);

	cv::Point3d pV(vt.at<double>(2, 0), vt.at<double>(2, 1), vt.at<double>(2, 2));
	
	vec3d normal = glm::normalize(toVec3(pV));

	ldiPlane result;
	result.point = toVec3(centroid);
	result.normal = normal;

	if (glm::length(normal) == 0.0) {
		result.point = vec3Zero;
		result.normal = vec3Up;
	}

	return result;
}

ldiCircle computerVisionFitCircle(std::vector<vec3d>& Points) {
	// Transform 3D points to 2D plane.
	ldiPlane basePlane = computerVisionFitPlane2(Points);
	mat4 planeBasis = planeGetBasis(basePlane);
	mat4 planeBasisInv = glm::inverse(planeBasis);
	std::vector<vec2> pointsOnPlane;

	// Project all points onto the plane.
	for (size_t i = 0; i < Points.size(); ++i) {
		vec3 proj = projectPointToPlane(Points[i], basePlane);
		vec3 planePos = planeBasisInv * vec4(proj, 1.0f);
		pointsOnPlane.push_back(vec2(planePos.x, planePos.y));
	}

	ldiCircle circle = circleFit(pointsOnPlane);

	// Transform 2D circle back to 3D plane.
	circle.normal = planeBasis * vec4(circle.normal, 0.0f);
	circle.origin = planeBasis * vec4(circle.origin, 1.0f);

	return circle;
}

std::vector<vec2> computerVisionFindScanLine(ldiImage Image) {
	std::vector<vec2> result;

	int mappingMin = 22;
	int mappingMax = 75;

	float diff = 255.0f / (mappingMax - mappingMin);
	for (int i = 0; i < Image.width * Image.height; ++i) {
		int v = Image.data[i];
		//v = (int)(GammaToLinear(v / 255.0) * 255.0);

		v = (v - mappingMin) * diff;

		if (v < 0) {
			v = 0;
		} else if (v > 255) {
			v = 255;
		}

		Image.data[i] = v;
	}

	int sigPeakMin = 50;

	struct ldiScanLineSignal {
		int x;
		int y0;
		int y1;
	};

	std::vector<vec4> tempSegs;

	// Process each column.
	for (int iX = 0; iX < Image.width; ++iX) {
		// Find all signals in this column.
		int sigState = 0;
		int sigStart = -1;

		for (int iY = 0; iY < Image.height; ++iY) {
			int v = Image.data[iY * Image.width + iX];

			if (sigState == 0) {
				// Looking for start of signal.
				if (v >= sigPeakMin) {
					sigState = 1;
					sigStart = iY;
				}
			} else if (sigState == 1) {
				// Looking for end of signal.
				if (v < sigPeakMin) {
					sigState = 0;

					tempSegs.push_back(vec4(iX, sigStart, iX, (iY - 1)));
				}
			}
		}
	}

	std::vector<vec4> segs;

	// Find extents of each signal.
	for (size_t i = 0; i < tempSegs.size(); ++i) {
		vec4 seg = tempSegs[i];
		vec4 final = seg;

		// Scan up to find break.
		int minY = 0;

		if (i != 0) {
			vec4 segUp = tempSegs[i - 1];

			if ((int)segUp.x == (int)seg.x) {
				minY = segUp.w + (seg.y - segUp.w) / 2;
			}
		}

		for (int iY = (int)seg.y; iY >= minY; --iY) {
			int v = Image.data[iY * Image.width + (int)seg.x];
			final.y = iY;

			if (v == 0) {
				break;
			}
		}

		// Scan down to find break.
		int maxY = Image.height - 1;

		if (i != tempSegs.size() - 1) {
			vec4 segDown = tempSegs[i + 1];

			if ((int)segDown.x == (int)seg.x) {
				maxY = seg.w + (segDown.y - seg.w) / 2;
			}
		}

		for (int iY = (int)seg.w; iY <= maxY; ++iY) {
			int v = Image.data[iY * Image.width + (int)seg.x];
			final.w = iY;

			if (v == 0) {
				break;
			}
		}

		segs.push_back(final);
	}

	// Find center point of each signal.
	for (size_t i = 0; i < segs.size(); ++i) {
		vec4 seg = segs[i];

		int x = (int)seg.x;
		int y0 = (int)seg.y;
		int y1 = (int)seg.w;

		float totalGravity = 0.0f;
		float finalY = 0.0f;

		for (int iY = y0; iY <= y1; ++iY) {
			float v = Image.data[iY * Image.width + x];
			totalGravity += v;
		}

		for (int iY = y0; iY <= y1; ++iY) {
			float v = Image.data[iY * Image.width + x];
			finalY += (iY + 0.5f) * (v / totalGravity);
		}

		result.push_back(vec2(x + 0.5f, finalY));
	}

	return result;
}

void computerVisionUndistortPoints(std::vector<vec2>& Points, cv::Mat CamMat, cv::Mat CamDist) {
	std::vector<cv::Point2f> distortedPoints(Points.size());
	std::vector<cv::Point2f> undistortedPoints;

	for (size_t pIter = 0; pIter < Points.size(); ++pIter) {
		distortedPoints[pIter] = toPoint2f(Points[pIter]);
	}

	cv::undistortPoints(distortedPoints, undistortedPoints, CamMat, CamDist, cv::noArray(), CamMat);

	for (size_t pIter = 0; pIter < Points.size(); ++pIter) {
		Points[pIter] = toVec2(undistortedPoints[pIter]);
	}
}