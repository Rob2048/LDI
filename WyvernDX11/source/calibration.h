#pragma once

// Bundle adjust and non-linear optimization:
 
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

// NOTE: Really in-depth camera calibration: http://mrcal.secretsauce.net/how-to-calibrate.html

// Takes stereo image sample files and generates the initial calibration samples for a job.
void calibFindInitialObservations(ldiCalibrationJob* Job, const std::string& DirectoryPath) {
	calibClearJob(Job);

	Job->camMat = cv::Mat::eye(3, 3, CV_64F);
	Job->camMat.at<double>(0, 0) = 2666.92;
	Job->camMat.at<double>(0, 1) = 0.0;
	Job->camMat.at<double>(0, 2) = 1704.76;
	Job->camMat.at<double>(1, 0) = 0.0;
	Job->camMat.at<double>(1, 1) = 2683.62;
	Job->camMat.at<double>(1, 2) = 1294.87;
	Job->camMat.at<double>(2, 0) = 0.0;
	Job->camMat.at<double>(2, 1) = 0.0;
	Job->camMat.at<double>(2, 2) = 1.0;

	Job->camDist = cv::Mat::zeros(8, 1, CV_64F);
	Job->camDist.at<double>(0) = 0.1937769949436188;
	Job->camDist.at<double>(1) = -0.3512980043888092;
	Job->camDist.at<double>(2) = 0.002319999970495701;
	Job->camDist.at<double>(3) = 0.00217368989251554;

	std::vector<std::string> filePaths = listAllFilesInDirectory(DirectoryPath);

	std::cout << "Filepaths: " << DirectoryPath << "\n";

	for (int i = 0; i < filePaths.size(); ++i) {
		if (endsWith(filePaths[i], ".dat")) {
			std::cout << "Check file " << (i + 1) << "/" << filePaths.size() << " " << filePaths[i] << "\n";

			ldiCalibSample sample = {};
			sample.path = filePaths[i];

			calibLoadCalibSampleData(&sample);
			computerVisionFindCharuco(sample.frame, &sample.cube, &Job->camMat, &Job->camDist);
			calibFreeCalibImages(&sample);

			Job->samples.push_back(sample);
		}
	}

	std::cout << "Initial observations complete\n";
}

void calibRealignWorldVolume(ldiCalibrationJob* Job, mat4 CubeOffset = mat4(1.0)) {
	// Align basis to world as reasonably as possible.
	Job->basisX = Job->axisX.direction;
	Job->basisY = glm::normalize(glm::cross(Job->basisX, Job->axisZ.direction));
	Job->basisZ = glm::normalize(glm::cross(Job->basisX, Job->basisY));

	mat4 worldToVolume = glm::identity<mat4>();
	worldToVolume[0] = vec4(Job->basisX, 0.0f);
	worldToVolume[1] = vec4(Job->basisY, 0.0f);
	worldToVolume[2] = vec4(Job->basisZ, 0.0f);

	mat4 volumeRealign = glm::rotate(mat4(1.0f), glm::radians(90.0f), Job->basisX);
	worldToVolume = volumeRealign * worldToVolume;

	vec3 originTrans = CubeOffset[3];
	std::cout << "Offset from center: " << originTrans.x << ", " << originTrans.y << ", " << originTrans.z << "\n";

	volumeRealign = glm::translate(mat4(1.0f), originTrans);
	worldToVolume = volumeRealign * worldToVolume;
	mat4 volumeToWorld = glm::inverse(worldToVolume);
	std::cout << "volumeToWorld: " << GetStr(&volumeToWorld) << "\n";

	// Transfrom cube points to make the 0,0,0 cube the zero.
	mat4 cubeTransform = volumeToWorld * CubeOffset;
	//std::cout << "Zero transform: " << GetStr(&zeroTransform) << "\n";
	calibCubeTransform(&Job->cube, cubeTransform);

	//----------------------------------------------------------------------------------------------------
	// Transform all volume elements into world space.
	//----------------------------------------------------------------------------------------------------
	{
		// Axis directions.
		Job->axisX.direction = glm::normalize(volumeToWorld * vec4(Job->axisX.direction, 0.0f));
		Job->axisY.direction = glm::normalize(volumeToWorld * vec4(Job->axisY.direction, 0.0f));
		Job->axisZ.direction = glm::normalize(volumeToWorld * vec4(Job->axisZ.direction, 0.0f));

		for (size_t i = 0; i < Job->axisXPoints.size(); ++i) {
			Job->axisXPoints[i] = volumeToWorld * vec4(Job->axisXPoints[i], 0.0f);
		}

		for (size_t i = 0; i < Job->axisYPoints.size(); ++i) {
			Job->axisYPoints[i] = volumeToWorld * vec4(Job->axisYPoints[i], 0.0f);
		}

		for (size_t i = 0; i < Job->axisZPoints.size(); ++i) {
			Job->axisZPoints[i] = volumeToWorld * vec4(Job->axisZPoints[i], 0.0f);
		}

		// Cameras to world space.
		Job->camVolumeMat = volumeToWorld * Job->camVolumeMat;
		
		// Normalize camera scale.
		Job->camVolumeMat[0] = vec4(glm::normalize(vec3(Job->camVolumeMat[0])), 0.0f);
		Job->camVolumeMat[1] = vec4(glm::normalize(vec3(Job->camVolumeMat[1])), 0.0f);
		Job->camVolumeMat[2] = vec4(glm::normalize(vec3(Job->camVolumeMat[2])), 0.0f);

		// Cube poses.
		for (size_t i = 0; i < Job->cubePoses.size(); ++i) {
			Job->cubePoses[i] = volumeToWorld * Job->cubePoses[i];
		}
		
		// Axis C points.
		for (size_t i = 0; i < Job->axisCPoints.size(); ++i) {
			Job->axisCPoints[i] = volumeToWorld * vec4(Job->axisCPoints[i], 1.0f);
		}

		// Axis A points.
		for (size_t i = 0; i < Job->axisAPoints.size(); ++i) {
			Job->axisAPoints[i] = volumeToWorld * vec4(Job->axisAPoints[i], 1.0f);
		}

		Job->axisC.origin = volumeToWorld * vec4(Job->axisC.origin, 1.0f);
		Job->axisC.direction = volumeToWorld * vec4(Job->axisC.direction, 0.0f);

		if (Job->axisC.direction.y < 0.0f) {
			Job->axisC.direction = -Job->axisC.direction;
		}

		//	std::cout << "Axis A - Origin: " << GetStr(Job->axisA.origin) << " Dir: " << GetStr(Job->axisA.direction) << "\n";

		Job->axisA.origin = volumeToWorld * vec4(Job->axisA.origin, 1.0f);
		Job->axisA.direction = volumeToWorld * vec4(-Job->axisA.direction, 0.0f);

		if (Job->axisA.direction.x > 0.0f) {
			Job->axisA.direction = -Job->axisA.direction;
		}
	}
}

// Determine metrics for the calibration volume.
void calibBuildCalibVolumeMetrics(ldiCalibrationJob* Job) {
	Job->metricsCalculated = false;

	if (!Job->initialEstimations) {
		std::cout << "Can't calculate metrics, no initial estimations\n";
		return;
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

	// TODO: Use view error to decide if view should be included.
	for (size_t sampleIter = 0; sampleIter < Job->poseToSampleIds.size(); ++sampleIter) {
		ldiCalibSample* sample = &Job->samples[Job->poseToSampleIds[sampleIter]];

		if (sample->phase != 1) {
			continue;
		}

		if (sample->A != 0 || sample->C != 13000) {
			continue;
		}

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
			pose.pose = Job->cubePoses[sampleIter];
			columnX[columnId].poses.push_back(pose);

			//std::cout << "(X) View: " << sampleIter << " Sample: " << stereoSampleId[sampleIter] <<  " Mech: " << x << "," << y << "," << z << "\n";
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
			pose.pose = Job->cubePoses[sampleIter];
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
			pose.pose = Job->cubePoses[sampleIter];
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

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
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

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
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

					//std::cout << "Cube point dist: " << distVolSpace << "/" << cubeMechDist << " = " << distNorm << "\n";
				}
			}
		}

		distAvgZ = distAccum / (double)distAccumCount;
	}

	std::cout << "Dist avg X: " << distAvgX << "\n";
	std::cout << "Dist avg Y: " << distAvgY << "\n";
	std::cout << "Dist avg Z: " << distAvgZ << "\n";

	//----------------------------------------------------------------------------------------------------
	// Find basis directions.
	//----------------------------------------------------------------------------------------------------
	Job->axisXPoints.clear();
	Job->axisYPoints.clear();
	Job->axisZPoints.clear();

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
				Job->axisXPoints.push_back(transPoint);
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
				Job->axisYPoints.push_back(transPoint);
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
				Job->axisZPoints.push_back(transPoint);
			}
		}
	}

	Job->axisX = computerVisionFitLine(Job->axisXPoints);
	Job->axisY = computerVisionFitLine(Job->axisYPoints);
	Job->axisZ = computerVisionFitLine(Job->axisZPoints);

	//----------------------------------------------------------------------------------------------------
	// Gather rotary axis samples.
	//----------------------------------------------------------------------------------------------------
	Job->axisCPoints.clear();
	Job->axisAPoints.clear();

	// Just for visualization.
	for (size_t poseIter = 0; poseIter < Job->poseToSampleIds.size(); ++poseIter) {
		ldiCalibSample* sample = &Job->samples[Job->poseToSampleIds[poseIter]];

		if (sample->phase == 2) {
			vec3 transPoint = Job->cubePoses[poseIter][3];
			Job->axisCPoints.push_back(transPoint);
		} else if (sample->phase == 3) {
			vec3 transPoint = Job->cubePoses[poseIter][3];
			Job->axisAPoints.push_back(transPoint);
		}
	}

	std::vector<vec3> circOriginsC;
	std::vector<vec3> circOriginsA;

	for (size_t pointIter = 0; pointIter < Job->cube.points.size(); ++pointIter) {
		if (pointIter / 9 == 2) {
			continue;
		}

		std::vector<vec3d> axisPointsC;
		std::vector<vec3d> axisPointsA;

		for (size_t poseIter = 0; poseIter < Job->poseToSampleIds.size(); ++poseIter) {
			ldiCalibSample* sample = &Job->samples[Job->poseToSampleIds[poseIter]];

			if (sample->phase == 2) {
				vec3d transPoint = mat4d(Job->cubePoses[poseIter]) * vec4d(Job->cube.points[pointIter], 1.0);
				axisPointsC.push_back(transPoint);
			} else if (sample->phase == 3) {
				vec3d transPoint = mat4d(Job->cubePoses[poseIter]) * vec4d(Job->cube.points[pointIter], 1.0);
				axisPointsA.push_back(transPoint);
			}
		}

		circOriginsC.push_back(computerVisionFitCircle(axisPointsC).origin);
		circOriginsA.push_back(computerVisionFitCircle(axisPointsA).origin);
	}

	ldiLine fitC = computerVisionFitLine(circOriginsC);
	Job->axisC.direction = fitC.direction;
	Job->axisC.origin = fitC.origin;
	
	ldiLine fitA = computerVisionFitLine(circOriginsA);
	Job->axisA.direction = fitA.direction;
	Job->axisA.origin = fitA.origin;

	//----------------------------------------------------------------------------------------------------
	// Calculate calib volume to world basis.
	//----------------------------------------------------------------------------------------------------
	// Find zero sample. (Phase 0).
	mat4 samp0;
	bool foundSample0 = false;

	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		ldiCalibSample* sample = &Job->samples[sampleIter];

		if (sample->phase == 0) {
			samp0 = Job->cubePoses[sampleIter];
			foundSample0 = true;
			std::cout << "Zero sample: " << sampleIter << "\n";
			break;
		}
	}

	if (!foundSample0) {
		std::cout << "Could not find zero sample.\n";
		return;
	}

	calibRealignWorldVolume(Job, samp0);
	
	Job->metricsCalculated = true;

	//std::cout << "Zero cube world: " << GetStr(&Job->cubeWorlds[0]) << "\n";
}

double calibGetProjectionRMSE(ldiCalibrationJob* Job) {
	std::vector<cv::Point3f> projPoints;

	Job->projObs.clear();
	Job->projReproj.clear();
	Job->projError.clear();
	
	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
	//for (size_t sampleIter = 0; sampleIter < Job->initialPoseToSampleIds.size(); ++sampleIter) {
		//ldiCalibStereoSample* sample = &Job->samples[Job->initialPoseToSampleIds[sampleIter]];
		ldiCalibSample* sample = &Job->samples[sampleIter];

		/*if (sample->phase != 3) {
			continue;
		}*/

		ldiHorsePosition pos = {};
		pos.x = sample->X;
		pos.y = sample->Y;
		pos.z = sample->Z;
		pos.a = sample->A;
		pos.c = sample->C;

		std::vector<vec3> points;
		horseGetProjectionCubePoints(Job, pos, points);

		std::vector<ldiCharucoBoard>* boards = &sample->cube.boards;
		for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
			ldiCharucoBoard* board = &(*boards)[boardIter];

			for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
				ldiCharucoCorner* corner = &board->corners[cornerIter];
				int cornerGlobalId = (board->id * 9) + corner->id;
				
				Job->projObs.push_back(corner->position);
				projPoints.push_back(toPoint3f(points[cornerGlobalId]));
			}
		}
	}

	mat4 camMat = glm::inverse(Job->camVolumeMat);
	cv::Mat rtMat = convertTransformToRT(convertGlmTransMatToOpenCvMat(camMat));
	
	cv::Mat r = cv::Mat::zeros(1, 3, CV_64F);
	r.at<double>(0) = rtMat.at<double>(0);
	r.at<double>(1) = rtMat.at<double>(1);
	r.at<double>(2) = rtMat.at<double>(2);

	cv::Mat t = cv::Mat::zeros(1, 3, CV_64F);
	t.at<double>(0) = rtMat.at<double>(3);
	t.at<double>(1) = rtMat.at<double>(4);
	t.at<double>(2) = rtMat.at<double>(5);

	cv::projectPoints(projPoints, r, t, Job->camMat, Job->camDist, Job->projReproj);

	double meanError = 0.0;
	
	for (size_t i = 0; i < Job->projObs.size(); ++i) {
		double err = glm::length(Job->projObs[i] - toVec2(Job->projReproj[i]));
		meanError += err * err;
		Job->projError.push_back(err);
	}

	meanError /= (double)Job->projObs.size();
	meanError = sqrt(meanError);

	std::cout << "Reprojection RMSE: " << meanError << "\n";

	return meanError;
}

// Takes stereo image sample files and generates scanner calibration.
void calibCalibrateScanner(ldiCalibrationJob* Job, const std::string& DirectoryPath) {
	for (size_t i = 0; i < Job->scanSamples.size(); ++i) {
		calibFreeCalibImages(&Job->scanSamples[i]);
	}
	Job->scanSamples.clear();
	
	Job->scannerCalibrated = false;
	Job->scanPoints.clear();
	Job->scanRays.clear();
	Job->scanWorldPoints.clear();

	if (!Job->metricsCalculated) {
		std::cout << "Scanner calibration requires metrics to be calculated\n";
		return;
	}

	std::vector<std::string> filePaths = listAllFilesInDirectory(DirectoryPath);

	for (int i = 0; i < filePaths.size(); ++i) {
		if (endsWith(filePaths[i], ".dat")) {
			ldiCalibSample sample = {};
			sample.path = filePaths[i];

			calibLoadCalibSampleData(&sample);

			// Find machine basis for this sample position.
			ldiHorsePosition horsePos = {};
			horsePos.x = sample.X;
			horsePos.y = sample.Y;
			horsePos.z = sample.Z;
			horsePos.c = sample.C;
			horsePos.a = sample.A;

			std::vector<vec3> cubePoints;
			std::vector<ldiCalibCubeSide> cubeSides;
			std::vector<vec3> cubeCorners;

			horseGetRefinedCubeAtPosition(Job, horsePos, cubePoints, cubeSides, cubeCorners);

			// Plane scan line bounds.
			ldiPlane boundPlanes[4];

			vec3 dir01 = glm::normalize(cubeCorners[1] - cubeCorners[0]);
			vec3 dir12 = glm::normalize(cubeCorners[2] - cubeCorners[1]);
			vec3 dir23 = glm::normalize(cubeCorners[3] - cubeCorners[2]);
			vec3 dir30 = glm::normalize(cubeCorners[0] - cubeCorners[3]);

			boundPlanes[0].normal = dir01;
			boundPlanes[0].point = cubeCorners[0] + dir01 * 0.2f;

			boundPlanes[1].normal = dir12;
			boundPlanes[1].point = cubeCorners[1] + dir12 * 0.01f;

			boundPlanes[2].normal = dir23;
			boundPlanes[2].point = cubeCorners[2] + dir23 * 0.2f;

			boundPlanes[3].normal = dir30;
			boundPlanes[3].point = cubeCorners[3] + dir30 * 3.0f;

			{
				cv::Mat srcImage(cv::Size(sample.frame.width, sample.frame.height), CV_8UC1, sample.frame.data);
				cv::Mat downscaleImage;
				cv::Mat upscaleImage;

				cv::resize(srcImage, downscaleImage, cv::Size(3280 / 2, 2464 / 2));
				cv::resize(downscaleImage, upscaleImage, cv::Size(3280, 2464));

				std::vector<vec2> points = computerVisionFindScanLine({ sample.frame.width / 2, sample.frame.height / 2, downscaleImage.data });
				//std::vector<vec2> points = computerVisionFindScanLine(sample.frame);

				std::vector<cv::Point2f> distortedPoints;
				std::vector<cv::Point2f> undistortedPoints;

				for (size_t pIter = 0; pIter < points.size(); ++pIter) {
					distortedPoints.push_back(toPoint2f(points[pIter]));
				}

				cv::undistortPoints(distortedPoints, undistortedPoints, Job->camMat, Job->camDist, cv::noArray(), Job->camMat);

				for (size_t pIter = 0; pIter < points.size(); ++pIter) {
					points[pIter] = toVec2(undistortedPoints[pIter]);
				}

				Job->scanPoints.push_back(points);

				// Project points against current machine basis.
				ldiCamera camera = horseGetCamera(Job, horsePos, 3280, 2464);

				//if (i == 0) {
				{
					for (size_t pIter = 0; pIter < points.size(); ++pIter) {
						ldiLine ray = screenToRay(&camera, points[pIter]);

						vec3 worldPoint;
						getRayPlaneIntersection(ray, cubeSides[1].plane, worldPoint);

						// Check point within bounds.
						bool pointWithinBounds = true;
						for (int bIter = 0; bIter < 4; ++bIter) {
							if (glm::dot(boundPlanes[bIter].normal, worldPoint - boundPlanes[bIter].point) <= 0.0f){
								pointWithinBounds = false;
								break;
							}
						}

						if (pointWithinBounds) {
							Job->scanWorldPoints.push_back(worldPoint);
							//Job->scanRays[j].push_back(ray);
						}
					}
				}
			}

			calibFreeCalibImages(&sample);

			Job->scanSamples.push_back(sample);
		}
	}

	std::vector<vec3> allWorldPoints;
	allWorldPoints.reserve(Job->scanWorldPoints.size() + Job->scanWorldPoints.size());
	allWorldPoints.insert(allWorldPoints.begin(), Job->scanWorldPoints.begin(), Job->scanWorldPoints.end());
	//allWorldPoints.insert(allWorldPoints.begin() + Job->scanWorldPoints[0].size(), Job->scanWorldPoints[1].begin(), Job->scanWorldPoints[1].end());

	ldiPlane scanPlane;
	computerVisionFitPlane(allWorldPoints, &Job->scanPlane);

	Job->scannerCalibrated = true;
}

void calibGetInitialEstimations(ldiCalibrationJob* Job) {
	// Unknown parameters (That need initial estimations):
	// - X axis direction.
	// - Y axis direction.
	// - Z axis direction.
	// - C axis direction and origin.
	// - A axis direction and origin.
	// - Cube 3D point positions.
	// - Camera intrinsics (camera matrix, lens distortion).
	// - Camera extrinsics (position, rotation).

	ldiCalibCube initialCube;
	calibCubeInit(&initialCube);
	Job->cube = initialCube;

	Job->poseToSampleIds.clear();
	Job->cubePoses.clear();
	Job->camVolumeMat = glm::identity<mat4>();
	Job->initialEstimations = false;

	double stepsToCm = 1.0 / ((200 * 32) / 0.8);
	Job->stepsToCm = glm::f64vec3(stepsToCm, stepsToCm, stepsToCm);
	
	//----------------------------------------------------------------------------------------------------
	// Find a pose for each calibration sample.
	//----------------------------------------------------------------------------------------------------
	std::cout << "Find pose for each sample\n";

	std::vector<std::vector<cv::Point2f>> viewObservations;
	std::vector<std::vector<int>> viewPointIds;
	std::vector<ldiHorsePositionAbs> viewPositions;

	for (size_t sampleIter = 0; sampleIter < Job->samples.size(); ++sampleIter) {
		ldiCalibSample* sample = &Job->samples[sampleIter];
		std::vector<cv::Point2f> imagePoints;
		std::vector<int> imagePointIds;
		std::vector<cv::Point3f> worldPoints;
		
		cv::Mat poseRT;

		std::vector<ldiCharucoBoard>* boards = &sample->cube.boards;
		for (size_t boardIter = 0; boardIter < boards->size(); ++boardIter) {
			ldiCharucoBoard* board = &(*boards)[boardIter];

			for (size_t cornerIter = 0; cornerIter < board->corners.size(); ++cornerIter) {
				ldiCharucoCorner* corner = &board->corners[cornerIter];
				int cornerGlobalId = (board->id * 9) + corner->id;
				vec3 targetPoint = initialCube.points[cornerGlobalId];

				imagePoints.push_back(cv::Point2f(corner->position.x, corner->position.y));
				worldPoints.push_back(cv::Point3f(targetPoint.x, targetPoint.y, targetPoint.z));
				imagePointIds.push_back(cornerGlobalId);
			}
		}

		if (imagePoints.size() >= 6) {
			cv::Mat r;
			cv::Mat t;

			if (computerVisionFindGeneralPoseRT(&Job->camMat, &Job->camDist, &imagePoints, &worldPoints, &r, &t)) {
				std::cout << "Found pose for sample " << sampleIter << "\n";

				poseRT = cv::Mat::zeros(1, 6, CV_64F);
				poseRT.at<double>(0) = r.at<double>(0);
				poseRT.at<double>(1) = r.at<double>(1);
				poseRT.at<double>(2) = r.at<double>(2);
				poseRT.at<double>(3) = t.at<double>(0);
				poseRT.at<double>(4) = t.at<double>(1);
				poseRT.at<double>(5) = t.at<double>(2);

				cv::Mat camTransMat = convertRvecTvec(poseRT);
				//cv::Mat cubeTransMat = camTransMat;//.inv();
				mat4 cubeMat = convertOpenCvTransMatToGlmMat(camTransMat);
				Job->cubePoses.push_back(cubeMat);
				Job->poseToSampleIds.push_back(sampleIter);

				// Add to BA output.
				//if (sample->phase == 3) {
				{
					// Save positions in absolute values.
					ldiHorsePositionAbs platformPos = {};
					platformPos.x = sample->X * Job->stepsToCm.x;
					platformPos.y = sample->Y * Job->stepsToCm.y;
					platformPos.z = sample->Z * Job->stepsToCm.z;
					platformPos.a = sample->A * (360.0 / (32.0 * 200.0 * 90.0));
					platformPos.c = (sample->C - 13000) * (360.0 / (32.0 * 200.0 * 30.0));

					viewObservations.push_back(imagePoints);
					viewPointIds.push_back(imagePointIds);
					viewPositions.push_back(platformPos);
				}
			} else {
				//std::cout << "Find pose - Sample: " << sampleIter << ": Not found\n";
			}
		} else {
			//std::cout << "Find pose - Sample: " << sampleIter << ": Not found\n";
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Get volume metrics.
	//----------------------------------------------------------------------------------------------------
	Job->initialEstimations = true;
	calibBuildCalibVolumeMetrics(Job);

	double rmse = calibGetProjectionRMSE(Job);

	//----------------------------------------------------------------------------------------------------
	// Create BA file.
	//----------------------------------------------------------------------------------------------------
	FILE* f;
	fopen_s(&f, "../cache/ba_input.txt", "w");

	if (f == 0) {
		std::cout << "Could not open bundle adjust input file for writing\n";
		return;
	}

	// Header.
	fprintf(f, "%d %d\n", (int)initialCube.points.size(), (int)viewPositions.size());

	// Axis directions.
	fprintf(f, "%f %f %f\n", Job->axisX.direction.x, Job->axisX.direction.y, Job->axisX.direction.z);
	fprintf(f, "%f %f %f\n", Job->axisY.direction.x, Job->axisY.direction.y, Job->axisY.direction.z);
	fprintf(f, "%f %f %f\n", Job->axisZ.direction.x, Job->axisZ.direction.y, Job->axisZ.direction.z);
	
	fprintf(f, "%f %f %f\n", Job->axisA.origin.x, Job->axisA.origin.y, Job->axisA.origin.z);
	fprintf(f, "%f %f %f\n", Job->axisA.direction.x, Job->axisA.direction.y, Job->axisA.direction.z);

	fprintf(f, "%f %f %f\n", Job->axisC.origin.x, Job->axisC.origin.y, Job->axisC.origin.z);
	fprintf(f, "%f %f %f\n", Job->axisC.direction.x, Job->axisC.direction.y, Job->axisC.direction.z);
	
	// Starting intrinsics.
	for (int j = 0; j < 9; ++j) {
		fprintf(f, "%f ", Job->camMat.at<double>(j));
	}
	fprintf(f, "\n");

	for (int j = 0; j < 8; ++j) {
		fprintf(f, "%f ", Job->camDist.at<double>(j));
	}
	fprintf(f, "\n");

	// Starting camera transform.
	auto invMat = glm::inverse(Job->camVolumeMat);
	cv::Mat camExt = convertTransformToRT(convertGlmTransMatToOpenCvMat(invMat));
	// 6 params. r, t
	for (int i = 0; i < 6; ++i) {
		fprintf(f, "%f ", camExt.at<double>(i));
	}
	fprintf(f, "\n");

	// 3D points.
	for (size_t pointIter = 0; pointIter < initialCube.points.size(); ++pointIter) {
		vec3 point = initialCube.points[pointIter];
		fprintf(f, "%f %f %f\n", point.x, point.y, point.z);
	}

	// Views.
	for (size_t viewIter = 0; viewIter < viewPositions.size(); ++viewIter) {
		ldiHorsePositionAbs pos = viewPositions[viewIter];
		fprintf(f, "%d %f %f %f %f %f\n", (int)viewObservations[viewIter].size(), pos.x, pos.y, pos.z, pos.a, pos.c);
		
		std::vector<cv::Point2f>* viewPoints = &viewObservations[viewIter];
		for (size_t pointIter = 0; pointIter < viewObservations[viewIter].size(); ++pointIter) {
			cv::Point2f point = viewObservations[viewIter][pointIter];
			int pointId = viewPointIds[viewIter][pointIter];

			fprintf(f, "%d %f %f\n", pointId, point.x, point.y);
		}
	}

	fclose(f);
}

void calibLoadNewBA(ldiCalibrationJob* Job, const std::string& FilePath) {
	FILE* f;

	fopen_s(&f, FilePath.c_str(), "r");

	if (f == 0) {
		std::cout << "Could not open bundle adjust file.\n";
		return;
	}

	int basePoseCount;
	int cubePointCount;

	fscanf_s(f, "%d\n", &cubePointCount);
	
	// Cam intrinsics
	double camInts[4];
	double camDist[4];
	fscanf_s(f, "%lf %lf %lf %lf\n", &camInts[0], &camInts[1], &camInts[2], &camInts[3]);
	fscanf_s(f, "%lf %lf %lf %lf\n", &camDist[0], &camDist[1], &camDist[2], &camDist[3]);

	cv::Mat cam = cv::Mat::eye(3, 3, CV_64F);
	cam.at<double>(0, 0) = camInts[0];
	cam.at<double>(0, 1) = 0.0;
	cam.at<double>(0, 2) = camInts[2];
	cam.at<double>(1, 0) = 0.0;
	cam.at<double>(1, 1) = camInts[1];
	cam.at<double>(1, 2) = camInts[3];
	cam.at<double>(2, 0) = 0.0;
	cam.at<double>(2, 1) = 0.0;
	cam.at<double>(2, 2) = 1.0;

	cv::Mat dist = cv::Mat::zeros(8, 1, CV_64F);
	dist.at<double>(0) = camDist[0];
	dist.at<double>(1) = camDist[1];
	dist.at<double>(2) = camDist[2];
	dist.at<double>(3) = camDist[3];

	Job->camMat = cam;
	Job->camDist = dist;

	double pose[6];
	fscanf_s(f, "%lf %lf %lf %lf %lf %lf\n", &pose[0], &pose[1], &pose[2], &pose[3], &pose[4], &pose[5]);

	cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64F);
	rVec.at<double>(0) = pose[0];
	rVec.at<double>(1) = pose[1];
	rVec.at<double>(2) = pose[2];

	cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64F);
	tVec.at<double>(0) = pose[3];
	tVec.at<double>(1) = pose[4];
	tVec.at<double>(2) = pose[5];

	cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
	cv::Rodrigues(rVec, cvRotMat);

	mat4 worldMat = glm::identity<mat4>();
	worldMat[0][0] = cvRotMat.at<double>(0, 0);
	worldMat[0][1] = cvRotMat.at<double>(1, 0);
	worldMat[0][2] = cvRotMat.at<double>(2, 0);
	worldMat[1][0] = cvRotMat.at<double>(0, 1);
	worldMat[1][1] = cvRotMat.at<double>(1, 1);
	worldMat[1][2] = cvRotMat.at<double>(2, 1);
	worldMat[2][0] = cvRotMat.at<double>(0, 2);
	worldMat[2][1] = cvRotMat.at<double>(1, 2);
	worldMat[2][2] = cvRotMat.at<double>(2, 2);
	worldMat[3][0] = tVec.at<double>(0);
	worldMat[3][1] = tVec.at<double>(1);
	worldMat[3][2] = tVec.at<double>(2);

	worldMat = glm::inverse(worldMat);
	Job->camVolumeMat = worldMat;

	// Axis directions.
	Job->axisX.origin = vec3Zero;
	Job->axisY.origin = vec3Zero;
	Job->axisZ.origin = vec3Zero;
	fscanf_s(f, "%f %f %f\n", &Job->axisX.direction.x, &Job->axisX.direction.y, &Job->axisX.direction.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisY.direction.x, &Job->axisY.direction.y, &Job->axisY.direction.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisZ.direction.x, &Job->axisZ.direction.y, &Job->axisZ.direction.z);

	fscanf_s(f, "%f %f %f\n", &Job->axisA.origin.x, &Job->axisA.origin.y, &Job->axisA.origin.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisA.direction.x, &Job->axisA.direction.y, &Job->axisA.direction.z);

	fscanf_s(f, "%f %f %f\n", &Job->axisC.origin.x, &Job->axisC.origin.y, &Job->axisC.origin.z);
	fscanf_s(f, "%f %f %f\n", &Job->axisC.direction.x, &Job->axisC.direction.y, &Job->axisC.direction.z);
	
	std::vector<vec3> cubePoints;

	for (int i = 0; i < cubePointCount; ++i) {
		int pointId;
		vec3 pos;
		fscanf_s(f, "%d %f %f %f\n", &pointId, &pos.x, &pos.y, &pos.z);
		cubePoints.push_back(pos);
	}

	calibCubeInit(&Job->cube);
	Job->cube.points = cubePoints;
	calibCubeCalculateMetrics(&Job->cube, true);

	fclose(f);

	calibGetProjectionRMSE(Job);
	calibRealignWorldVolume(Job);
	calibGetProjectionRMSE(Job);
}

void calibOptimizeVolume(ldiCalibrationJob* Job) {
	std::cout << "Starting stereo calibration: " << getTime() << "\n";

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char args[2048];
	sprintf_s(args, "python bundleAdjust2.py ../../bin/cache/ba_input.txt ../../bin/cache/ba_refined.txt");

	CreateProcessA(
		NULL,
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		"../../assets/bin",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);

	calibLoadNewBA(Job, "../cache/ba_refined.txt");
}

void calibCompareVolumeCalibrations(const std::string& CalibPathA, const std::string& CalibPathB) {
	ldiCalibrationJob jobA;
	if (!calibLoadCalibJob(CalibPathA, &jobA)) {
		return;
	}

	ldiCalibrationJob jobB;
	if (!calibLoadCalibJob(CalibPathB, &jobB)) {
		return;
	}

	float aXY = glm::degrees(glm::angle(jobA.axisX.direction, jobA.axisY.direction));
	float aXZ = glm::degrees(glm::angle(jobA.axisX.direction, jobA.axisZ.direction));
	float aYZ = glm::degrees(glm::angle(jobA.axisY.direction, jobA.axisZ.direction));
	float aXA = glm::degrees(glm::angle(jobA.axisX.direction, jobA.axisA.direction));
	float aXC = glm::degrees(glm::angle(jobA.axisX.direction, jobA.axisC.direction));
	
	float bXY = glm::degrees(glm::angle(jobB.axisX.direction, jobB.axisY.direction));
	float bXZ = glm::degrees(glm::angle(jobB.axisX.direction, jobB.axisZ.direction));
	float bYZ = glm::degrees(glm::angle(jobB.axisY.direction, jobB.axisZ.direction));
	float bXA = glm::degrees(glm::angle(jobB.axisX.direction, jobB.axisA.direction));
	float bXC = glm::degrees(glm::angle(jobB.axisX.direction, jobB.axisC.direction));

	float errorXY = aXY - bXY;
	float errorXZ = aXZ - bXZ;
	float errorYZ = aYZ - bYZ;
	float errorXA = aXA - bXA;
	float errorXC = aXC - bXC;
	
	std::cout << "Set A:\n";
	std::cout << "Angle XY: " << aXY << "\n";
	std::cout << "Angle XZ: " << aXZ << "\n";
	std::cout << "Angle YZ: " << aYZ << "\n";
	std::cout << "Angle XA: " << aXA << "\n";
	std::cout << "Angle XC: " << aXC << "\n";
	std::cout << "Cam mat: " << jobA.camMat << "\n";
	std::cout << "Cam dist: " << jobA.camDist << "\n";
	
	std::cout << "Set B:\n";
	std::cout << "Angle XY: " << bXY << "\n";
	std::cout << "Angle XZ: " << bXZ << "\n";
	std::cout << "Angle YZ: " << bYZ << "\n";
	std::cout << "Angle XA: " << bXA << "\n";
	std::cout << "Angle XC: " << bXC << "\n";
	std::cout << "Cam mat: " << jobB.camMat << "\n";
	std::cout << "Cam dist: " << jobB.camDist << "\n";

	std::cout << "Error:\n";
	std::cout << "Angle XY: " << errorXY << "\n";
	std::cout << "Angle XZ: " << errorXZ << "\n";
	std::cout << "Angle YZ: " << errorYZ << "\n";
	std::cout << "Angle XA: " << errorXA << "\n";
	std::cout << "Angle XC: " << errorXC << "\n";
	std::cout << "Cam mat: " << (jobA.camMat - jobB.camMat) << "\n";
	std::cout << "Cam dist: " << (jobA.camDist - jobB.camDist) << "\n";
}