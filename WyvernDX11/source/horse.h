#pragma once

struct ldiTextInfo {
	vec2 position;
	std::string text;
	vec4 color;
};

struct ldiTransform {
	vec3 localPos;
	vec3 localRot;
	mat4 local;
	mat4 world;
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

void renderTransformOrigin(ldiApp* AppContext, ldiCamera* Camera, ldiTransform* Transform, std::string Text, std::vector<ldiTextInfo>* TextBuffer) {
	vec3 root = transformGetWorldPoint(Transform, vec3(0, 0, 0));

	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(1, 0, 0)), vec3(1, 0, 0));
	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(0, 1, 0)), vec3(0, 1, 0));
	pushDebugLine(&AppContext->defaultDebug, root, transformGetWorldPoint(Transform, vec3(0, 0, 1)), vec3(0, 0, 1));

	displayTextAtPoint(Camera, root, Text, vec4(1.0f, 1.0f, 1.0f, 0.6f), TextBuffer);
}