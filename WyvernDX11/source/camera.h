#pragma once

struct ldiCamera {
	vec3 position;
	vec3 rotation;
	mat4 viewMat;
	mat4 projMat;
	mat4 invProjMat;
	mat4 projViewMat;
	float fov;
	int viewWidth;
	int viewHeight;
};

struct ldiTextInfo {
	vec2 position;
	std::string text;
	vec4 color;
};

vec3 extractEulerAnglesXY(const glm::mat4& matrix) {
	float sy = sqrt(matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0]);
	bool singular = sy < 1e-6;
	float x, y;

	if (!singular) {
		x = atan2(matrix[1][2], matrix[2][2]);
		y = atan2(-matrix[0][2], sy);
	} else {
		x = atan2(-matrix[2][1], matrix[1][1]);
		y = atan2(-matrix[0][2], sy);
	}

	return glm::vec3(glm::degrees(x), glm::degrees(y), 0.0f);
}

void updateCamera3dBasicFps(ldiCamera* Camera, float ViewWidth, float ViewHeight) {
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);

	Camera->viewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);
	Camera->projMat = glm::perspectiveFovRH_ZO(glm::radians(Camera->fov), ViewWidth, ViewHeight, 0.01f, 200.0f);
	Camera->invProjMat = inverse(Camera->projMat);
	Camera->projViewMat = Camera->projMat * Camera->viewMat;
	Camera->viewWidth = ViewWidth;
	Camera->viewHeight = ViewHeight;
}

mat4 cameraCreateProjectionFromOpenCvCamera(float Width, float Height, cv::Mat& CameraMatrix, float Near, float Far) {
	mat4 result = glm::identity<mat4>();

	float Fx = CameraMatrix.at<double>(0, 0);
	float Fy = CameraMatrix.at<double>(1, 1);
	float Cx = CameraMatrix.at<double>(0, 2);
	float Cy = CameraMatrix.at<double>(1, 2);

	result[0][0] = 2 * (Fx / Width);
	result[0][1] = 0;
	result[0][2] = 0;
	result[0][3] = 0;

	result[1][0] = 0;
	result[1][1] = -2 * (Fy / Height);
	result[1][2] = 0;
	result[1][3] = 0;

	result[2][0] = (Width - 2.0f * Cx) / Width;
	result[2][1] = (Height - 2.0f * Cy) / -Height;
	result[2][2] = Far / (Near - Far);
	result[2][3] = -1;

	result[3][0] = 0;
	result[3][1] = 0;
	result[3][2] = -(Far * Near) / (Far - Near);
	result[3][3] = 0;

	return result;
}

mat4 cameraProjectionToVirtualViewport(mat4 ProjMat, vec2 ViewPortTopLeft, vec2 ViewPortSize, vec2 WindowSize) {
	float scaleX = ViewPortSize.x / WindowSize.x;
	float scaleY = ViewPortSize.y / WindowSize.y;

	// Scale existing FOV.
	ProjMat[0][0] *= scaleX;
	ProjMat[1][1] *= scaleY;

	// Scale existing shift.
	ProjMat[2][0] *= scaleX;
	ProjMat[2][1] *= scaleY;

	// Add new shift so that middle of virtual viewport is Top left: 1, -1, Bottom right: -1, 1
	ProjMat[2][0] -= ((ViewPortTopLeft.x + ViewPortSize.x * 0.5f) / ViewPortSize.x) * 2 * scaleX - 1;
	ProjMat[2][1] += ((ViewPortTopLeft.y + ViewPortSize.y * 0.5f) / ViewPortSize.y) * 2 * scaleY - 1;

	return ProjMat;
}

mat4 cameraConvertOpenCvWorldToViewMat(mat4 World) {
	// NOTE: Converts the world transform of a camera (From OpenCV) to a view matrix.
	// NOTE: OpenCV cams look down +Z so we negate.
	World[2] = -World[2];
	
	return glm::inverse(World);
}

vec3 projectPoint(ldiCamera* Camera, vec3 Pos) {
	vec4 worldPos = vec4(Pos.x, Pos.y, Pos.z, 1);
	vec4 clipPos = Camera->projViewMat * worldPos;
	clipPos.x /= clipPos.w;
	clipPos.y /= clipPos.w;

	vec3 screenPos;
	screenPos.x = (clipPos.x * 0.5 + 0.5) * Camera->viewWidth;
	screenPos.y = (1.0f - (clipPos.y * 0.5 + 0.5)) * Camera->viewHeight;
	screenPos.z = clipPos.z;

	return screenPos;
}

void displayTextAtPoint(ldiCamera* Camera, vec3 Position, std::string Text, vec4 Color, std::vector<ldiTextInfo>* TextBuffer) {
	vec3 projPos = projectPoint(Camera, Position);
	if (projPos.z > 0) {
		TextBuffer->push_back({ vec2(projPos.x, projPos.y), Text, Color });
	}
}

ldiLine screenToRay(ldiCamera* Camera, vec2 Pos) {
	//NOTE: Precision of this is really bad unless using doubles.
	ldiLine result = {};

	vec3d clipPos = vec3d(vec2d(Pos), 1.0);
	clipPos.x = ((clipPos.x / (double)Camera->viewWidth) - 0.5) * 2.0;
	clipPos.y = ((1.0 - (clipPos.y / (double)Camera->viewHeight)) - 0.5) * 2.0;

	mat4d invProjView = glm::inverse((mat4d)Camera->projViewMat);

	vec4d homoPos = invProjView * vec4d(clipPos, 1.0);
	vec3d worldPos = vec3d(homoPos) / homoPos.w;

	vec3d camWorldPos = glm::inverse((mat4d)Camera->viewMat)[3];

	result.origin = camWorldPos;
	result.direction = glm::normalize(worldPos - camWorldPos);

	return result;
}