#pragma once

struct ldiElipse {
	vec3 origin;
	vec3 forward;
	vec3 right;
	vec3 up;
	float scaleForward;
	float scaleRight;
};

void elipseRender(ldiApp* AppContext, ldiElipse* Elipse) {
	// Axis gizmo.
	pushDebugLine(&AppContext->defaultDebug, Elipse->origin, Elipse->origin + Elipse->right, vec3(1, 0, 0));
	pushDebugLine(&AppContext->defaultDebug, Elipse->origin, Elipse->origin + Elipse->up, vec3(0, 1, 0));
	pushDebugLine(&AppContext->defaultDebug, Elipse->origin, Elipse->origin + Elipse->forward, vec3(0, 0, 1));

	pushDebugElipse(&AppContext->defaultDebug, Elipse->origin, Elipse->scaleRight, Elipse->scaleForward, vec3(1, 0, 0), 32, Elipse->right, Elipse->forward);
}

void elipseInit(ldiElipse* Elipse, vec3 Origin, vec3 Forward, float Width, float Height) {
	Forward = glm::normalize(Forward);

	Elipse->origin = Origin;
	Elipse->forward = Forward;

	// NOTE: Assume 2D plane for now, so right is always a swizzle.
	Elipse->right = vec3(Forward.z, Forward.y, -Forward.x);
	Elipse->up = glm::cross(Elipse->forward, Elipse->right);

	Elipse->scaleForward = Height;
	Elipse->scaleRight = Width;
}

void elipseCollisionSetup(ldiApp* AppContext) {
	// ...
}

void elipseCollisionRender(ldiApp* AppContext) {
	ldiElipse e1 = {};
	elipseInit(&e1, vec3(1, 0, 1), vec3(1, 0, 1), 0.25f, 0.5f);

	ldiElipse e2 = {};
	elipseInit(&e2, vec3(2, 0, 1), vec3(0, 0, 1), 0.25f, 0.5f);

	elipseRender(AppContext, &e1);
	elipseRender(AppContext, &e2);

	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, 0), 0.25f, vec3(0, 1, 0), 32);
	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, 0.25f), 0.22f, vec3(0, 1, 0), 32);
	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, -0.25f), 0.22f, vec3(0, 1, 0), 32);

	vec3 planeOrigin(2, 1, 3);
	vec3 planeNormal(0, 1, 1);
	planeNormal = glm::normalize(planeNormal);

	pushDebugPlane(&AppContext->defaultDebug, planeOrigin, planeNormal, 2.0f, vec3(0, 1, 1));

	vec3 laserOrigin(2.5f, 4, 3);
	vec3 laserTarget = laserOrigin + vec3(0, -6.0f, 0);
	float laserBeamWidth = 0.2f;
	int laserSegs = 24;

	float angleDot = glm::dot(vec3(0, 1, 0), planeNormal);
	float angle = glm::degrees(acosf(angleDot));
	float aspect = 1.0f / angleDot;
	std::cout << "Angle: " << angleDot << " " << angle << " " << aspect << "\n";
	

	// 45 degs: 1.4141
	// 63 degs: 2.20269
	// 70 degs: 2.92

	// Cast ray for central laser beam.
	{
		vec3 rayOrigin = laserOrigin;
		vec3 rayDir = laserTarget - rayOrigin;
		rayDir = glm::normalize(rayDir);

		ldiRaycastResult hitResult = physicsRaycastPlane(rayOrigin, rayDir, planeOrigin, planeNormal, 20.0f);

		if (hitResult.hit) {
			pushDebugLine(&AppContext->defaultDebug, rayOrigin, hitResult.pos, vec3(0, 0.5f, 0));
			pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, 0.01f, vec3(1, 1, 0), 3);
			pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + hitResult.normal, vec3(1, 0, 1));
		} else {
			pushDebugLine(&AppContext->defaultDebug, rayOrigin, rayOrigin + rayDir * 20.0f, vec3(1, 0, 0));
		}
	}

	// Cast all rays for laser beam.
	{
		float divInc = glm::radians(360.0f / laserSegs);
		
		for (int i = 0; i < laserSegs; ++i) {
			float rad = divInc * i;
			vec3 rayOrigin = vec3Right * (float)sin(rad) * laserBeamWidth + vec3Forward * (float)cos(rad) * laserBeamWidth + laserOrigin;
			vec3 rayDir = laserTarget - rayOrigin;
			rayDir = glm::normalize(rayDir);
		
			ldiRaycastResult hitResult = physicsRaycastPlane(rayOrigin, rayDir, planeOrigin, planeNormal, 20.0f);

			if (hitResult.hit) {
				pushDebugLine(&AppContext->defaultDebug, rayOrigin, hitResult.pos, vec3(0, 1, 0));
				pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, 0.01f, vec3(1, 1, 0), 3);
				pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + hitResult.normal, vec3(1, 0, 1));
			} else {
				pushDebugLine(&AppContext->defaultDebug, rayOrigin, rayOrigin + rayDir * 20.0f, vec3(1, 0, 0));
			}
		}
	}
}
