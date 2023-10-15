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

void updateCamera3dBasicFps(ldiCamera* Camera, float ViewWidth, float ViewHeight) {
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);

	Camera->viewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);
	Camera->projMat = glm::perspectiveFovRH_ZO(glm::radians(Camera->fov), ViewWidth, ViewHeight, 0.01f, 100.0f);
	Camera->invProjMat = inverse(Camera->projMat);
	Camera->projViewMat = Camera->projMat * Camera->viewMat;
	Camera->viewWidth = ViewWidth;
	Camera->viewHeight = ViewHeight;
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