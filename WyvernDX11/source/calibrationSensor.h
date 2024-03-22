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
	ldiCalibSensor startingSensor;
	ldiCalibSensor simSensor;
	std::vector<ldiCalibSensorCalibSample> samples;
};

void calibSensorUpdateTransform(ldiCalibSensor* Sensor) {
	Sensor->mat = glm::translate(Sensor->position) * glm::toMat4(quat(glm::radians(Sensor->rotation)));
	//normal Sensor-> = (vec3)(sensorWorld * vec4(0, 0, -1, 0.0f));
	Sensor->axisX = (vec3)(Sensor->mat * vec4(1, 0, 0, 0.0f));
	Sensor->axisY = (vec3)(Sensor->mat * vec4(0, 1, 0, 0.0f));
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
	calibSensorInit(&Calib->startingSensor);
	//Calib->startingSensor.position = vec3(1, 1, 1);
	//Calib->startingSensor.rotation = vec3(45, 90, 0);
	Calib->startingSensor.position = vec3(0, 0, 0);
	Calib->startingSensor.rotation = vec3(90, 115, 0);
	calibSensorUpdateTransform(&Calib->startingSensor);

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

void calibSensorRunSimulatedGather(ldiCalibSensorCalibration* Calib, ldiCalibrationJob* MachineCalib, ldiCalibSensor* SimSensor) {
	std::cout << "Running calibration sensor gather simulation\n";

	calibSensorCalibrationInit(Calib, SimSensor);

	ldiHorsePosition horsePos = {};
	horsePos.x = 0;
	horsePos.y = 0;
	horsePos.z = 0;
	horsePos.c = 13000;
	horsePos.a = 0;

	/*
	{
		horsePos.c = 13000;
		int steps = 3;
		int start = -1400;
		int stop = 1400;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			std::cout << "X: " << horsePos.x;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	{
		horsePos.c = -60000;
		int steps = 2;
		int start = -2000;
		int stop = 2200;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			std::cout << "X: " << horsePos.x;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}
	*/
	
	/*{
		horsePos.c = 13000;
		int steps = 16;
		int start = -1400;
		int stop = 1400;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	{
		horsePos.c = -60000;
		int steps = 16;
		int start = -2000;
		int stop = 2200;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}

	{
		horsePos.c = -115000;
		int steps = 16;
		int start = -2000;
		int stop = 2200;
		for (int i = 0; i <= steps; ++i) {
			int step = (stop - start) / steps;
			horsePos.x = start + i * step;
			calibSensorCaptureSample(Calib, MachineCalib, horsePos);
		}
	}*/

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

	std::cout << "Completed calibration sensor gather simulation\n";
}