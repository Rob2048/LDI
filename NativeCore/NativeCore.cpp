#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

using namespace cv;

//------------------------------------------------------------------------------------------------------------------------
// Timer utility functions.
//------------------------------------------------------------------------------------------------------------------------
static int64_t timeFrequency;
static int64_t timeCounterStart;

void timerInit() {
	LARGE_INTEGER freq;
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	timeCounterStart = counter.QuadPart;
	timeFrequency = freq.QuadPart;
}

double getTimeSeconds() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - timeCounterStart;
	double result = (double)time / ((double)timeFrequency);

	return result;
}

//------------------------------------------------------------------------------------------------------------------------
// OpenCV functions.
//------------------------------------------------------------------------------------------------------------------------
cv::Ptr<cv::aruco::Dictionary> _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

static void calcBoardCornerPositions(Size boardSize, float squareSize, std::vector<Point3f>& corners)
{
	corners.clear();

	// Asymetric circles grid.
	for( int i = 0; i < boardSize.height; i++ ) {
		for( int j = 0; j < boardSize.width; j++ ) {
			corners.push_back(Point3f((2*j + i % 2)*squareSize, i*squareSize, 0));
		}
	}
}

void calibrationImageUpload(int Width, int Height, std::string Path, std::string OutPath) {
	int width = Width;
	int height = Height;
	std::string path = Path;
	std::string outputPath = OutPath;
	int totalPixels = width * height;

	std::cerr << "Calibration upload " << width << " " << height << " " << path << " -> " << outputPath << "\n" << std::flush;

	FILE* f = fopen(path.c_str(), "rb");

	uint8_t* imgData = new uint8_t[totalPixels];
	int bytesRead = fread(imgData, totalPixels, 1, f);

	fclose(f);

	std::cerr << "Image read: " << bytesRead << "\n" << std::flush;

	Mat image(Size(width, height), CV_8UC1, imgData);

	std::vector<Point2f> pointBuf;
	Size boardSize(4, 11);

	double t = getTimeSeconds();
	bool found = false;

	cv::SimpleBlobDetector::Params blobParams;
	blobParams.filterByArea = true;
	blobParams.maxArea = 10000;

	blobParams.filterByCircularity = true;
	blobParams.minCircularity = 0.1;
	blobParams.maxCircularity = 100.0;
	
	blobParams.filterByInertia = true;
	blobParams.minInertiaRatio = 0.01;
	blobParams.maxInertiaRatio = 100.0;

	blobParams.filterByConvexity = false;

	cv::Ptr<FeatureDetector> blobDetector = cv::SimpleBlobDetector::create(blobParams);


	std::vector<cv::KeyPoint> keypoints;
	blobDetector->detect(image, keypoints);

	try {
		found = findCirclesGrid(image, boardSize, pointBuf, CALIB_CB_ASYMMETRIC_GRID | CALIB_CB_CLUSTERING, blobDetector);
	} catch (cv::Exception& e) {
		// NOTE: If there are no circles found in OpenCV 4.5.1 then an exception is thrown.
		std::cout << "CV exception: " << e.what() << "\n";
	}

	t = getTimeSeconds() - t;

	std::cerr << found << " " << pointBuf.size() << " in " << t << "\n";

	FILE* outputF = fopen(outputPath.c_str(), "wb");

	/*int pointCount = keypoints.size();	
	fwrite(&pointCount, 4, 1, outputF);
	for (int i = 0; i < pointCount; ++i) {
		float x = keypoints[i].pt.x;
		float y = keypoints[i].pt.y;

		fwrite(&x, 4, 1, outputF);
		fwrite(&y, 4, 1, outputF);
	}*/
	
	int pointCount = pointBuf.size();	
	fwrite(&found, 1, 1, outputF);
	fwrite(&pointCount, 4, 1, outputF);
	for (int i = 0; i < pointCount; ++i) {
		float x = pointBuf[i].x;
		float y = pointBuf[i].y;

		fwrite(&x, 4, 1, outputF);
		fwrite(&y, 4, 1, outputF);
	}

	fclose(f);

	delete[] imgData;
}

void calibrationImageUploadCmd() {
	int width = 0;
	int height = 0;
	std::string path;
	std::string outputPath;

	std::cin >> width;
	std::cin >> height;
	std::getline(std::cin, path); // NOTE: Consumes random linefeed?
	std::getline(std::cin, path);
	std::getline(std::cin, outputPath);

	calibrationImageUpload(width, height, path, outputPath);
}

void calibrationProcessCmd() {
	std::string inPath;
	std::string outPath;

	std::getline(std::cin, inPath);
	std::getline(std::cin, outPath);
	
	std::cerr << "Calibration: " << inPath << " -> " << outPath << "\n" << std::flush;

	FILE* inF = fopen(inPath.c_str(), "rb");

	int captureCount = 0;
	fread(&captureCount, 4, 1, inF);

	std::cerr << "Capture count: " << captureCount << "\n";

	if (captureCount == 0) {
		return;
	}

	std::vector<std::vector<Point3f> > objectPoints(1);
	std::vector<std::vector<Vec2f> > imagePoints;
	Size boardSize(4, 11);

	// Blob 95x95
	// x = 236
	// y = 118
	calcBoardCornerPositions(boardSize, 1.1810f, objectPoints[0]);

	objectPoints.resize(captureCount, objectPoints[0]);

	for (int i = 0; i < captureCount; ++i) {
		int pointCount = 0;
		fread(&pointCount, 4, 1, inF);

		std::vector<Vec2f> screenPoints;

		for (int j = 0; j < pointCount; ++j) {
			float x = 0.0f;
			float y = 0.0f;

			fread(&x, 4, 1, inF);
			fread(&y, 4, 1, inF);

			screenPoints.push_back(Vec2f(x, y));
		}

		imagePoints.push_back(screenPoints);
	}

	fclose(inF);

	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1437.433155080189;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 960;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1437.433155080189;
	cameraMatrix.at<double>(1, 2) = 540;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	Mat distCoeffs = Mat::zeros(8, 1, CV_64F);
	std::vector<Mat> rvecs;
	std::vector<Mat> tvecs;

	//double rms = calibrateCamera(objectPoints, imagePoints, Size(1920, 1080), cameraMatrix, distCoeffs, rvecs, tvecs, CALIB_USE_INTRINSIC_GUESS | CALIB_FIX_PRINCIPAL_POINT | CALIB_FIX_ASPECT_RATIO | CALIB_ZERO_TANGENT_DIST | CALIB_FIX_K1 | CALIB_FIX_K2 | CALIB_FIX_K3, cv::TermCriteria(cv::TermCriteria::COUNT, 1000, 0.0));
	double rms = calibrateCamera(objectPoints, imagePoints, Size(1920, 1080), cameraMatrix, distCoeffs, rvecs, tvecs);

	std::cerr << "Calibration error: " << rms << "\n";
	std::cerr << "Camera matrix: " << cameraMatrix << "\n";
	std::cerr << "Dist coeffs: " << distCoeffs << "\n";

	FILE* outF = fopen(outPath.c_str(), "wb");

	fwrite(cameraMatrix.data, 8, 9, outF);
	fwrite(distCoeffs.data, 8, 8, outF);

	fwrite(&captureCount, 4, 1, outF);

	for (int i = 0; i < tvecs.size(); ++i) {
		std::cerr << "TVec[" << i << "]: " << tvecs[i].at<double>(0) << ", " << tvecs[i].at<double>(1) << ", " << tvecs[i].at<double>(2) << "\n";
		std::cerr << "RVec[" << i << "]: " << rvecs[i].at<double>(0) << ", " << rvecs[i].at<double>(1) << ", " << rvecs[i].at<double>(2) << "\n";

		Mat rotMat = Mat::zeros(3, 3, CV_64F);
		Rodrigues(rvecs[i], rotMat);

		std::cerr << rotMat.at<double>(0,0) << ", " << rotMat.at<double>(0,1) << ", " << rotMat.at<double>(0,2) << "\n";
		std::cerr << rotMat.at<double>(1,0) << ", " << rotMat.at<double>(1,1) << ", " << rotMat.at<double>(1,2) << "\n";
		std::cerr << rotMat.at<double>(2,0) << ", " << rotMat.at<double>(2,1) << ", " << rotMat.at<double>(2,2) << "\n";

		fwrite(tvecs[i].data, 8, 3, outF);
		fwrite(rotMat.data, 8, 3 * 3, outF);
	}

	//Mat rotationVec;
	//Mat translationVec;
	//solvePnP(objectPoints[0], imagePoints[0], cameraMatrix, distCoeffs, rotationVec, translationVec);

	//std::cerr << "PNP: " << translationVec << ", " << rotationVec << "\n";

	fclose(outF);
}

void findPoseCmd() {
	std::string calibPath;
	std::string imgPath;
	std::string outPath;

	std::getline(std::cin, calibPath);
	std::getline(std::cin, imgPath);
	std::getline(std::cin, outPath);

	std::cerr << "Pose: " << calibPath << " " << imgPath << " -> " << outPath << "\n" << std::flush;

	// Calibration input.
	FILE* calibF = fopen(calibPath.c_str(), "rb");

	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
	Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

	fread(cameraMatrix.data, 8, 9, calibF);
	fread(distCoeffs.data, 8, 8, calibF);

	fclose(calibF);

	// Image point input.
	std::vector<Point2d> imagePoints;

	FILE* imgF = fopen(imgPath.c_str(), "rb");

	bool found = false;
	int pointCount = 0;

	fread(&found, 1, 1, imgF);
	fread(&pointCount, 4, 1, imgF);

	if (!found || pointCount == 0) {
		fclose(imgF);
		std::cerr << "No points found to match pose with\n";
		return;
	}

	for (int i = 0; i < pointCount; ++i) {
		float x = 0.0f;
		float y = 0.0f;

		fread(&x, 4, 1, imgF);
		fread(&y, 4, 1, imgF);

		imagePoints.push_back(Point2d(x, y));
	}

	fclose(imgF);

	std::cerr << "Found: " << found << " Points: " << pointCount << "\n";

	// Calculate pose
	std::vector<Point3f> objectPoints;
	Mat rotationVec;
	Mat translationVec;

	Size boardSize(4, 11);
	calcBoardCornerPositions(boardSize, 1.1810f, objectPoints);

	solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rotationVec, translationVec);

	//std::cerr << "PNP: " << translationVec << ", " << rotationVec << "\n";

	// Pose output.
	FILE* outF = fopen(outPath.c_str(), "wb");

	Mat rotMat = Mat::zeros(3, 3, CV_64F);
	Rodrigues(rotationVec, rotMat);
		
	fwrite(translationVec.data, 8, 3, outF);
	fwrite(rotMat.data, 8, 3 * 3, outF);

	fclose(outF);
}

void _findBlobs(uint8_t* Data, int DataSize, SOCKET Client) {
	double t = getTimeSeconds();

	if (DataSize < 9) {
		return;
	}

	int width;
	int height;

	memcpy(&width, Data + 1, 4);
	memcpy(&height, Data + 5, 4);

	Mat image(Size(width, height), CV_8UC1, Data + 9);

	cv::SimpleBlobDetector::Params blobParams;
	blobParams.filterByArea = true;
	blobParams.maxArea = 10000;

	blobParams.filterByCircularity = true;
	blobParams.minCircularity = 0.1;
	blobParams.maxCircularity = 100.0;

	blobParams.filterByInertia = true;
	blobParams.minInertiaRatio = 0.01;
	blobParams.maxInertiaRatio = 100.0;

	blobParams.filterByConvexity = false;

	cv::Ptr<FeatureDetector> blobDetector = cv::SimpleBlobDetector::create(blobParams);

	std::vector<cv::KeyPoint> keypoints;
	blobDetector->detect(image, keypoints);

	t = getTimeSeconds() - t;

	std::cout << "Find blobs: " << width << ", " << height << ", " << keypoints.size() << " in " << (t * 1000.0) << " ms\n" << std::flush;

	int pointCount = keypoints.size();

	send(Client, (const char*)&pointCount, 4, 0);

	for (int i = 0; i < pointCount; ++i) {

		Point2f pos = keypoints[i].pt;
		float size = keypoints[i].size;

		send(Client, (const char*)&pos, 8, 0);
		send(Client, (const char*)&size, 4, 0);
	}

	cv::Mat flipped;
	cv::flip(image, flipped, 0);

	std::vector<int> markerIds;
	std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
	cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
	// TODO: Enable subpixel corner refinement.
	//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
	
	cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_100);
	cv::aruco::detectMarkers(flipped, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

	std::cout << "Corners: " << markerCorners.size() << " MarkerId: " << markerIds.size() << " Rejected: " << rejectedCandidates.size() << "\n" << std::flush;

	int polygonCount = rejectedCandidates.size() + markerCorners.size();
	send(Client, (const char*)&polygonCount, 4, 0);

	for (int i = 0; i < rejectedCandidates.size(); ++i) {
		int polygonPointCount = rejectedCandidates[i].size();
		int polygonType = 0;
		send(Client, (const char*)&polygonType, 4, 0);
		send(Client, (const char*)&polygonPointCount, 4, 0);

		for (int j = 0; j < polygonPointCount; ++j) {
			Point2f pos = rejectedCandidates[i][j];
			pos.y = height - pos.y;
			send(Client, (const char*)&pos, 8, 0);
		}
	}

	for (int i = 0; i < markerCorners.size(); ++i) {
		int polygonPointCount = markerCorners[i].size();
		int polygonType = 1;
		int polygonId = markerIds[i];

		send(Client, (const char*)&polygonType, 4, 0);
		send(Client, (const char*)&polygonPointCount, 4, 0);
		send(Client, (const char*)&polygonId, 4, 0);

		for (int j = 0; j < polygonPointCount; ++j) {
			Point2f pos = markerCorners[i][j];
			pos.y = height - pos.y;
			send(Client, (const char*)&pos, 8, 0);
		}
	}
}

void _findCharuco(uint8_t* Data, int DataSize, SOCKET Client) {
	double t = getTimeSeconds();

	if (DataSize < 9) {
		return;
	}

	int offset = 1;

	int width;
	int height;
	int id;

	memcpy(&width, Data + offset, 4); offset += 4;
	memcpy(&height, Data + offset, 4); offset += 4;
	memcpy(&id, Data + offset, 4); offset += 4;

	//std::cout << "Finding charuco\n" << std::flush;

	try {
		Mat imageSrc(Size(width, height), CV_8UC1, Data + offset);
		Mat image;
		cv::flip(imageSrc, image, 0);
		Mat imageDebug;
		cv::cvtColor(image, imageDebug, cv::COLOR_GRAY2RGB);

		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		// TODO: Enable subpixel corner refinement.
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
		cv::aruco::detectMarkers(image, _dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
		// Use refine strategy to detect more markers.
		//cv::Ptr<cv::aruco::Board> board = charucoBoard.staticCast<aruco::Board>();
		//aruco::refineDetectedMarkers(image, board, markerCorners, markerIds, rejectedCandidates);

		if (id != -1) {
			aruco::drawDetectedMarkers(imageDebug, markerCorners, markerIds);
		}
		
		std::vector<std::vector<Point2f>> charucoCorners(6);
		std::vector<std::vector<int>> charucoIds(6);

		// Interpolate charuco corners for each possible board.
		for (int i = 0; i < 6; ++i) {
			if (markerCorners.size() > 0) {
				aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners[i], charucoIds[i]);
			}

			if (charucoCorners[i].size() > 0) {
				if (id != -1) {
					aruco::drawDetectedCornersCharuco(imageDebug, charucoCorners[i], charucoIds[i]);
				}
			}
		}

		int boardCount = charucoIds.size();
		send(Client, (const char*)&boardCount, 4, 0);

		for (int i = 0; i < boardCount; ++i) {
			int markerCount = charucoIds[i].size();
			send(Client, (const char*)&markerCount, 4, 0);

			for (int j = 0; j < markerCount; ++j) {
				send(Client, (const char*)&charucoIds[i][j], 4, 0);
				send(Client, (const char*)&charucoCorners[i][j], 8, 0);
			}
		}

		if (id != -1) {
			char buf[256];
			sprintf_s(buf, "calib captures/chr_%d.png", id);
			cv::imwrite(buf, imageDebug);
		}

	} catch (Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}

	//std::cout << "Done finding charuco\n" << std::flush;
}

void _fitLine(uint8_t* Data, int DataSize, SOCKET Client) {
	int offset = 1;

	int pointCount = 0;
	memcpy(&pointCount, Data + offset, 4); offset += 4;

	std::vector<Point3f> points;

	for (int i = 0; i < pointCount; ++i) {
		Point3f	point;
		memcpy(&point, Data + offset, 12); offset += 12;
		points.push_back(point);
	}

	std::cout << "Find line fit " << pointCount << "\n" << std::flush;

	Point3f centroid(0, 0, 0);

	for (int i = 0; i < pointCount; ++i) {
		centroid += points[i];
	}

	centroid /= pointCount;

	std::cout << "Centroid: " << centroid.x << ", " << centroid.y << ", " << centroid.z << "\n" << std::flush;

	Mat mP = Mat(pointCount, 3, CV_32F);

	for (int i = 0; i < pointCount; ++i) {
		points[i] -= centroid;
		mP.at<float>(i, 0) = points[i].x;
		mP.at<float>(i, 1) = points[i].y;
		mP.at<float>(i, 2) = points[i].z;
	}

	Mat w, u, vt;
	SVD::compute(mP, w, u, vt);

	//SVD::compute(mP, w);
	Point3f pW(w.at<float>(0, 0), w.at<float>(0, 1), w.at<float>(0, 2));
	Point3f pU(u.at<float>(0, 0), u.at<float>(0, 1), u.at<float>(0, 2));
	Point3f pV(vt.at<float>(0, 0), vt.at<float>(0, 1), vt.at<float>(0, 2));

	/*std::cout << "W: " << pW.x << ", " << pW.y << ", " << pW.z << "\n" << std::flush;
	std::cout << "U: " << pU.x << ", " << pU.y << ", " << pU.z << "\n" << std::flush;
	std::cout << "V: " << pV.x << ", " << pV.y << ", " << pV.z << "\n" << std::flush;*/
	
	send(Client, (const char*)&centroid, 12, 0);
	send(Client, (const char*)&pV, 12, 0);
}

float Sign(Point2f p1, Point2f p2, Point2f p3) {
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool PointInTriangle(Point2f pt, Point2f v1, Point2f v2, Point2f v3) {
	float d1, d2, d3;
	bool has_neg, has_pos;

	d1 = Sign(pt, v1, v2);
	d2 = Sign(pt, v2, v3);
	d3 = Sign(pt, v3, v1);

	has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(has_neg && has_pos);
}

void _findScanline(uint8_t* Data, int DataSize, SOCKET Client) {
	if (DataSize < 9) {
		return;
	}

	try {

		// std::cout << "Start data...\n" << std::flush;

		int offset = 1;

		Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
		Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

		memcpy(cameraMatrix.data, Data + offset, 8 * 9); offset += 8 * 9;
		memcpy(distCoeffs.data, Data + offset, 8 * 8); offset += 8 * 8;

		Mat tvec = Mat(3, 1, CV_64F);
		Mat rvec = Mat(3, 3, CV_64F);

		memcpy(tvec.data, Data + offset, 8 * 3); offset += 8 * 3;
		memcpy(rvec.data, Data + offset, 8 * 9); offset += 8 * 9;

		int width;
		int height;
		int id;

		memcpy(&width, Data + offset, 4); offset += 4;
		memcpy(&height, Data + offset, 4); offset += 4;
		memcpy(&id, Data + offset, 4); offset += 4;

		// std::cout << "Image: " << width << " " << height << "\n" << std::flush;

		Mat imageSrc(Size(width, height), CV_8UC1, Data + offset); offset += width * height;

		int boundryPointCount = 0;
		std::vector<Point3f> boundryPoints;

		memcpy(&boundryPointCount, Data + offset, 4); offset += 4;

		// std::cout << "Boundry count: " << boundryPointCount << "\n" << std::flush;

		for (int i = 0; i < boundryPointCount; ++i) {
			Point3f point;
			memcpy(&point, Data + offset, 12); offset += 12;
			boundryPoints.push_back(point);
		}

		// std::cout << "Got data...\n" << std::flush;

		Mat image;
		cv::flip(imageSrc, image, 0);

		// Threshold image.
		for (int i = 0; i < width * height; ++i) {
			uint8_t v = image.at<uint8_t>(i);

			if (v > 100) {
				v = 255;
			}
			else {
				v = 0;
			}

			/*float vF = v;
			vF *= 2.0f;

			if (vF > 255.0f) {
				vF = 255.0f;
			}
			else if (vF < 0.0f) {
				vF = 0.0f;
			}*/

			image.at<uint8_t>(i) = (uint8_t)v;
		}

		Mat imageDebug;
		cv::cvtColor(image, imageDebug, cv::COLOR_GRAY2RGB);
		
		double t = getTimeSeconds();

		float* lineLocs = new float[height];

		// Find point for each line.
		for (int i = 0; i < height; ++i) {
			int startIdx = i * width;
			int endIdx = startIdx + width;
			lineLocs[i] = 0.0f;

			int lineStart = 0;
			int lineEnd = 0;
			int lineState = 0;

			for (int j = startIdx; j < endIdx; ++j) {

				if (lineState == 0) {
					if (image.at<uint8_t>(j) > 0) {
						lineStart = j - startIdx;
						lineState = 1;
					}
				}
				else if (lineState == 1) {
					if (image.at<uint8_t>(j) == 0) {
						lineEnd = j - startIdx - 1;
						break;
					}
				}
			}

			if (lineEnd >= lineStart) {
				lineLocs[i] = lineStart + (lineEnd - lineStart) / 2.0f;
			}
		}

		t = getTimeSeconds() - t;
		// std::cout << "Scanline found in " << (t * 1000.0) << " ms\n" << std::flush;

		// Find boundry points.		
		std::vector<Point2f> projectedImagePoints;

		if (boundryPointCount != 0) {
			projectPoints(boundryPoints, rvec, tvec, cameraMatrix, distCoeffs, projectedImagePoints);

			for (int i = 0; i < projectedImagePoints.size(); ++i) {
				projectedImagePoints[i].y = height - projectedImagePoints[i].y;
			}

			for (int i = 0; i < projectedImagePoints.size(); ++i) {
				Point2f p = projectedImagePoints[i];
				// std::cout << "Point: " << p.x << ", " << p.y << "\n" << std::flush;

				circle(imageDebug, p, 4, Scalar(255, 0, 0), FILLED);

				Point2f b;

				if (i < projectedImagePoints.size() - 1) {
					b = projectedImagePoints[i + 1];
				}
				else {
					b = projectedImagePoints[0];
				}

				line(imageDebug, p, b, Scalar(255, 0, 64), 2);
			}
		}

		std::vector<Point2f> scanPoints;

		for (int i = 0; i < height; ++i) {
			if (lineLocs[i] == 0.0f) {
				continue;
			}

			//circle(imageDebug, Point2f(lineLocs[i], i + 0.5f), 1, Scalar(0, 0, 255), FILLED);
			Point2f p = Point2f(lineLocs[i], i);

			if (boundryPointCount == 0 || PointInTriangle(p, projectedImagePoints[0], projectedImagePoints[1], projectedImagePoints[3]) ||
				PointInTriangle(p, projectedImagePoints[1], projectedImagePoints[2], projectedImagePoints[3])) {

				scanPoints.push_back(p);

				int index = (i * width + (int)lineLocs[i]) * 3;
				imageDebug.at<uint8_t>(index + 0) = 0;
				imageDebug.at<uint8_t>(index + 1) = 0;
				imageDebug.at<uint8_t>(index + 2) = 255;
			}
		}

		delete[] lineLocs;

		std::vector<Point2f> scanPointsUndistorted;

		if (scanPoints.size() > 0) {
			undistortPoints(scanPoints, scanPointsUndistorted, cameraMatrix, distCoeffs);
		}

		float camFx = cameraMatrix.at<double>(0, 0);
		float camFy = cameraMatrix.at<double>(1, 1);
		float camCx = cameraMatrix.at<double>(0, 2);
		float camCy = cameraMatrix.at<double>(1, 2);

		// std::cout << camFx << " " << camFy << " " << camCx << " " << camCy << "\n" << std::flush;

		int scanPointCount = scanPointsUndistorted.size();
		int sendR = send(Client, (const char*)&scanPointCount, 4, 0);

		int sockErr = WSAGetLastError();
		
		// std::cout << "Sent: " << (int)Client << " " << sendR << " " << sockErr << "\n" << std::flush;

		for (int i = 0; i < scanPointsUndistorted.size(); ++i) {
			//scanPointsUndistorted[i].x = scanPointsUndistorted[i].x * camFx + camCx;
			//scanPointsUndistorted[i].y = scanPointsUndistorted[i].y * camFy + camCy;
			//std::cout << "UD: " << i << " " << scanPointsUndistorted[i].x << ", " << scanPointsUndistorted[i].y << "\n" << std::flush;

			send(Client, (const char*)&scanPointsUndistorted[i], 8, 0);
		}

		if (id != -1) {
			char buf[256];
			sprintf_s(buf, "scanline_%d.png", id);
			cv::imwrite(buf, imageDebug);
		}

	}
	catch (Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}

	//std::cout << "Done finding charuco\n" << std::flush;
}

void _calibrateCameraCharuco(uint8_t* Data, int DataSize, SOCKET Client) {
	
	std::vector<std::vector<int>> charucoIds;
	std::vector<std::vector<Point2f>> charucoCorners;

	int offset = 1;
	int boardCount = 0;
	memcpy(&boardCount, Data + offset, 4); offset += 4;

	for (int i = 0; i < boardCount; ++i) {
		std::vector<int> ids;
		std::vector<Point2f> corners;

		int markerCount = 0;
		memcpy(&markerCount, Data + offset, 4); offset += 4;

		for (int j = 0; j < markerCount; ++j) {
			int markerId;
			Point2f markerPos;

			memcpy(&markerId, Data + offset, 4); offset += 4;
			memcpy(&markerPos, Data + offset, 8); offset += 8;

			ids.push_back(markerId);
			corners.push_back(markerPos);
		}

		charucoIds.push_back(ids);
		charucoCorners.push_back(corners);
	}

	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
	cameraMatrix.at<double>(0, 0) = 1437.433155080189;
	cameraMatrix.at<double>(0, 1) = 0.0;
	cameraMatrix.at<double>(0, 2) = 960;
	cameraMatrix.at<double>(1, 0) = 0.0;
	cameraMatrix.at<double>(1, 1) = 1437.433155080189;
	cameraMatrix.at<double>(1, 2) = 540;
	cameraMatrix.at<double>(2, 0) = 0.0;
	cameraMatrix.at<double>(2, 1) = 0.0;
	cameraMatrix.at<double>(2, 2) = 1.0;

	Mat distCoeffs = Mat::zeros(8, 1, CV_64F);
	std::vector<Mat> rvecs;
	std::vector<Mat> tvecs;

	double rms = aruco::calibrateCameraCharuco(charucoCorners, charucoIds, _charucoBoards[0], Size(1920, 1080), cameraMatrix, distCoeffs, rvecs, tvecs);

	/*std::cout << "Calibration error: " << rms << "\n";
	std::cout << "Camera matrix: " << cameraMatrix << "\n";
	std::cout << "Dist coeffs: " << distCoeffs << "\n" << std::flush;*/

	send(Client, (const char*)&rms, 8, 0);
	send(Client, (const char*)cameraMatrix.data, 9 * 8, 0);
	send(Client, (const char*)distCoeffs.data, 8 * 8, 0);

	int validBoardCount = tvecs.size();
	send(Client, (const char*)&validBoardCount, 4, 0);

	for (int i = 0; i < tvecs.size(); ++i) {
		//std::cerr << "TVec[" << i << "]: " << tvecs[i].at<double>(0) << ", " << tvecs[i].at<double>(1) << ", " << tvecs[i].at<double>(2) << "\n";
		//std::cerr << "RVec[" << i << "]: " << rvecs[i].at<double>(0) << ", " << rvecs[i].at<double>(1) << ", " << rvecs[i].at<double>(2) << "\n";

		Mat rotMat = Mat::zeros(3, 3, CV_64F);
		Rodrigues(rvecs[i], rotMat);

		//std::cerr << rotMat.at<double>(0, 0) << ", " << rotMat.at<double>(0, 1) << ", " << rotMat.at<double>(0, 2) << "\n";
		//std::cerr << rotMat.at<double>(1, 0) << ", " << rotMat.at<double>(1, 1) << ", " << rotMat.at<double>(1, 2) << "\n";
		//std::cerr << rotMat.at<double>(2, 0) << ", " << rotMat.at<double>(2, 1) << ", " << rotMat.at<double>(2, 2) << "\n";

		//fwrite(tvecs[i].data, 8, 3, outF);
		//fwrite(rotMat.data, 8, 3 * 3, outF);

		send(Client, (const char*)tvecs[i].data, 8 * 3, 0);
		send(Client, (const char*)rotMat.data, 8 * 9, 0);
	}
}

void _findCharucoPose(uint8_t* Data, int DataSize, SOCKET Client) {
	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
	Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

	int offset = 1;

	memcpy(cameraMatrix.data, Data + offset, 8 * 9); offset += 8 * 9;
	memcpy(distCoeffs.data, Data + offset, 8 * 8); offset += 8 * 8;

	//std::cout << "Camera matrix: " << cameraMatrix << "\n";
	//std::cout << "Dist coeffs: " << distCoeffs << "\n";

	int markerCount = 0;
	memcpy(&markerCount, Data + offset, 4); offset += 4;

	//std::cout << "Marker counts: " << markerCount << "\n";

	std::vector<int> markerIds;
	std::vector<Point2f> markerPos;

	for (int i = 0; i < markerCount; ++i) {
		int id;
		Point2f point;

		memcpy(&id, Data + offset, 4); offset += 4;
		memcpy(&point, Data + offset, 8); offset += 8;

		//std::cout << "Marker " << i << " id: " << id << " pos: " << point << "\n";

		markerIds.push_back(id);
		markerPos.push_back(point);
	}

	Mat rvec;
	Mat tvec;

	bool found = aruco::estimatePoseCharucoBoard(markerPos, markerIds, _charucoBoards[0], cameraMatrix, distCoeffs, rvec, tvec);

	int sendFlag = found;
	send(Client, (const char*)&sendFlag, 4, 0);

	if (found) {
		Mat rotMat = Mat::zeros(3, 3, CV_64F);
		Rodrigues(rvec, rotMat);

		send(Client, (const char*)tvec.data, 8 * 3, 0);
		send(Client, (const char*)rotMat.data, 8 * 9, 0);
	}
}

void _findGeneralPose(uint8_t* Data, int DataSize, SOCKET Client) {
	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
	Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

	int offset = 1;

	memcpy(cameraMatrix.data, Data + offset, 8 * 9); offset += 8 * 9;
	memcpy(distCoeffs.data, Data + offset, 8 * 8); offset += 8 * 8;

	//std::cout << "Camera matrix: " << cameraMatrix << "\n";
	//std::cout << "Dist coeffs: " << distCoeffs << "\n";

	int markerCount = 0;
	memcpy(&markerCount, Data + offset, 4); offset += 4;

	//std::cout << "Marker counts: " << markerCount << "\n";

	std::vector<Point2f> markerPos;
	std::vector<Point3f> worldPoints;

	for (int i = 0; i < markerCount; ++i) {
		Point2f point;
		Point3f worldPos;

		memcpy(&point, Data + offset, 8); offset += 8;
		memcpy(&worldPos, Data + offset, 12); offset += 12;

		//std::cout << "Marker " << i << " id: " << id << " pos: " << point << "\n";

		markerPos.push_back(point);
		worldPoints.push_back(worldPos);
	}

	//Mat rvec;
	//Mat tvec;
	//bool found = aruco::estimatePoseCharucoBoard(markerPos, markerIds, _charucoBoards[0], cameraMatrix, distCoeffs, rvec, tvec);

	// Calculate pose
	std::vector<Point2d> imagePoints;
	//bool found = solvePnP(worldPoints, markerPos, cameraMatrix, distCoeffs, rvec, tvec);

	std::vector<Mat> rvecs;
	std::vector<Mat> tvecs;
	std::vector<float> rms;

	int solutionCount = solvePnPGeneric(worldPoints, markerPos, cameraMatrix, distCoeffs, rvecs, tvecs, false, SOLVEPNP_ITERATIVE, noArray(), noArray(), rms);

	solutionCount = rvecs.size();

	send(Client, (const char*)&solutionCount, 4, 0);

	for (int i = 0; i < solutionCount; ++i) {
		std::vector<Point2f> projectedImagePoints;
		projectPoints(worldPoints, rvecs[i], tvecs[i], cameraMatrix, distCoeffs, projectedImagePoints);

		Mat rotMat = Mat::zeros(3, 3, CV_64F);
		Rodrigues(rvecs[i], rotMat);

		send(Client, (const char*)tvecs[i].data, 8 * 3, 0);
		send(Client, (const char*)rotMat.data, 8 * 9, 0);

		int projectedPointCount = projectedImagePoints.size();
		send(Client, (const char*)&projectedPointCount, 4, 0);

		for (int j = 0; j < projectedPointCount; ++j) {
			send(Client, (const char*)&projectedImagePoints[j], 8, 0);
		}

		send(Client, (const char*)&rms[i], 4, 0);
	}
}

void _triangulatePoints(uint8_t* Data, int DataSize, SOCKET Client) {
	int offset = 1;

	Mat camProj0 = Mat::eye(3, 4, CV_64F);
	Mat camProj1 = Mat::eye(3, 4, CV_64F);

	memcpy(camProj0.data, Data + offset, 8 * 12); offset += 8 * 16;
	memcpy(camProj1.data, Data + offset, 8 * 12); offset += 8 * 16;

	int pointCount = 0;
	memcpy(&pointCount, Data + offset, 4); offset += 4;

	std::vector<Point2f> points0;
	std::vector<Point2f> points1;

	for (int i = 0; i < pointCount; ++i) {
		Point2f pos;
		memcpy(&pos, Data + offset, 8); offset += 8;
		points1.push_back(pos);
	}
	
	std::cout << "Camera proj0: " << camProj0 << "\n";
	std::cout << "Camera proj1: " << camProj1 << "\n";
	std::cout << "Point count: " << pointCount << "\n" << std::flush;

	cv::Mat outputs;
	cv::triangulatePoints(camProj0, camProj1, points0, points1, outputs);

	int outputPoints = outputs.size().width;
	send(Client, (const char*)&outputPoints, 4, 0);

	for (int i = 0; i < outputPoints; ++i) {
		float w = outputs.at<float>(3, i);
		float x = outputs.at<float>(0, i) / w;
		float y = outputs.at<float>(1, i) / w;
		float z = outputs.at<float>(2, i) / w;

		send(Client, (const char*)&x, 4, 0);
		send(Client, (const char*)&y, 4, 0);
		send(Client, (const char*)&z, 4, 0);
	}

	std::cout << "3d points: " << outputPoints << "\n" << std::flush;
}

//------------------------------------------------------------------------------------------------------------------------
// Sockets interface.
//------------------------------------------------------------------------------------------------------------------------
#define BUFLEN (1024 * 1024 * 16)
uint8_t _recvBuf[BUFLEN];
int _recvLen = 0;

uint8_t _packetData[BUFLEN];
int _packetRecvState = 0;
int _packetPayloadLen = 0;
int _packetSize = 0;

bool runServer() {
	WSADATA wsaData;
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	int iSendResult;
	
	int recvBufLen = BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "Net startup failed: " << iResult << "\n";
		return false;
	}

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, "20000", &hints, &result);
	if (iResult != 0) {
		std::cout << "Getaddrinfo failed: " << iResult << "\n";
		WSACleanup();
		return false;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Listen socket failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Bind failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Listen failed: " << WSAGetLastError() << "\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	std::cout << "Net started successfully\n" << std::flush;

	// Accept
	clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		std::cout << "Accept failed: " << WSAGetLastError() << "\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	closesocket(listenSocket);

	std::cout << "Got client connection " << (int)clientSocket << "\n" << std::flush;

	// Receive until shutdown.
	do {
		iResult = recv(clientSocket, (char*)(_recvBuf), BUFLEN, 0);
		_recvLen = iResult;
		//std::cout << "Got bytes: " << _recvLen << "\n" << std::flush;

		if (iResult > 0) {
			
			for (int i = 0; i < _recvLen; ++i) {
				if (_packetRecvState == 0) {
					_packetData[_packetSize++] = _recvBuf[i];

					if (_packetSize == 4) {
						memcpy(&_packetPayloadLen, _packetData, 4);
						_packetSize = 0;

						//std::cout << "Got payload size: " << _packetPayloadLen << "\n" << std::flush;

						if (_packetPayloadLen > BUFLEN || _packetPayloadLen < 1) {
							std::cerr << "Invalid packet payload size: " << _packetPayloadLen << "\n" << std::flush;
						} else {
							_packetRecvState = 1;
						}
					}
				} else {
					_packetData[_packetSize++] = _recvBuf[i];

					//std::cout << "Packet size: " << _packetSize << "/" << _packetPayloadLen << "\n" << std::flush;

					if (_packetSize == _packetPayloadLen) {
						//std::cout << "Got packet\n" << std::flush;
						_packetRecvState = 0;
						_packetSize = 0;

						uint8_t opcode = _packetData[0];

						switch (opcode) {
							case 1: {
								_findBlobs(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 2: {
								_findCharuco(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 3: {
								_calibrateCameraCharuco(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 4: {
								_findCharucoPose(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 5: {
								_triangulatePoints(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 6: {
								_findGeneralPose(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 7: {
								_findScanline(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
							case 8: {
								_fitLine(_packetData, _packetPayloadLen, clientSocket);
								break;
							}
						}

						if (_packetData[0] == 1) {
							
						}
					}
				}
			}

		} else if (iResult == 0) {
			std::cout << "Client connection closing\n" << std::flush;
		} else {
			std::cout << "Client connection error: " << WSAGetLastError() << "\n" << std::flush;
			closesocket(clientSocket);
			WSACleanup();
			return false;
		}

	} while (iResult > 0);

	closesocket(clientSocket);
	WSACleanup();

	return true;
}

void CreateCharucos() {
	//for (int i = 0; i < 6; ++i) {
	//	cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.009f, 0.006f, _dictionary);
	//	charucoBoards.push_back(board);

	//	// NOTE: Shift ids.
	//	// board->ids.
	//	int offset = i * board->ids.size();

	//	for (int j = 0; j < board->ids.size(); ++j) {
	//		board->ids[j] = offset + j;
	//	}
	//}

	try {
		for (int i = 0; i < 6; ++i) {
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
			char fileName[512];
			sprintf_s(fileName, "charuco_small_%d.png", i);
			cv::imwrite(fileName, markerImage);
		}

	}
	catch (Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}

//------------------------------------------------------------------------------------------------------------------------
// Application entry.
//------------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	std::cout << "Native core started\n" << std::flush;
	timerInit();

	CreateCharucos();

	//calibrationImageUpload(1920, 1080, "C:\\Projects\\LDI\\Camera Sim\\temp00.dat", "C:\\Projects\\LDI\\Camera Sim\\out.dat");
	//return 0;

	/*cv::Mat markerImage;
	cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_100);
	cv::aruco::drawMarker(dictionary, 69, 200, markerImage, 1);
	cv::imwrite("marker23.png", markerImage);*/

	runServer();

	// CLI interface
	/*std::string cmd;
	
	while (true) {
		std::getline(std::cin, cmd);
		
		if (cmd.compare("uci") == 0) {
			calibrationImageUploadCmd();
		} else if (cmd.compare("calib") == 0) {
			calibrationProcessCmd();
		} else if (cmd.compare("pose") == 0) {
			findPoseCmd();
		} else if (cmd.compare("exit") == 0) {
			break;
		}
	}*/

	std::cout << "Native core completed\n" << std::flush;

	return 0;
}