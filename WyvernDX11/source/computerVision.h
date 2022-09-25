#pragma once

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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
};

struct ldiCharucoResults {
	std::vector<ldiCharucoMarker> markers;
	std::vector<ldiCharucoBoard> boards;
};

struct ldiCalibPoint {
	int id;
	int boardId;
	int fullId;
	vec3 position;
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

cv::Ptr<cv::aruco::Dictionary> _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

void createCharucos(bool Output) {
	try {
		for (int i = 0; i < 6; ++i) {
			//	cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.009f, 0.006f, _dictionary);
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.9f, 0.6f, _dictionary);
			
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 0.9f, 0.6f, _dictionary);
			cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(6, 6, 1.0f, 0.7f, _dictionary);
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

void findCharuco(ldiImage Image, ldiApp* AppContext, ldiCharucoResults* Results) {
	int offset = 1;

	try {
		cv::Mat image(cv::Size(Image.width, Image.height), CV_8UC1, Image.data);
		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		// TODO: Enable subpixel corner refinement. WTF is this doing?
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
		
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
					ldiCharucoBoard board;
					board.id = i;

					for (int j = 0; j < charucoIds.size(); ++j) {
						ldiCharucoCorner corner;
						corner.id = charucoIds[j];
						corner.position = vec2(charucoCorners[j].x, charucoCorners[j].y);
						board.corners.push_back(corner);
					}

					Results->boards.push_back(board);
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

void _calibrateCameraCharuco(ldiApp* AppContext, std::vector<ldiCalibSample>* Samples, int ImageWidth, int ImageHeight) {
	// Input data is:
	// board count
	// for each board:
	//		marker count
	//		for each marker:
	//			marker id
	//			pos x, y



	std::vector<std::vector<int>> charucoIds;
	std::vector<std::vector<cv::Point2f>> charucoCorners;

	/*int offset = 1;
	int boardCount = 0;
	memcpy(&boardCount, Data + offset, 4); offset += 4;

	for (int i = 0; i < boardCount; ++i) {
		std::vector<int> ids;
		std::vector<cv::Point2f> corners;

		int markerCount = 0;
		memcpy(&markerCount, Data + offset, 4); offset += 4;

		for (int j = 0; j < markerCount; ++j) {
			int markerId;
			cv::Point2f markerPos;

			memcpy(&markerId, Data + offset, 4); offset += 4;
			memcpy(&markerPos, Data + offset, 8); offset += 8;

			ids.push_back(markerId);
			corners.push_back(markerPos);
		}

		charucoIds.push_back(ids);
		charucoCorners.push_back(corners);
	}*/

	int boardId = 0;

	for (size_t sampleIter = 0; sampleIter < Samples->size(); ++sampleIter) {
		for (size_t boardIter = 0; boardIter < (*Samples)[sampleIter].charucos.boards.size(); ++boardIter) {
			if ((*Samples)[sampleIter].charucos.boards[boardIter].corners.size() < 6) {
				continue;
			}

			++boardId;
			if (boardId == 10) {
				continue;
			}

			std::vector<int> cornerIds;
			std::vector<cv::Point2f> cornerPositions;

			for (size_t cornerIter = 0; cornerIter < (*Samples)[sampleIter].charucos.boards[boardIter].corners.size(); ++cornerIter) {
				ldiCharucoCorner corner = (*Samples)[sampleIter].charucos.boards[boardIter].corners[cornerIter];

				cornerIds.push_back(corner.id);
				// TODO: The corner positions are 0,0 at center pixel. What does the calibration algo expect? Does it matter?
				//cornerPositions.push_back(cv::Point2f(corner.position.x + 0.5f, corner.position.y + 0.5f));
				cornerPositions.push_back(cv::Point2f(corner.position.x + 0.5f, corner.position.y + 0.5f));

				std::cout << sampleIter << " " << boardIter << " " << corner.id << " " << corner.position.x << ", " << corner.position.y << "\n";
			}

			charucoIds.push_back(cornerIds);
			charucoCorners.push_back(cornerPositions);
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

	double t0 = _getTime(AppContext);

	cv::Mat stdDevIntrinsics;
	cv::Mat stdDevExtrinsics;
	cv::Mat perViewErrors;
	//double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6 | cv::CALIB_ZERO_TANGENT_DIST | cv::CALIB_FIX_FOCAL_LENGTH | cv::CALIB_USE_INTRINSIC_GUESS | cv::CALIB_FIX_PRINCIPAL_POINT);
	double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(ImageWidth, ImageHeight), cameraMatrix, distCoeffs, rvecs, tvecs, stdDevIntrinsics, stdDevExtrinsics, perViewErrors, cv::CALIB_USE_INTRINSIC_GUESS);

	std::cout << stdDevIntrinsics.size << "\n";
	std::cout << stdDevExtrinsics.size << "\n";
	std::cout << perViewErrors.size << "\n";
	std::cout << stdDevIntrinsics << "\n\n";
	std::cout << perViewErrors << "\n\n";

	t0 = _getTime(AppContext) - t0;
	std::cout << t0 * 1000.0f << " ms\n";

	std::cout << "Calibration error: " << rms << "\n";
	std::cout << "Camera matrix: " << cameraMatrix << "\n";
	std::cout << "Dist coeffs: " << distCoeffs << "\n";

	//int validBoardCount = tvecs.size();
	//send(Client, (const char*)&validBoardCount, 4, 0);

	//for (int i = 0; i < tvecs.size(); ++i) {
	//	//std::cerr << "TVec[" << i << "]: " << tvecs[i].at<double>(0) << ", " << tvecs[i].at<double>(1) << ", " << tvecs[i].at<double>(2) << "\n";
	//	//std::cerr << "RVec[" << i << "]: " << rvecs[i].at<double>(0) << ", " << rvecs[i].at<double>(1) << ", " << rvecs[i].at<double>(2) << "\n";

	//	Mat rotMat = Mat::zeros(3, 3, CV_64F);
	//	Rodrigues(rvecs[i], rotMat);

	//	//std::cerr << rotMat.at<double>(0, 0) << ", " << rotMat.at<double>(0, 1) << ", " << rotMat.at<double>(0, 2) << "\n";
	//	//std::cerr << rotMat.at<double>(1, 0) << ", " << rotMat.at<double>(1, 1) << ", " << rotMat.at<double>(1, 2) << "\n";
	//	//std::cerr << rotMat.at<double>(2, 0) << ", " << rotMat.at<double>(2, 1) << ", " << rotMat.at<double>(2, 2) << "\n";

	//	//fwrite(tvecs[i].data, 8, 3, outF);
	//	//fwrite(rotMat.data, 8, 3 * 3, outF);

	//	send(Client, (const char*)tvecs[i].data, 8 * 3, 0);
	//	send(Client, (const char*)rotMat.data, 8 * 9, 0);
	//}
}