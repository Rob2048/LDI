#pragma once

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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
	std::vector<ldiCharucoMarker> markers;
	std::vector<ldiCharucoCorner> corners;
};

struct ldiCharucoResults {
	std::vector<ldiCharucoBoard> boards;
	
	/*std::vector<vec2>	markerCorners;
	std::vector<int>	markerIds;
	std::vector<vec2>	charucoCorners;
	std::vector<int>	charucoIds;*/
};

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

		std::vector<std::vector<cv::Point2f>> charucoCorners(6);
		std::vector<std::vector<int>> charucoIds(6);

		Results->markerCorners.clear();
		Results->charucoCorners.clear();
		Results->charucoIds.clear();
		Results->markerIds = markerIds;

		for (int i = 0; i < markerCorners.size(); ++i) {
			for (int j = 0; j < markerCorners[i].size(); ++j) {
				Results->markerCorners.push_back(vec2(markerCorners[i][j].x, markerCorners[i][j].y));
			}
		}

		// Interpolate charuco corners for each possible board.
		//t0 = _getTime(AppContext);
		for (int i = 0; i < 6; ++i) {
			if (markerCorners.size() > 0) {
				cv::aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners[i], charucoIds[i]);
			}
		}
		//t0 = _getTime(AppContext) - t0;
		//std::cout << "Interpolar corners: " << (t0 * 1000.0) << " ms\n";

		int boardCount = charucoIds.size();
		//std::cout << "Boards: " << boardCount << "\n";

		for (int i = 0; i < boardCount; ++i) {
			int markerCount = charucoIds[i].size();
			//std::cout << "Board " << i << " markers: " << markerCount << "\n";

			for (int j = 0; j < markerCount; ++j) {
				int cornerId = charucoIds[i][j];
				Results->charucoCorners.push_back(vec2(charucoCorners[i][j].x, charucoCorners[i][j].y));
				Results->charucoIds.push_back(cornerId);
				//std::cout << cornerId << ": " << charucoCorners[i][j].x << ", " << charucoCorners[i][j].y << "\n";
			}
		}

		/*t0 = _getTime(AppContext) - t0;
		std::cout << "Find charuco: " << (t0 * 1000.0) << " ms\n";*/

	} catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}

void _calibrateCameraCharuco(ldiApp* AppContext, uint8_t* Data, int DataSize) {
	// Input data is:
	// board count
	// for each board:
	//		marker count
	//		for each marker:
	//			marker id
	//			pos x, y

	std::vector<std::vector<int>> charucoIds;
	std::vector<std::vector<cv::Point2f>> charucoCorners;

	int offset = 1;
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
	}

	// TODO: Get better start params?
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1437.433155080189;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 800;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1437.433155080189;
	cameraMatrix.at<double>(1, 2) = 650;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;

	double rms = cv::aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], cv::Size(1600, 1300), cameraMatrix, distCoeffs, rvecs, tvecs);

	/*std::cout << "Calibration error: " << rms << "\n";
	std::cout << "Camera matrix: " << cameraMatrix << "\n";
	std::cout << "Dist coeffs: " << distCoeffs << "\n" << std::flush;*/

	//send(Client, (const char*)&rms, 8, 0);
	//send(Client, (const char*)cameraMatrix.data, 9 * 8, 0);
	//send(Client, (const char*)distCoeffs.data, 8 * 8, 0);

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

struct ldiCalibPoint {
	int id;
	int boardId;
	int fullId;
	vec3 position;
};

struct ldiPhysicalCamera {
	int calibrationData;
};

void calibGenerateTargetCalibPoints() {

}