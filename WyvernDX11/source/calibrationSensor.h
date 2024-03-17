#pragma once

struct ldiCalibSensor {
	double	widthPixels;
	double	heightPixels;
	double	pixelSizeCm;

	vec3	calibSensorOrigin;
	vec3	calibSensorRot;
	mat4	calibSensorMat;

	vec3	intersectionLineP0;
	vec3	intersectionLineP1;

	bool	hit;
	vec2	hitP0;
	vec2	hitP1;
};

void calibSensorInit(ldiCalibSensor* Sensor) {
	Sensor->widthPixels = 1280;
	Sensor->heightPixels = 800;
	Sensor->pixelSizeCm = 0.0003;

	Sensor->calibSensorOrigin = vec3(0, 0, 0);
	Sensor->calibSensorRot = vec3(90, 115, 0);
}

vec3 calibSensorFromWorldToPixel(ldiCalibSensor* Sensor, mat4 WorldInv, vec3 P) {
	vec3 result = WorldInv * vec4(P, 1.0f);
	result.x = (result.x) / Sensor->pixelSizeCm + Sensor->widthPixels * 0.5;
	result.y = (result.y) / Sensor->pixelSizeCm + Sensor->heightPixels * 0.5;
	result.z = 0;

	return result;
}

vec3 calibSensorFromPixelToWorld(ldiCalibSensor* Sensor, mat4 World, vec3 P) {
	vec3 result;
	result.x = (P.x - (Sensor->widthPixels * 0.5)) * Sensor->pixelSizeCm;
	result.y = (P.y - (Sensor->heightPixels * 0.5)) * Sensor->pixelSizeCm;
	result.z = 0;
	
	return World * vec4(result, 1.0);
}