#pragma once

#define CAM_IMG_WIDTH 1600
#define CAM_IMG_HEIGHT 1300
//#define CAM_IMG_WIDTH 1920
//#define CAM_IMG_HEIGHT 1080
//#define CAM_IMG_WIDTH 3280
//#define CAM_IMG_HEIGHT 2464

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
			cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 0.9f, 0.6f, _dictionary);
			_charucoBoards.push_back(board);

			// NOTE: Shift ids.
			int offset = i * board->ids.size();

			for (int j = 0; j < board->ids.size(); ++j) {
				board->ids[j] = offset + j;
			}

			// NOTE: Image output.
			cv::Mat markerImage;
			board->draw(cv::Size(1000, 1000), markerImage, 50, 1);

			if (Output) {
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

void findCharuco(uint8_t* Data, ldiApp* AppContext) {
	double t0 = _getTime(AppContext);

	int offset = 1;

	int width = CAM_IMG_WIDTH;
	int height = CAM_IMG_HEIGHT;
	int id = -1;

	try {
		cv::Mat image(cv::Size(width, height), CV_8UC1, Data);
		//Mat image;
		//cv::flip(imageSrc, image, 0);
		cv::Mat imageDebug;
		cv::cvtColor(image, imageDebug, cv::COLOR_GRAY2RGB);

		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		// TODO: Enable subpixel corner refinement.
		// WTF is this doing?
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
		cv::aruco::detectMarkers(image, _dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
		// Use refine strategy to detect more markers.
		//cv::Ptr<cv::aruco::Board> board = charucoBoard.staticCast<aruco::Board>();
		//aruco::refineDetectedMarkers(image, board, markerCorners, markerIds, rejectedCandidates);

		if (id != -1) {
			cv::aruco::drawDetectedMarkers(imageDebug, markerCorners, markerIds);
		}

		std::vector<std::vector<cv::Point2f>> charucoCorners(6);
		std::vector<std::vector<int>> charucoIds(6);

		AppContext->camImageMarkerCorners.clear();
		AppContext->camImageCharucoCorners.clear();
		AppContext->camImageMarkerIds = markerIds;

		for (int i = 0; i < markerCorners.size(); ++i) {
			for (int j = 0; j < markerCorners[i].size(); ++j) {
				AppContext->camImageMarkerCorners.push_back(vec2(markerCorners[i][j].x, markerCorners[i][j].y));
			}
		}

		// Interpolate charuco corners for each possible board.
		for (int i = 0; i < 6; ++i) {
			if (markerCorners.size() > 0) {
				cv::aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners[i], charucoIds[i]);
			}

			if (charucoCorners[i].size() > 0) {
				if (id != -1) {
					cv::aruco::drawDetectedCornersCharuco(imageDebug, charucoCorners[i], charucoIds[i]);
				}
			}
		}

		int boardCount = charucoIds.size();
		//send(Client, (const char*)&boardCount, 4, 0);
		std::cout << "Boards: " << boardCount << "\n";

		for (int i = 0; i < boardCount; ++i) {
			int markerCount = charucoIds[i].size();
			std::cout << "Board " << i << " markers: " << markerCount << "\n";
			//send(Client, (const char*)&markerCount, 4, 0);

			for (int j = 0; j < markerCount; ++j) {
				AppContext->camImageCharucoCorners.push_back(vec2(charucoCorners[i][j].x, charucoCorners[i][j].y));
				std::cout << charucoCorners[i][j].x << ", " << charucoCorners[i][j].y << "\n";
				/*send(Client, (const char*)&charucoIds[i][j], 4, 0);
				send(Client, (const char*)&charucoCorners[i][j], 8, 0);*/
			}
		}

		if (id != -1) {
			char buf[256];
			sprintf_s(buf, "debug_%d.png", id);
			cv::imwrite(buf, imageDebug);
		}

		t0 = _getTime(AppContext) - t0;
		std::cout << "Find charuco: " << (t0 * 1000.0) << " ms\n";
	}
	catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}