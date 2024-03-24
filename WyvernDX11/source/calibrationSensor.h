#pragma once

struct ldiCalibSensor {
	double widthPixels;
	double heightPixels;
	double pixelSizeCm;
	float worldWidth;
	float worldHeight;

	vec3 position;
	vec3 rotation;
	mat4 mat;
	vec3 axisX;
	vec3 axisY;
	
	vec2 intersectionLineP0;
	vec2 intersectionLineP1;

	bool hit;
	vec2 hitP0;
	vec2 hitP1;
	vec3 hitWorldP0;
	vec3 hitWorldP1;

	vec3 worldScanIntP0;
	vec3 worldScanIntP1;
};

struct ldiCalibSensorCalibSample {
	ldiHorsePosition horsePosition;

	ldiPlane scanPlaneWorkSpace;

	vec2 scanLineP0;
	vec2 scanLineP1;
};

struct ldiCalibSensorCalibration {
	ldiCalibSensor simSensor;
	ldiCalibSensor calibratedSensor;
	std::vector<ldiCalibSensorCalibSample> samples;
};

void calibSensorUpdateTransform(ldiCalibSensor* Sensor) {
	Sensor->mat = glm::translate(Sensor->position) * glm::toMat4(quat(glm::radians(Sensor->rotation)));
	Sensor->axisX = (vec3)(Sensor->mat * vec4(1, 0, 0, 0.0f));
	Sensor->axisY = (vec3)(Sensor->mat * vec4(0, 1, 0, 0.0f));
}

void calibSensorSetTransform(ldiCalibSensor* Sensor, mat4& Mat) {
	Sensor->mat = Mat;

	Sensor->position = vec3(Mat[3]);
	quat q = glm::toQuat(Mat);
	Sensor->rotation = glm::degrees(glm::eulerAngles(q));

	std::cout << "calibSensorSetTransform\n";
	std::cout << GetStr(Sensor->rotation) << "\n";
	std::cout << GetStr(&Sensor->mat) << "\n";

	calibSensorUpdateTransform(Sensor);
	std::cout << GetStr(&Sensor->mat) << "\n";
}

void calibSensorInit(ldiCalibSensor* Sensor) {
	Sensor->widthPixels = 1280;
	Sensor->heightPixels = 800;
	Sensor->pixelSizeCm = 0.0003;
	Sensor->worldWidth = Sensor->widthPixels * Sensor->pixelSizeCm;
	Sensor->worldHeight = Sensor->heightPixels * Sensor->pixelSizeCm;

	Sensor->position = vec3(0, 0, 0);
	Sensor->rotation = vec3(90, 115, 0);
	calibSensorUpdateTransform(Sensor);
}

vec2 calibSensorFromWorldToPixel(ldiCalibSensor* Sensor, mat4 WorldInv, vec3 P) {
	vec2 result = WorldInv * vec4(P, 1.0f);
	result.x = (result.x) / Sensor->pixelSizeCm + Sensor->widthPixels * 0.5;
	result.y = (result.y) / Sensor->pixelSizeCm + Sensor->heightPixels * 0.5;

	return result;
}

vec3 calibSensorFromPixelToWorld(ldiCalibSensor* Sensor, mat4 World, vec2 P) {
	vec3 result;
	result.x = (P.x - (Sensor->widthPixels * 0.5)) * Sensor->pixelSizeCm;
	result.y = (P.y - (Sensor->heightPixels * 0.5)) * Sensor->pixelSizeCm;
	result.z = 0;
	
	return World * vec4(result, 1.0);
}

void calibSensorCalibrationInit(ldiCalibSensorCalibration* Calib, ldiCalibSensor* SimSensor) {
	calibSensorInit(&Calib->calibratedSensor);
	//Calib->startingSensor.position = vec3(1, 1, 1);
	//Calib->startingSensor.rotation = vec3(45, 90, 0);
	Calib->calibratedSensor.position = vec3(0, 0.5, 0);
	Calib->calibratedSensor.rotation = vec3(90, 115, 0);
	calibSensorUpdateTransform(&Calib->calibratedSensor);

	Calib->simSensor = *SimSensor;
	Calib->samples.clear();
}

void calibSensorUpdate(ldiCalibSensor* Sensor, ldiCalibrationJob* MachineCalib, ldiHorsePosition HorsePos) {
	calibSensorUpdateTransform(Sensor);
	
	if (!MachineCalib->metricsCalculated || !MachineCalib->scannerCalibrated) {
		return;
	}

	// Find live intersection of scan plane and sensor.
	mat4 workTrans = horseGetWorkTransform(MachineCalib, HorsePos);

	mat4 sensorWorld = workTrans * Sensor->mat;
	mat4 sensorWorldInv = glm::inverse(sensorWorld);

	vec3 sensorOrigin = sensorWorld * vec4(0, 0, 0, 1.0f);
	vec3 sensorNormal = (vec3)(sensorWorld * vec4(0, 0, -1, 0.0f));
	//vec3 sensorAxisX = (vec3)(sensorWorld * vec4(1, 0, 0, 0.0f));
	//vec3 sensorAxisY = (vec3)(sensorWorld * vec4(0, 1, 0, 0.0f));
	ldiPlane sensorPlane = { sensorOrigin, sensorNormal };

	ldiPlane scanPlane = horseGetScanPlane(MachineCalib, HorsePos);

	vec3 rO;
	vec3 rD;
	getRayAtPlaneIntersection(sensorPlane, scanPlane, rO, rD);

	Sensor->worldScanIntP0 = rO - rD * 4.0f;
	Sensor->worldScanIntP1 = rO + rD * 4.0f;
	Sensor->intersectionLineP0 = calibSensorFromWorldToPixel(Sensor, sensorWorldInv, Sensor->worldScanIntP0);
	Sensor->intersectionLineP1 = calibSensorFromWorldToPixel(Sensor, sensorWorldInv, Sensor->worldScanIntP1);
	
	// Convert plane instersection back to sensor pixel space.
	vec2 p0Pixel = calibSensorFromWorldToPixel(Sensor, sensorWorldInv, rO);
	vec2 p1Pixel = calibSensorFromWorldToPixel(Sensor, sensorWorldInv, rO + rD);
	vec2 lineDir = glm::normalize(p1Pixel - p0Pixel);

	// Get the clip points of the line vs sensor in pixel space.
	vec2 entryP;
	vec2 exitP;
	Sensor->hit = getLineRectIntersection(p0Pixel, lineDir, vec2(0, 0), vec2(Sensor->widthPixels, Sensor->heightPixels), entryP, exitP);

	if (Sensor->hit) {
		Sensor->hitP0 = entryP;
		Sensor->hitP1 = exitP;
		Sensor->hitWorldP0 = calibSensorFromPixelToWorld(Sensor, sensorWorld, vec3(entryP, 0));
		Sensor->hitWorldP1 = calibSensorFromPixelToWorld(Sensor, sensorWorld, vec3(exitP, 0));
	}
}

void calibSensorCaptureSample(ldiCalibSensorCalibration* Calib, ldiCalibrationJob* MachineCalib, ldiHorsePosition HorsePos) {
	calibSensorUpdate(&Calib->simSensor, MachineCalib, HorsePos);

	if (Calib->simSensor.hit) {
		mat4 workTrans = horseGetWorkTransform(MachineCalib, HorsePos);
		mat4 invWorkTrans = glm::inverse(workTrans);

		ldiPlane scanPlane = horseGetScanPlane(MachineCalib, HorsePos);
		scanPlane.point = invWorkTrans * vec4(scanPlane.point, 1.0f);
		scanPlane.normal = invWorkTrans * vec4(scanPlane.normal, 0.0f);

		ldiCalibSensorCalibSample sample = {};
		sample.horsePosition = HorsePos;
		sample.scanPlaneWorkSpace = scanPlane;
		sample.scanLineP0 = Calib->simSensor.hitP0;
		sample.scanLineP1 = Calib->simSensor.hitP1;

		Calib->samples.push_back(sample);
	}
}

struct ldiCalibSensorScanCalibContext {
	vec2 center;
	float centerCounts;
	vec2 normal;
	float countProportion;
};

ldiCalibSensorScanCalibContext calibSensorGetScanCalibContext(std::vector<ldiCalibSensorCalibSample>& CalibSamples) {
	ldiCalibSensorScanCalibContext result = {};

	float avgCount = 0.0f;
	for (size_t sampIter = 0; sampIter < CalibSamples.size(); ++sampIter) {
		ldiCalibSensorCalibSample* s = &CalibSamples[sampIter];

		vec2 dir = s->scanLineP0 - s->scanLineP1;
		vec2 normal = glm::normalize(vec2(-dir.y, dir.x));
		result.normal += normal;

		vec2 center = (s->scanLineP0 + s->scanLineP1) * 0.5f;
		result.center += center;

		result.centerCounts += s->horsePosition.x;

		avgCount += 1.0f;

		std::cout << sampIter << ": P0: " << GetStr(s->scanLineP0) << " P1: " << GetStr(s->scanLineP1) << "\n";
		std::cout << sampIter << ": Normal: " << GetStr(normal) << "\n";
	}

	result.normal = glm::normalize(result.normal / avgCount);
	result.center /= avgCount;
	result.centerCounts /= avgCount;
	std::cout << "Avg normal: " << GetStr(result.normal) << " Avg center: " << GetStr(result.center) << " Avg counts: " << result.centerCounts << "\n";

	float avgPropCount = 0.0;
	for (size_t sampIter = 1; sampIter < CalibSamples.size(); ++sampIter) {
		ldiCalibSensorCalibSample* s0 = &CalibSamples[sampIter - 1];
		ldiCalibSensorCalibSample* s1 = &CalibSamples[sampIter - 0];

		vec2 center0 = (s0->scanLineP0 + s0->scanLineP1) * 0.5f;
		float d0 = glm::dot(-center0, result.normal);

		vec2 center1 = (s1->scanLineP0 + s1->scanLineP1) * 0.5f;
		float d1 = glm::dot(-center1, result.normal);

		result.countProportion += (s1->horsePosition.x - s0->horsePosition.x) / (d1 - d0);
		avgPropCount += 1.0;
	}

	result.countProportion /= avgPropCount;
	std::cout << "Average proportion: " << result.countProportion << "\n";

	return result;
}

int calibSensorGetCountsFromPixelPos(ldiCalibSensorScanCalibContext Context, vec2 PixelPos) {
	vec2 delta = PixelPos - Context.center;
	float dist = glm::dot(delta, Context.normal);
	float result = -dist * Context.countProportion + Context.centerCounts;

	return (int)(roundf(result));
}

struct ldiCalibSensorPoint {
	vec2 pixelPos;
	vec3 workPos;
	std::vector<ldiPlane> scanPlanes;
};

void calibSensorRunSimulatedGather(ldiCalibSensorCalibration* Calib, ldiCalibrationJob* MachineCalib, ldiCalibSensor* SimSensor) {
	std::cout << "Running calibration sensor gather simulation\n";

	std::vector<ldiCalibSensorPoint> points;

	ldiCalibSensorPoint point = {};
	point.pixelPos = vec2(1280.0 * 0.5, 800 * 0.5);
	points.push_back(point);

	point.pixelPos = vec2(1280.0, 800 * 0.5);
	points.push_back(point);

	point.pixelPos = vec2(1280.0 * 0.5, 800);
	points.push_back(point);

	calibSensorCalibrationInit(Calib, SimSensor);

	ldiHorsePosition horsePos = {};
	horsePos.x = 0;
	horsePos.y = 0;
	horsePos.z = 0;
	horsePos.c = 13000;
	horsePos.a = 0;

	//----------------------------------------------------------------------------------------------------
	// Sweep A.
	//----------------------------------------------------------------------------------------------------
	size_t samplesStart = Calib->samples.size();
	{
		horsePos.c = 13000;
		int steps = 16;
		int start = -4000;
		int stop = 1000;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	std::vector<ldiCalibSensorCalibSample> samples = { Calib->samples.begin() + samplesStart, Calib->samples.end() };
	ldiCalibSensorScanCalibContext context = calibSensorGetScanCalibContext(samples);

	for (size_t i = 0; i < points.size(); ++i) {
		int counts = calibSensorGetCountsFromPixelPos(context, points[i].pixelPos);
		//std::cout << "Point " << GetStr(points[i].pixelPos) << ": " << counts << "\n";

		ldiHorsePosition localHorse = horsePos;
		localHorse.x = counts;
		
		mat4 workTrans = horseGetWorkTransform(MachineCalib, localHorse);
		mat4 invWorkTrans = glm::inverse(workTrans);

		ldiPlane scanPlane = horseGetScanPlane(MachineCalib, localHorse);
		scanPlane.point = invWorkTrans * vec4(scanPlane.point, 1.0f);
		scanPlane.normal = invWorkTrans * vec4(scanPlane.normal, 0.0f);
		
		points[i].scanPlanes.push_back(scanPlane);
	}

	//----------------------------------------------------------------------------------------------------
	// Sweep B.
	//----------------------------------------------------------------------------------------------------
	samplesStart = Calib->samples.size();
	{
		horsePos.c = -60000;
		int steps = 16;
		int start = -4000;
		int stop = 1000;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	samples = { Calib->samples.begin() + samplesStart, Calib->samples.end() };
	context = calibSensorGetScanCalibContext(samples);

	for (size_t i = 0; i < points.size(); ++i) {
		int counts = calibSensorGetCountsFromPixelPos(context, points[i].pixelPos);
		std::cout << "Point " << GetStr(points[i].pixelPos) << ": " << counts << "\n";

		ldiHorsePosition localHorse = horsePos;
		localHorse.x = counts;

		mat4 workTrans = horseGetWorkTransform(MachineCalib, localHorse);
		mat4 invWorkTrans = glm::inverse(workTrans);

		ldiPlane scanPlane = horseGetScanPlane(MachineCalib, localHorse);
		scanPlane.point = invWorkTrans * vec4(scanPlane.point, 1.0f);
		scanPlane.normal = invWorkTrans * vec4(scanPlane.normal, 0.0f);

		points[i].scanPlanes.push_back(scanPlane);
	}

	//----------------------------------------------------------------------------------------------------
	// Sweep C.
	//----------------------------------------------------------------------------------------------------
	samplesStart = Calib->samples.size();
	{
		horsePos.c = -115000;
		int steps = 16;
		int start = -4000;
		int stop = 1000;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	samples = { Calib->samples.begin() + samplesStart, Calib->samples.end() };
	context = calibSensorGetScanCalibContext(samples);

	for (size_t i = 0; i < points.size(); ++i) {
		int counts = calibSensorGetCountsFromPixelPos(context, points[i].pixelPos);
		std::cout << "Point " << GetStr(points[i].pixelPos) << ": " << counts << "\n";

		ldiHorsePosition localHorse = horsePos;
		localHorse.x = counts;

		mat4 workTrans = horseGetWorkTransform(MachineCalib, localHorse);
		mat4 invWorkTrans = glm::inverse(workTrans);

		ldiPlane scanPlane = horseGetScanPlane(MachineCalib, localHorse);
		scanPlane.point = invWorkTrans * vec4(scanPlane.point, 1.0f);
		scanPlane.normal = invWorkTrans * vec4(scanPlane.normal, 0.0f);

		points[i].scanPlanes.push_back(scanPlane);
	}

	//----------------------------------------------------------------------------------------------------
	// Calculate new sensor plane.
	//----------------------------------------------------------------------------------------------------
	for (size_t i = 0; i < points.size(); ++i) {
		ldiCalibSensorPoint& p = points[i];

		//std::cout << "Plane " << i << ": " << points[i].scanPlanes.size() << "\n";

		// TODO: Gather multiple sets of 3 planes and take average position.
		vec3 intersectPoint;
		if (getPointAtIntersectionOfPlanes(p.scanPlanes[0], p.scanPlanes[1], p.scanPlanes[2], &intersectPoint)) {
			p.workPos = intersectPoint;
		}
	}

	vec3 p0 = points[0].workPos;
	vec3 p1 = points[1].workPos;
	vec3 p2 = points[2].workPos;

	vec3 p0p1 = p1 - p0;
	vec3 p0p2 = p2 - p0;

	vec3 axisZ = glm::normalize(glm::cross(p0p1, p0p2));
	vec3 axisY = glm::normalize(glm::cross(axisZ, p0p1));
	vec3 axisX = glm::normalize(glm::cross(axisY, axisZ));

	mat4 newSensorMat = mat4(1.0);
	newSensorMat[0] = vec4(axisX, 0.0f);
	newSensorMat[1] = vec4(axisY, 0.0f);
	newSensorMat[2] = vec4(axisZ, 0.0f);
	newSensorMat[3] = vec4(p0, 1.0f);

	calibSensorSetTransform(&Calib->calibratedSensor, newSensorMat);

	//----------------------------------------------------------------------------------------------------
	// Calculate error.
	//----------------------------------------------------------------------------------------------------
	float se = 0.0f;
	int seCount = 0;

	for (size_t sampIter = 0; sampIter < Calib->samples.size(); ++sampIter) {
		ldiCalibSensorCalibSample* s = &Calib->samples[sampIter];

		calibSensorUpdate(&Calib->calibratedSensor, MachineCalib, s->horsePosition);

		if (Calib->calibratedSensor.hit) {
			{
				vec2 p0 = Calib->calibratedSensor.hitP0;
				//std::cout << "Compare: " << GetStr(s->scanLineP0) << " " << GetStr(p0) << "\n";
				float err2 = glm::distance2(p0, s->scanLineP0);
				se += err2;
				++seCount;
			}

			{			
				vec2 p1 = Calib->calibratedSensor.hitP1;
				//std::cout << "Compare: " << GetStr(s->scanLineP1) << " " << GetStr(p1) << "\n";
				float err2 = glm::distance2(p1, s->scanLineP1);
				se += err2;
				++seCount;
			}
		} else {
			//std::cout << "No hit\n";
		}
	}

	float mse = se / (float)seCount;
	float rmse = sqrt(mse);

	std::cout << "RMSE: " << rmse << "\n";
	std::cout << "Completed calibration sensor gather simulation\n";
}