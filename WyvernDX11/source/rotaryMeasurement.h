#pragma once

struct ldiRotaryPoint {
	int id;
	vec2 pos;
	float info;
	float zeroAngle;
	float relAngle;
};

struct ldiRotaryResults {
	std::thread					workerThread;

	// Only worker thread.
	cv::VideoCapture			webcamCapture;

	// Shared by threads.
	std::mutex					frameMutex;
	volatile bool				frameAvailable = false;
	uint8_t* greyFrame;
	std::vector<cv::KeyPoint>	blobsInitial;

	// Only main thread.
	std::vector<vec2>			blobs;

	bool						showGizmos = false;

	bool						foundCenter = false;
	vec2						centerPos;

	bool						foundLocator = false;
	vec2						locatorPos;

	std::vector<ldiRotaryPoint>	points;

	ldiRotaryPoint				accumPoints[32];

	float						relativeAngleToZero;

	float						lastResults[25];
	float						stdDev;
	float						stdDevStoppedLimit = 0.002;

	float						processTime = 0.0f;
};

float filterAngles(float Src, float Dest) {
	// NOTE: Turns out the angles don't really need filtering :O.

	// NOTE: Filter flops hard when doing 0-360 flip.
	//a->info = a->info * 0.90f + angle * 0.1f;

	return Dest;
}

float subAngles(float A, float B) {
	// NOTE: Equivalent of A - B
	B -= A;

	if (B > 180) {
		B -= 360;
	} else if (B < -180) {
		B += 360;
	}

	return B;
}

float avgAngles(int Count, float* Data) {
	float avgStart = Data[0];
	float accum = 0.0f;

	for (int i = 0; i < Count; ++i) {
		float v = Data[i];

		v -= avgStart;

		if (v > 180) {
			v -= 360;
		} else if (v < -180) {
			v += 360;
		}

		accum += v / (float)Count;
	}

	accum += avgStart;

	if (accum >= 360.0) {
		accum -= 360.0;
	} else if (accum < 0.0) {
		accum += 360.0;
	}

	return accum;
}

bool compareInterval(ldiRotaryPoint i1, ldiRotaryPoint i2) {
	return (i1.info < i2.info);
}

void rotaryMeasurementWorkerThread(ldiRotaryResults* Rotary) {
	std::cout << "Running rotary worker thread\n";

	//----------------------------------------------------------------------------------------------------
	// Start webcamera camera.
	//----------------------------------------------------------------------------------------------------
	std::cout << "Start camera...\n";
	Rotary->webcamCapture.open(0, cv::CAP_ANY);

	if (!Rotary->webcamCapture.isOpened()) {
		std::cout << "Failed to open camera\n";
	} else {
		std::cout << "Camera started\n";
	}

	cv::Mat webcamFrame;
	cv::Mat frameGrey;

	cv::SimpleBlobDetector::Params params = cv::SimpleBlobDetector::Params();
	//std::cout << params.minArea << " " << params.maxArea << "\n";
	params.minArea = 70;
	params.maxArea = 700;

	std::vector<cv::KeyPoint> blobs;

	while (true) {
		if (Rotary->webcamCapture.read(webcamFrame)) {
			cv::cvtColor(webcamFrame, frameGrey, cv::COLOR_BGR2GRAY);

			cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
			detector->detect(frameGrey, blobs);

			std::unique_lock<std::mutex> lock(Rotary->frameMutex);
			Rotary->frameAvailable = true;
			memcpy(Rotary->greyFrame, frameGrey.data, 640 * 480);

			Rotary->blobsInitial.clear();
			for (size_t i = 0; i < blobs.size(); ++i) {
				Rotary->blobsInitial.push_back(blobs[i]);
			}

			lock.unlock();
		}
	}

	std::cout << "Rotary workder thread completed\n";
}

void rotaryMeasurementInit(ldiRotaryResults* RotaryResults) {
	RotaryResults->greyFrame = new uint8_t[640 * 480];

	// NOTE: Disabled for now.
	//RotaryResults->workerThread = std::thread(rotaryMeasurementWorkerThread, RotaryResults);
}

bool rotaryMeasurementProcess(ldiRotaryResults* RotaryResults) {
	std::unique_lock<std::mutex> lock(RotaryResults->frameMutex, std::defer_lock);
	if (!lock.try_lock()) {
		return false;
	}

	if (!RotaryResults->frameAvailable) {
		return false;
	}

	RotaryResults->blobs.clear();
	for (size_t i = 0; i < RotaryResults->blobsInitial.size(); ++i) {
		vec2 point(RotaryResults->blobsInitial[i].pt.x, RotaryResults->blobsInitial[i].pt.y);
		RotaryResults->blobs.push_back(point);
	}

	RotaryResults->frameAvailable = false;
	lock.unlock();

	RotaryResults->foundCenter = false;
	RotaryResults->foundLocator = false;
	RotaryResults->points.clear();

	float viewWidth = 640.0f;
	float viewHeight = 480.0f;
	float viewCenterX = viewWidth / 2.0f;
	float viewCenterY = viewHeight / 2.0f;

	float trackCenterRadius = 40.0f;
	float trackInnerRadius = 180.0f;
	float trackOuterRadius = 260.0f;

	std::vector<ldiRotaryPoint> tempResults;

	for (size_t i = 0; i < RotaryResults->blobs.size(); ++i) {
		vec2 k = RotaryResults->blobs[i];

		vec2 imgCenter(viewCenterX, viewCenterY);
		vec2 pointPos(k.x, k.y);
		float centerDist = glm::distance(imgCenter, pointPos);

		// Identify center.
		if (centerDist < trackCenterRadius) {
			RotaryResults->foundCenter = true;
			RotaryResults->centerPos = pointPos;
		} else if (centerDist < trackInnerRadius) {
			RotaryResults->foundLocator = true;
			RotaryResults->locatorPos = pointPos;
		} else if (centerDist < trackOuterRadius) {
			ldiRotaryPoint p;
			p.id = -1;
			p.pos = pointPos;
			tempResults.push_back(p);
		}
	}

	if (!RotaryResults->foundCenter || !RotaryResults->foundLocator || tempResults.size() != 32) {
		return true;
	}

	// Find the starting point.
	int startId = -1;
	float startAngleOffset = 0.0f;

	for (size_t i = 0; i < tempResults.size(); ++i) {
		ldiRotaryPoint* a = &tempResults[i];

		vec2 vA = RotaryResults->centerPos - a->pos;
		vec2 vB = RotaryResults->locatorPos - a->pos;

		float lenA = glm::length(vA);

		vec2 vAnorm = vA / lenA;
		vec2 vBnorm = vB / lenA;

		float t = glm::dot(vAnorm, vBnorm);
		a->info = t;

		if (t < 0.505) {
			a->id = 0;
			//RotaryResults->points.push_back(*a);

			startId = i;

			// Get angle:
			vec2 pV = a->pos - RotaryResults->centerPos;
			float r = glm::length(pV);

			float theta = atan2(pV.y, pV.x);
			float angle = theta * (180 / M_PI) + 180.0;

			startAngleOffset = angle;
		}
	}

	if (startId == -1) {
		return true;
	}

	// Get point ID's from angle.
	for (size_t i = 0; i < tempResults.size(); ++i) {
		ldiRotaryPoint* a = &tempResults[i];

		// Get angle:
		vec2 pV = a->pos - RotaryResults->centerPos;
		float r = glm::length(pV);

		float theta = atan2(pV.y, pV.x);
		float angle = theta * (180 / M_PI) + 180.0;

		a->info = angle - startAngleOffset;

		float segWidth = (360.0 / 32);
		float segAngle = a->info;

		if (segAngle < 0.0f) {
			segAngle += 360.0f;
		}

		int pointId = (segAngle + (segWidth * 0.5f)) / segWidth;
		a->id = pointId;

		ldiRotaryPoint* b = &RotaryResults->accumPoints[pointId];

		b->id = a->id;
		b->pos = a->pos;
	}

	float angles[16];

	for (int i = 0; i < 16; ++i) {
		ldiRotaryPoint* a = &RotaryResults->accumPoints[i];
		ldiRotaryPoint* b = &RotaryResults->accumPoints[i + 16];

		vec2 pV = a->pos - b->pos;
		float r = glm::length(pV);

		float theta = atan2(pV.y, pV.x);
		float angle = theta * (180 / M_PI) + 180.0;

		a->info = filterAngles(a->info, angle);
		a->relAngle = a->info - a->zeroAngle;

		if (a->relAngle < 0) {
			a->relAngle += 360.0;
		}

		angles[i] = a->relAngle;
	}

	RotaryResults->relativeAngleToZero = avgAngles(16, angles);

	// Standard deviation over X seconds (25 FPS).
	// Shift array becuase I'm too lazy to make it circular. Last element is always the most recent.
	const int resultCount = 5;
	for (int i = 0; i < resultCount - 1; ++i) {
		RotaryResults->lastResults[i] = RotaryResults->lastResults[i + 1];
	}
	RotaryResults->lastResults[resultCount - 1] = RotaryResults->relativeAngleToZero;

	// Calculate mean.
	float sdevMean = avgAngles(resultCount, RotaryResults->lastResults);

	// Calc diff to mean.
	float sdevErrSqr[resultCount];
	for (int i = 0; i < resultCount; ++i) {
		float err = subAngles(RotaryResults->lastResults[i], sdevMean);
		sdevErrSqr[i] = err * err;
	}
	float sdevRootErrMean = sqrt(avgAngles(resultCount, sdevErrSqr));

	RotaryResults->stdDev = sdevRootErrMean;

	return true;
}