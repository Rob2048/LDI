#pragma once

#include <fstream>

// Things to investigate:
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

struct ldiStereoPair {
	mat4 cam0world;
	mat4 cam1world;

	int sampleId;
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
	std::vector<vec3> stCubePoints;
	std::vector<mat4> stCubeWorlds;
	mat4 stStereoCamWorld[2];

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
	// Other data (Caluclate stereo extrinsics, bundle adjust, etc). Not saved.
	//----------------------------------------------------------------------------------------------------
	cv::Mat rtMat[2];

	std::vector<int> massModelPointIds;
	std::vector<vec3> massModelTriangulatedPoints;
	std::vector<vec3> massModelTriangulatedPointsBundleAdjust;
	std::vector<int> massModelBundleAdjustPointIds;
	std::vector<vec2> massModelImagePoints[2];
	std::vector<vec2> massModelUndistortedPoints[2];

	std::vector<std::vector<vec3>> centeredPointGroups;

	std::vector<vec3> cubePointCentroids;
	std::vector<int> cubePointCounts;

	std::vector<ldiPlane> baCubePlanes;
	std::vector<vec3> baIndvCubePoints;
	std::vector<mat4> baViews;
	std::vector<int> baViewIds;
	std::vector<ldiStereoPair> baStereoPairs;
	mat4 baStereoCamWorld[2];

	std::vector<vec3> axisCPoints;
	ldiPlane axisCPlane;
	std::vector<vec3> axisCPointsPlaneProjected;
	ldiCircle axisCCircle;

	std::vector<vec3> axisAPoints;
	ldiPlane axisAPlane;
	std::vector<vec3> axisAPointsPlaneProjected;
	ldiCircle axisACircle;

	std::vector<std::vector<vec2>> scanPoints[2];
	std::vector<ldiLine> scanRays[2];
	std::vector<vec3> scanWorldPoints[2];
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

struct ldiCalibCubeSide {
	int id;
	vec3 corners[4];
	ldiPlane plane;
};

auto _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

ldiImage _calibrationTargetImage;
float _calibrationTargetFullWidthCm;
float _calibrationTargetFullHeightCm;
std::vector<vec3> _calibrationTargetLocalPoints;

std::vector<ldiCalibCubePoint> _cubeWorldPoints;
std::vector<vec3> _cubeGlobalPoints;
std::vector<cv::Point3f> _refinedModelPoints;
vec3 _refinedModelCentroid;
std::vector<ldiCalibCubeSide> _refinedModelSides;
vec3 _refinedModelCorners[8];

void computerVisionFitPlane(std::vector<vec3>& Points, ldiPlane* ResultPlane);

void initCubePoints() {
	_cubeWorldPoints.clear();
	_cubeGlobalPoints.clear();
	_cubeGlobalPoints.resize(9*6, vec3(0.0f, 0.0f, 0.0f));

	float ps = 0.9f;
	// Board 0.
	_cubeWorldPoints.push_back({ 0, 0, 0, { 0.0f, ps * 0, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 3, { 0.0f, ps * 0, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 6, { 0.0f, ps * 0, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0, 1, { 0.0f, ps * 1, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 4, { 0.0f, ps * 1, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 7, { 0.0f, ps * 1, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0, 2, { 0.0f, ps * 2, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 5, { 0.0f, ps * 2, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 8, { 0.0f, ps * 2, -ps * 2 } });

	// Board 1.
	_cubeWorldPoints.push_back({ 0, 0, 15, { -ps * 0 - 1.1f, 2.9f, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 16, { -ps * 0 - 1.1f, 2.9f, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 17, { -ps * 0 - 1.1f, 2.9f, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0, 12, { -ps * 1 - 1.1f, 2.9f, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 13, { -ps * 1 - 1.1f, 2.9f, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 14, { -ps * 1 - 1.1f, 2.9f, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0,  9, { -ps * 2 - 1.1f, 2.9f, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 10, { -ps * 2 - 1.1f, 2.9f, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 11, { -ps * 2 - 1.1f, 2.9f, -ps * 2 } });

	// Board 3.
	_cubeWorldPoints.push_back({ 0, 0, 27, { -ps * 0 - 1.1f, ps * 0, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 28, { -ps * 1 - 1.1f, ps * 0, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 29, { -ps * 2 - 1.1f, ps * 0, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 30, { -ps * 0 - 1.1f, ps * 1, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 31, { -ps * 1 - 1.1f, ps * 1, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 32, { -ps * 2 - 1.1f, ps * 1, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 33, { -ps * 0 - 1.1f, ps * 2, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 34, { -ps * 1 - 1.1f, ps * 2, -2.9f + 4.0f } });
	_cubeWorldPoints.push_back({ 0, 0, 35, { -ps * 2 - 1.1f, ps * 2, -2.9f + 4.0f } });

	// Board 4.
	_cubeWorldPoints.push_back({ 0, 0, 44, { -4.0f, ps * 0, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 43, { -4.0f, ps * 0, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 42, { -4.0f, ps * 0, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0, 41, { -4.0f, ps * 1, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 40, { -4.0f, ps * 1, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 39, { -4.0f, ps * 1, -ps * 2 } });
	_cubeWorldPoints.push_back({ 0, 0, 38, { -4.0f, ps * 2, -ps * 0 } });
	_cubeWorldPoints.push_back({ 0, 0, 37, { -4.0f, ps * 2, -ps * 1 } });
	_cubeWorldPoints.push_back({ 0, 0, 36, { -4.0f, ps * 2, -ps * 2 } });

	// Board 5.
	_cubeWorldPoints.push_back({ 0, 0, 45, { -ps * 0 - 1.1f, ps * 0, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 48, { -ps * 1 - 1.1f, ps * 0, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 51, { -ps * 2 - 1.1f, ps * 0, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 46, { -ps * 0 - 1.1f, ps * 1, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 49, { -ps * 1 - 1.1f, ps * 1, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 52, { -ps * 2 - 1.1f, ps * 1, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 47, { -ps * 0 - 1.1f, ps * 2, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 50, { -ps * 1 - 1.1f, ps * 2, -2.9f } });
	_cubeWorldPoints.push_back({ 0, 0, 53, { -ps * 2 - 1.1f, ps * 2, -2.9f } });

	for (size_t i = 0; i < _cubeWorldPoints.size(); ++i) {
		_cubeGlobalPoints[_cubeWorldPoints[i].globalId] = _cubeWorldPoints[i].position;
	}

	_refinedModelPoints.resize(54, cv::Point3f(0, 0, 0));
	_refinedModelPoints[0] = cv::Point3f(-1.98825, 1.29859, -0.939209);
	_refinedModelPoints[1] = cv::Point3f(-1.98738, 2.1859, -0.945938);
	_refinedModelPoints[2] = cv::Point3f(-1.98767, 3.07523, -0.955707);
	_refinedModelPoints[3] = cv::Point3f(-1.98773, 1.29276, -1.834);
	_refinedModelPoints[4] = cv::Point3f(-1.98753, 2.17956, -1.84425);
	_refinedModelPoints[5] = cv::Point3f(-1.98762, 3.06817, -1.85376);
	_refinedModelPoints[6] = cv::Point3f(-1.98763, 1.28601, -2.73269);
	_refinedModelPoints[7] = cv::Point3f(-1.98734, 2.17282, -2.74178);
	_refinedModelPoints[8] = cv::Point3f(-1.98832, 3.06141, -2.75162);
	_refinedModelPoints[9] = cv::Point3f(-4.8767, 4.19408, -0.971789);
	_refinedModelPoints[10] = cv::Point3f(-4.88696, 4.19076, -1.86617);
	_refinedModelPoints[11] = cv::Point3f(-4.89669, 4.1887, -2.76067);
	_refinedModelPoints[12] = cv::Point3f(-3.98516, 4.19448, -0.983547);
	_refinedModelPoints[13] = cv::Point3f(-3.99326, 4.19135, -1.87982);
	_refinedModelPoints[14] = cv::Point3f(-4.00884, 4.18812, -2.77485);
	_refinedModelPoints[15] = cv::Point3f(-3.0932, 4.19549, -0.99768);
	_refinedModelPoints[16] = cv::Point3f(-3.10409, 4.19249, -1.89118);
	_refinedModelPoints[17] = cv::Point3f(-3.11445, 4.1881, -2.78841);

	_refinedModelPoints[27] = cv::Point3f(-3.12042, 1.33457, 0.162105);
	_refinedModelPoints[28] = cv::Point3f(-4.00706, 1.33349, 0.156452);
	_refinedModelPoints[29] = cv::Point3f(-4.89682, 1.33046, 0.153138);
	_refinedModelPoints[30] = cv::Point3f(-3.1226, 2.22426, 0.163952);
	_refinedModelPoints[31] = cv::Point3f(-4.01133, 2.22106, 0.157525);
	_refinedModelPoints[32] = cv::Point3f(-4.90042, 2.21698, 0.152692);
	_refinedModelPoints[33] = cv::Point3f(-3.1283, 3.11603, 0.162232);
	_refinedModelPoints[34] = cv::Point3f(-4.01666, 3.11253, 0.158036);
	_refinedModelPoints[35] = cv::Point3f(-4.90552, 3.10918, 0.151275);
	_refinedModelPoints[36] = cv::Point3f(-5.98213, 3.09321, -2.7856);
	_refinedModelPoints[37] = cv::Point3f(-5.98253, 3.10323, -1.88828);
	_refinedModelPoints[38] = cv::Point3f(-5.98069, 3.11076, -0.989514);
	_refinedModelPoints[39] = cv::Point3f(-5.98624, 2.21062, -2.77497);
	_refinedModelPoints[40] = cv::Point3f(-5.9829, 2.21998, -1.87861);
	_refinedModelPoints[41] = cv::Point3f(-5.9854, 2.23115, -0.982124);
	_refinedModelPoints[42] = cv::Point3f(-5.9854, 1.32872, -2.7628);
	_refinedModelPoints[43] = cv::Point3f(-5.98286, 1.33622, -1.86702);
	_refinedModelPoints[44] = cv::Point3f(-5.98201, 1.34486, -0.974018);
	_refinedModelPoints[45] = cv::Point3f(-3.03964, 1.30058, -3.83264);
	_refinedModelPoints[46] = cv::Point3f(-3.0557, 2.1838, -3.83887);
	_refinedModelPoints[47] = cv::Point3f(-3.07218, 3.06857, -3.84363);
	_refinedModelPoints[48] = cv::Point3f(-3.93381, 1.28422, -3.8304);
	_refinedModelPoints[49] = cv::Point3f(-3.9503, 2.16706, -3.8348);
	_refinedModelPoints[50] = cv::Point3f(-3.96661, 3.0545, -3.84084);
	_refinedModelPoints[51] = cv::Point3f(-4.82765, 1.26639, -3.8256);
	_refinedModelPoints[52] = cv::Point3f(-4.84587, 2.15218, -3.83292);
	_refinedModelPoints[53] = cv::Point3f(-4.8611, 3.03663, -3.83663);

	vec3 centroidAccum(0.0f, 0.0f, 0.0f);
	int centroidCount = 0;

	for (size_t i = 0; i < _refinedModelPoints.size(); ++i) {
		if (i >= 18 && i <= 26) {
			continue;
		}

		_refinedModelCentroid += toVec3(_refinedModelPoints[i]);
		++centroidCount;
	}
	_refinedModelCentroid /= (float)centroidCount;

	_refinedModelSides.resize(6);
	for (int i = 0; i < 6; ++i) {
		_refinedModelSides[i].id = i;

		if (i == 2) {
			continue;
		}

		std::vector<vec3> points;

		for (int j = i * 9; j < (i + 1) * 9; ++j) {
			points.push_back(toVec3(_refinedModelPoints[j]));
		}

		computerVisionFitPlane(points, &_refinedModelSides[i].plane);
	}
	ldiCalibCubeSide bottom = {};
	bottom.id = 2;
	bottom.plane = _refinedModelSides[1].plane;
	bottom.plane.point += bottom.plane.normal * 4.0f;
	_refinedModelSides[2] = bottom;

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[1].plane, _refinedModelSides[0].plane, _refinedModelSides[3].plane, &_refinedModelCorners[0])) {
		 // Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[1].plane, _refinedModelSides[0].plane, _refinedModelSides[5].plane, &_refinedModelCorners[1])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[1].plane, _refinedModelSides[4].plane, _refinedModelSides[5].plane, &_refinedModelCorners[2])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[1].plane, _refinedModelSides[4].plane, _refinedModelSides[3].plane, &_refinedModelCorners[3])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[2].plane, _refinedModelSides[0].plane, _refinedModelSides[3].plane, &_refinedModelCorners[4])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[2].plane, _refinedModelSides[0].plane, _refinedModelSides[5].plane, &_refinedModelCorners[5])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[2].plane, _refinedModelSides[4].plane, _refinedModelSides[5].plane, &_refinedModelCorners[6])) {
		// Cube failed.
	}

	if (getPointAtIntersectionOfPlanes(_refinedModelSides[2].plane, _refinedModelSides[4].plane, _refinedModelSides[3].plane, &_refinedModelCorners[7])) {
		// Cube failed.
	}

	_refinedModelSides[0].corners[0] = _refinedModelCorners[0];
	_refinedModelSides[0].corners[1] = _refinedModelCorners[1];
	_refinedModelSides[0].corners[2] = _refinedModelCorners[5];
	_refinedModelSides[0].corners[3] = _refinedModelCorners[4];

	_refinedModelSides[1].corners[0] = _refinedModelCorners[0];
	_refinedModelSides[1].corners[1] = _refinedModelCorners[3];
	_refinedModelSides[1].corners[2] = _refinedModelCorners[2];
	_refinedModelSides[1].corners[3] = _refinedModelCorners[1];

	_refinedModelSides[2].corners[0] = _refinedModelCorners[4];
	_refinedModelSides[2].corners[1] = _refinedModelCorners[5];
	_refinedModelSides[2].corners[2] = _refinedModelCorners[6];
	_refinedModelSides[2].corners[3] = _refinedModelCorners[7];

	_refinedModelSides[3].corners[0] = _refinedModelCorners[3];
	_refinedModelSides[3].corners[1] = _refinedModelCorners[0];
	_refinedModelSides[3].corners[2] = _refinedModelCorners[4];
	_refinedModelSides[3].corners[3] = _refinedModelCorners[7];

	_refinedModelSides[4].corners[0] = _refinedModelCorners[2];
	_refinedModelSides[4].corners[1] = _refinedModelCorners[3];
	_refinedModelSides[4].corners[2] = _refinedModelCorners[7];
	_refinedModelSides[4].corners[3] = _refinedModelCorners[6];

	_refinedModelSides[5].corners[0] = _refinedModelCorners[1];
	_refinedModelSides[5].corners[1] = _refinedModelCorners[2];
	_refinedModelSides[5].corners[2] = _refinedModelCorners[6];
	_refinedModelSides[5].corners[3] = _refinedModelCorners[5];

	std::cout << glm::distance(_refinedModelCorners[0], _refinedModelCorners[3]) << "\n";
	std::cout << glm::distance(_refinedModelCorners[3], _refinedModelCorners[2]) << "\n";
	std::cout << glm::distance(_refinedModelCorners[2], _refinedModelCorners[1]) << "\n";
	std::cout << glm::distance(_refinedModelCorners[1], _refinedModelCorners[0]) << "\n";
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

//bool computerVisionFindGeneralPoseOutliers(cv::Mat* CameraMatrix, cv::Mat* DistCoeffs, std::vector<cv::Point2f>* ImagePoints, std::vector<cv::Point3f>* WorldPoints) {
//	std::vector<cv::Mat> rvecs;
//	std::vector<cv::Mat> tvecs;
//	std::vector<float> rms;
//
//	int solutionCount = cv::solvePnPGeneric(*WorldPoints, *ImagePoints, *CameraMatrix, *DistCoeffs, rvecs, tvecs, false, cv::SOLVEPNP_ITERATIVE, cv::noArray(), cv::noArray(), rms);
//	//std::cout << "  Solutions: " << solutionCount << "\n";
//	// TODO: Why do we set solutionCount to this?
//	solutionCount = rvecs.size();
//	//std::cout << "  Solutions: " << solutionCount << "\n";
//
//	for (int i = 0; i < solutionCount; ++i) {
//		*RVec = rvecs[i];
//		*TVec = tvecs[i];
//
//		std::cout << "  Found pose (" << i << "): " << rms[i] << "\n";
//
//		if (rms[i] > 3.0) {
//			return false;
//		}
//	}
//
//	if (solutionCount > 0) {
//		return true;
//	}
//
//	return false;
//}

ldiLine computerVisionFitLine(std::vector<vec3>& Points) {
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

		ldiLine lineFit = computerVisionFitLine(fitPoints);
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

		ldiLine lineFit = computerVisionFitLine(fitPoints);
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
		cv::Mat calibCameraMatrix = Job->defaultCamMat[i];
		cv::Mat calibCameraDist = Job->defaultCamDist[i];
		
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

		Job->refinedCamMat[i] = calibCameraMatrix;
		Job->refinedCamDist[i] = calibCameraDist;
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
	sampleMat.resize(1000);

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

	return;

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
				vec3 targetPoint = _cubeGlobalPoints[cornerGlobalId];

				imagePoints.push_back(cv::Point2f(corner->position.x, corner->position.y));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);

				//std::cout << "        Corner " << cornerIter << " (" << cornerGlobalId << ") " << corner->position.x << ", " << corner->position.y << " " << targetPoint.x << ", " << targetPoint.y << ", " << targetPoint.z << "\n";
			}
		}

		if (imagePoints.size() >= 6) {
			mat4 pose;

			std::cout << "Find pose - Sample: " << sampleIter << " :\n";
			if (computerVisionFindGeneralPose(&Job->defaultCamMat[hawkId], &Job->defaultCamDist[hawkId], &imagePoints, &worldPoints, &pose)) {
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
	fprintf(f, "%d %d %d\n", _cubeWorldPoints.size(), poses.size(), totalImagePointCount);

	// Camera intrinsics.
	cv::Mat calibCameraMatrix = Job->defaultCamMat[hawkId];
	cv::Mat calibCameraDist = Job->defaultCamDist[hawkId];

	fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
		calibCameraMatrix.at<double>(0), calibCameraMatrix.at<double>(1), calibCameraMatrix.at<double>(2), calibCameraMatrix.at<double>(4), calibCameraMatrix.at<double>(5),
		calibCameraDist.at<double>(0), calibCameraDist.at<double>(1), calibCameraDist.at<double>(2), calibCameraDist.at<double>(3));

	// 3D points.
	for (size_t pointIter = 0; pointIter < _cubeWorldPoints.size(); ++pointIter) {
		int pointId = _cubeWorldPoints[pointIter].globalId;
		vec3 point = _cubeWorldPoints[pointIter].position;
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

	vec3 cubePoints[9 * 6];

	for (int i = 0; i < modelPointCount; ++i) {
		vec3 position;
		int pointId;

		fscanf_s(file, "%d %f %f %f\r\n", &pointId, &position[0], &position[1], &position[2]);
		//std::cout << pointId << " " << position.x << ", " << position.y << ", " << position.z << "\n";

		//Job->baIndvCubePoints.push_back(position);

		cubePoints[pointId] = position;
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

	//----------------------------------------------------------------------------------------------------
	// Get scale.
	//----------------------------------------------------------------------------------------------------
	float scaleAccum = 0.0f;
	int scaleAccumCount = 0;

	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		if (boardIter == 2) {
			continue;
		}

		int globalIdBase = boardIter * 9;

		// Match points: 0-1, 1-2, 3-4, 4-5, 6-7, 7-8,
		// 0-3, 3-6, 1-4, 4-7, 2-5, 5-8

		for (int i = 0; i < 3; ++i) {
			// Rows
			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
				int pId0 = globalIdBase + (i * 3) + pairIter + 0;
				int pId1 = globalIdBase + (i * 3) + pairIter + 1;

				float dist = glm::distance(cubePoints[pId0], cubePoints[pId1]);
				//std::cout << "Row (Board " << boardIter << ") " << pId0 << " - " << pId1 << ": " << dist << "\n";
				scaleAccum += dist;
				scaleAccumCount += 1;
			}

			// Columns
			for (int pairIter = 0; pairIter < 3 - 1; ++pairIter) {
				int pId0 = globalIdBase + i + (pairIter * 3) + 0;
				int pId1 = globalIdBase + i + (pairIter * 3) + 3;

				float dist = glm::distance(cubePoints[pId0], cubePoints[pId1]);
				//std::cout << "Col (Board " << boardIter << ") " << pId0 << " - " << pId1 << ": " << dist << "\n";
				scaleAccum += dist;
				scaleAccumCount += 1;
			}
		}
	}

	float scaleAvg = scaleAccum / (float)scaleAccumCount;
	float invScale = 0.9 / scaleAvg;
	std::cout << "Scale avg: " << scaleAvg << "\n";

	// NOTE
	//for (int i = 0; i < 9 * 6; ++i) {
		//cubePoints[i] *= invScale;
	//}

	//----------------------------------------------------------------------------------------------------
	// Calculate cube metrics.
	//----------------------------------------------------------------------------------------------------
	Job->baCubePlanes.clear();

	for (int boardIter = 0; boardIter < 6; ++boardIter) {
		if (boardIter == 2) {
			continue;
		}

		std::vector<vec3> points;
		points.reserve(9);

		for (int pointIter = boardIter * 9; pointIter < (boardIter + 1) * 9; ++pointIter) {
			points.push_back(cubePoints[pointIter]);
		}

		ldiPlane plane;
		computerVisionFitPlane(points, &plane);
		Job->baCubePlanes.push_back(plane);

		std::cout << "Board: " << boardIter << " " << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z << "\n";
	}

	float dist03 = glm::distance(Job->baCubePlanes[0].point, Job->baCubePlanes[3].point);
	float dist24 = glm::distance(Job->baCubePlanes[2].point, Job->baCubePlanes[4].point);
	std::cout << "Side dists: " << dist03 << " " << dist24 << "\n";

	//----------------------------------------------------------------------------------------------------
	// Calc basis.
	//----------------------------------------------------------------------------------------------------
	vec3 axisX = Job->baCubePlanes[0].normal;
	vec3 axisY = Job->baCubePlanes[1].normal;
	vec3 axisZ = Job->baCubePlanes[2].normal;

	vec3 basisX = axisX;
	vec3 basisY = glm::normalize(-glm::cross(basisX, axisZ));
	vec3 basisZ = glm::normalize(glm::cross(basisX, basisY));

	mat4 worldToVolume = glm::identity<mat4>();

	worldToVolume[0] = vec4(basisX, 0.0f);
	worldToVolume[1] = vec4(basisY, 0.0f);
	worldToVolume[2] = vec4(basisZ, 0.0f);

	mat4 volumeToWorld = glm::inverse(worldToVolume);

	for (int i = 0; i < 9 * 6; ++i) {
		cubePoints[i] = volumeToWorld * vec4(cubePoints[i], 1.0);
		Job->baIndvCubePoints.push_back(cubePoints[i]);
	}

	vec3 cubeCentroid(0, 0, 0.0f);
	int centroidCount = 0;

	for (int i = 0; i < 9 * 6; ++i) {
		if (i >= 18 && i <=26) {
			continue;
		}

		cubeCentroid += cubePoints[i];
		++centroidCount;
	}

	cubeCentroid /= centroidCount;
	std::cout << "Count: " << centroidCount << " Center: " << cubeCentroid.x << ", " << cubeCentroid.y << ", " << cubeCentroid.z << "\n";

	for (int i = 0; i < 9 * 6; ++i) {
		cubePoints[i] -= cubeCentroid;
	}

	//----------------------------------------------------------------------------------------------------
	// Output refined cube.
	//----------------------------------------------------------------------------------------------------
	for (int i = 0; i < 9 * 6; ++i) {
		vec3 point = cubePoints[i];
		std::cout << "tempModelPoints[" << i << "] = cv::Point3f(" << point.x << ", " << point.y << ", " << point.z << ");\n";
	}

	return true;
}

void computerVisionBundleAdjustStereoBoth(ldiCalibrationJob* Job) {
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
					vec3 targetPoint = _cubeGlobalPoints[cornerGlobalId];

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
				if (computerVisionFindGeneralPose(&Job->defaultCamMat[hawkIter], &Job->defaultCamDist[hawkIter], &imagePoints, &worldPoints, &pose)) {
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
	fprintf(f, "%d %d %d\n", _cubeWorldPoints.size(), poses.size(), totalImagePointCount);

	// Camera intrinsics.
	for (size_t i = 0; i < poseOwner.size(); ++i) {
		int hawkId = poseOwner[i];
		cv::Mat calibCameraMatrix = Job->defaultCamMat[hawkId];
		cv::Mat calibCameraDist = Job->defaultCamDist[hawkId];

		fprintf(f, "%f %f %f %f %f %f %f %f %f\n",
			calibCameraMatrix.at<double>(0), calibCameraMatrix.at<double>(1), calibCameraMatrix.at<double>(2), calibCameraMatrix.at<double>(4), calibCameraMatrix.at<double>(5),
			calibCameraDist.at<double>(0), calibCameraDist.at<double>(1), calibCameraDist.at<double>(2), calibCameraDist.at<double>(3));
	}

	// 3D points.
	for (size_t pointIter = 0; pointIter < _cubeWorldPoints.size(); ++pointIter) {
		int pointId = _cubeWorldPoints[pointIter].globalId;
		vec3 point = _cubeWorldPoints[pointIter].position;
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