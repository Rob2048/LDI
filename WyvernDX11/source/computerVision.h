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
			cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(5, 5, 1.0f, 0.7f, _dictionary);
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

struct ldiCharucoResults {
	std::vector<vec2>	markerCorners;
	std::vector<int>	markerIds;
	std::vector<vec2>	charucoCorners;
	std::vector<int>	charucoIds;
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
			std::cout << "Board " << i << " markers: " << markerCount << "\n";

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