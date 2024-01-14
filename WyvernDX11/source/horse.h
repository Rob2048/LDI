#pragma once

//----------------------------------------------------------------------------------------------------
// Transform.
//----------------------------------------------------------------------------------------------------
struct ldiTransform {
	vec3 localPos;
	vec3 localRot;
	mat4 local;
	mat4 world;
};

void transformUpdateLocal(ldiTransform* Transform) {
	Transform->local = glm::rotate(mat4(1.0f), glm::radians(Transform->localRot.x), vec3(1, 0, 0));
	Transform->local = glm::rotate(Transform->local, glm::radians(Transform->localRot.y), vec3(0, 1, 0));
	Transform->local = glm::rotate(Transform->local, glm::radians(Transform->localRot.z), vec3(0, 0, 1));
	// TODO: Check if can put local instead of ident mat.
	Transform->local = glm::translate(mat4(1.0f), Transform->localPos) * Transform->local;
}

void transformUpdateWorld(ldiTransform* Transform, ldiTransform* Parent) {
	Transform->world = Transform->local;

	if (Parent) {
		Transform->world = Parent->world * Transform->world;
	}
}

void transformInit(ldiTransform* Transform, vec3 Pos, vec3 Rot) {
	Transform->localPos = Pos;
	Transform->localRot = Rot;
	transformUpdateLocal(Transform);
}

vec3 transformGetWorldPoint(ldiTransform* Transform, vec3 LocalPos) {
	return Transform->world * vec4(LocalPos.x, LocalPos.y, LocalPos.z, 1.0f);
}

void renderTransformOrigin(ldiApp* AppContext, ldiCamera* Camera, ldiTransform* Transform, std::string Text, std::vector<ldiTextInfo>* TextBuffer) {
	vec3 root = transformGetWorldPoint(Transform, vec3(0, 0, 0));

	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(1, 0, 0)), vec3(1, 0, 0));
	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(0, 1, 0)), vec3(0, 1, 0));
	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(0, 0, 1)), vec3(0, 0, 1));

	displayTextAtPoint(Camera, root, Text, vec4(1.0f, 1.0f, 1.0f, 0.6f), TextBuffer);
}

void renderOrigin(ldiApp* AppContext, ldiCamera* Camera, mat4 WorldMatrix, std::string Text, std::vector<ldiTextInfo>* TextBuffer) {
	vec3 root = WorldMatrix * vec4(0, 0, 0, 1);

	vec3 p0 = WorldMatrix * vec4(1, 0, 0, 1);
	vec3 p1 = WorldMatrix * vec4(0, 1, 0, 1);
	vec3 p2 = WorldMatrix * vec4(0, 0, 1, 1);

	pushDebugLine(&AppContext->defaultDebug, root, p0, vec3(1, 0, 0));
	pushDebugLine(&AppContext->defaultDebug, root, p1, vec3(0, 1, 0));
	pushDebugLine(&AppContext->defaultDebug, root, p2, vec3(0, 0, 1));

	displayTextAtPoint(Camera, root, Text, vec4(1.0f, 1.0f, 1.0f, 0.6f), TextBuffer);
}

//----------------------------------------------------------------------------------------------------
// Horse.
//----------------------------------------------------------------------------------------------------
// Machine position in steps.
struct ldiHorsePosition {
	int x;
	int y;
	int z;
	int c;
	int a;
};

struct ldiHorsePositionAbs {
	double x;
	double y;
	double z;
	double c;
	double a;
};

struct ldiHorse {
	ldiTransform origin;
	ldiTransform axisX;
	ldiTransform axisY;
	ldiTransform axisZ;
	ldiTransform axisA;
	ldiTransform axisC;
	float x;
	float y;
	float z;
	float a;
	float b;
};

void horseUpdateMats(ldiHorse* Horse) {
	transformUpdateWorld(&Horse->origin, 0);
	transformUpdateWorld(&Horse->axisX, &Horse->origin);
	transformUpdateWorld(&Horse->axisY, &Horse->axisX);
	transformUpdateWorld(&Horse->axisZ, &Horse->axisY);
	transformUpdateWorld(&Horse->axisC, &Horse->axisZ);
	transformUpdateWorld(&Horse->axisA, &Horse->origin);
}

void horseInit(ldiHorse* Horse) {
	// X: -10 to 10
	// Y: -10 to 10
	// Z: -10 to 10

	transformInit(&Horse->origin, vec3(-30.0f, 0, 0), vec3(-90, 0, 0));
	transformInit(&Horse->axisX, vec3(19.5f, 0, 0), vec3(0, 0, 0));
	transformInit(&Horse->axisY, vec3(0, 0, 6.8f), vec3(0, 0, 0));
	transformInit(&Horse->axisZ, vec3(0, 0, 17.4f), vec3(0, 0, 0));
	transformInit(&Horse->axisC, vec3(13.7f, 0, 0.0f), vec3(0, 0, 0));
	transformInit(&Horse->axisA, vec3(32.0f, 0, 32), vec3(0, 0, 0));

	horseUpdateMats(Horse);
}

void horseUpdate(ldiHorse* Horse) {
	Horse->axisX.localPos.x = Horse->x + 19.5f;
	Horse->axisY.localPos.y = Horse->y;
	Horse->axisZ.localPos.z = Horse->z + 17.4f;
	Horse->axisA.localRot.x = Horse->a;
	Horse->axisC.localRot.z = Horse->b;

	transformUpdateLocal(&Horse->axisX);
	transformUpdateLocal(&Horse->axisY);
	transformUpdateLocal(&Horse->axisZ);
	transformUpdateLocal(&Horse->axisA);
	transformUpdateLocal(&Horse->axisC);

	horseUpdateMats(Horse);
}

void horseGetRefinedCubeAtPosition(ldiCalibrationJob* Job, ldiHorsePosition Position, std::vector<vec3>& Points, std::vector<ldiCalibCubeSide>& Sides, std::vector<vec3>& Corners) {
	vec3 offset = glm::f64vec3(Position.x, Position.y, Position.z) * Job->stepsToCm;
	vec3 mechTrans = offset.x * Job->axisX.direction + offset.y * Job->axisY.direction + offset.z * -Job->axisZ.direction;

	vec3 refToAxisC = Job->axisC.origin;
	float axisCAngleDeg = (Position.c - 13000) * 0.001875;
	mat4 axisCRot = glm::rotate(mat4(1.0f), glm::radians(-axisCAngleDeg), Job->axisC.direction);

	mat4 axisCMat = axisCRot * glm::translate(mat4(1.0), -refToAxisC);
	axisCMat = glm::translate(mat4(1.0), refToAxisC) * axisCMat;

	// Transform points.
	Points.resize(Job->cube.points.size(), vec3(0.0f, 0.0f, 0.0f));
	for (size_t i = 0; i < Job->cube.points.size(); ++i) {
		if (i >= 18 && i <= 26) {
			continue;
		}

		Points[i] = axisCMat * vec4(Job->cube.points[i], 1.0f);
		Points[i] += mechTrans;
	}

	// Transform sides.
	Sides.resize(Job->cube.sides.size(), {});
	for (size_t i = 0; i < Job->cube.sides.size(); ++i) {
		Sides[i].id = Job->cube.sides[i].id;

		ldiPlane plane = Job->cube.sides[i].plane;

		plane.point = axisCMat * vec4(plane.point, 1.0f);
		plane.point += mechTrans;
		
		plane.normal = glm::normalize(axisCMat * vec4(plane.normal, 0.0f));

		Sides[i].plane = plane;

		for (int c = 0; c < 4; ++c) {
			vec3 transCorner = axisCMat * vec4(Job->cube.sides[i].corners[c], 1.0f);
			transCorner += mechTrans;

			Sides[i].corners[c] = transCorner;
		}
	}

	// Transform corners.
	Corners.resize(8, vec3(0.0f, 0.0f, 0.0f));
	for (int i = 0; i < 8; ++i) {
		Corners[i] = axisCMat * vec4(Job->cube.corners[i], 1.0f);
		Corners[i] += mechTrans;
	}
}

void horseGetProjectionCubePoints(ldiCalibrationJob* Job, ldiHorsePosition Position, std::vector<vec3>& Points) {
	vec3 offset = glm::f64vec3(Position.x, Position.y, Position.z) * Job->stepsToCm;
	vec3 mechTrans = offset.x * Job->axisX.direction + offset.y * Job->axisY.direction + offset.z * -Job->axisZ.direction;

	vec3 refToAxisC = Job->axisC.origin;
	float axisCAngleDeg = (Position.c - 13000) * 0.001875;
	mat4 axisCRot = glm::rotate(mat4(1.0f), glm::radians(-axisCAngleDeg), Job->axisC.direction);
	mat4 axisCMat = axisCRot * glm::translate(mat4(1.0), -refToAxisC);
	axisCMat = glm::translate(mat4(1.0), refToAxisC) * axisCMat;

	vec3 refToAxisA = Job->axisA.origin;
	float axisAAngleDeg = (Position.a) * (360.0 / (32.0 * 200.0 * 90.0));
	mat4 axisARot = glm::rotate(mat4(1.0f), glm::radians(axisAAngleDeg), Job->axisA.direction);
	mat4 axisAMat = axisARot * glm::translate(mat4(1.0), -refToAxisA);
	axisAMat = glm::translate(mat4(1.0), refToAxisA) * axisAMat;

	// Transform points.
	Points.resize(Job->cube.points.size(), vec3(0.0f, 0.0f, 0.0f));
	for (size_t i = 0; i < Job->cube.points.size(); ++i) {
		if (i >= 18 && i <= 26) {
			continue;
		}

		Points[i] = axisCMat * vec4(Job->cube.points[i], 1.0f);
		Points[i] = axisAMat * vec4(Points[i], 1.0f);
		Points[i] += mechTrans;
	}
}

mat4 horseGetWorkTransform(ldiCalibrationJob* Job, ldiHorsePosition Position) {
	vec3 offset = glm::f64vec3(Position.x, Position.y, Position.z) * Job->stepsToCm;
	vec3 mechTrans = offset.x * Job->axisX.direction + offset.y * Job->axisY.direction + offset.z * -Job->axisZ.direction;

	float axisCAngleDeg = Position.c * 0.001875;
	mat4 axisCRot = glm::rotate(mat4(1.0f), glm::radians(-axisCAngleDeg), Job->axisC.direction);
	mat4 axisCMat = glm::translate(mat4(1.0), Job->axisC.origin + mechTrans) * axisCRot;

	return axisCMat;
}

ldiCamera horseGetCamera(ldiCalibrationJob* Job, ldiHorsePosition Position, int Width, int Height, bool UseViewport, vec2 ViewPortTopLeft, vec2 ViewPortSize) {
	mat4 camWorldMat = Job->camVolumeMat;

	{
		vec3 refToAxis = Job->axisA.origin - vec3(0.0f, 0.0f, 0.0f);
		float axisAngleDeg = (Position.a) * (360.0 / (32.0 * 200.0 * 90.0));
		mat4 axisRot = glm::rotate(mat4(1.0f), glm::radians(-axisAngleDeg), Job->axisA.direction);

		camWorldMat[3] = vec4(vec3(camWorldMat[3]) - refToAxis, 1.0f);
		camWorldMat = axisRot * camWorldMat;
		camWorldMat[3] = vec4(vec3(camWorldMat[3]) + refToAxis, 1.0f);
	}

	mat4 viewMat = cameraConvertOpenCvWorldToViewMat(camWorldMat);
	mat4 projMat = mat4(1.0);

	projMat = cameraCreateProjectionFromOpenCvCamera(Width, Height, Job->camMat, 0.01f, 100.0f);
	
	if (UseViewport) {
		projMat = cameraProjectionToVirtualViewport(projMat, ViewPortTopLeft, ViewPortSize, vec2(Width, Height));
	}

	ldiCamera camera = {};
	camera.viewMat = viewMat;
	camera.projMat = projMat;
	camera.invProjMat = inverse(projMat);
	camera.projViewMat = projMat * viewMat;
	camera.viewWidth = Width;
	camera.viewHeight = Height;

	return camera;
}

ldiCamera horseGetCamera(ldiCalibrationJob* Job, ldiHorsePosition Position, int Width, int Height) {
	return horseGetCamera(Job, Position, Width, Height, false, vec2(0.0f, 0.0f), vec2(Width, Height));
}

ldiPlane horseGetScanPlane(ldiCalibrationJob* Job, ldiHorsePosition Position) {
	ldiPlane result = {};

	vec3 refToAxis = Job->axisA.origin;
	float axisAngleDeg = (Position.a) * (360.0 / (32.0 * 200.0 * 90.0));
	mat4 axisRot = glm::rotate(mat4(1.0f), glm::radians(-axisAngleDeg), Job->axisA.direction);

	vec3 newPoint = vec4(Job->scanPlane.point - refToAxis, 1.0f);
	newPoint = axisRot * vec4(newPoint, 1.0f);
	newPoint = vec4(newPoint + refToAxis, 1.0f);

	vec3 newNormal = axisRot * vec4(Job->scanPlane.normal, 0.0f);

	result.point = newPoint;
	result.normal = glm::normalize(newNormal);
	return result;
}