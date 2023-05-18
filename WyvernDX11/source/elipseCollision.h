#pragma once

struct ldiElipse {
	vec3 origin;
	vec3 forward;
	vec3 right;
	vec3 up;
	float scaleForward;
	float scaleRight;
};

struct ldiElipseCollisionTester {
	vec3 planeOrigin;
	vec3 planeRot;

	float laserPlaneAngleDot;
	float laserPlaneAngleDeg;
	float laserPlaneAngleAspect;
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

void elipseCollisionSetup(ldiApp* AppContext, ldiElipseCollisionTester* Tool) {
	Tool->planeOrigin = vec3(2, 1, 3);
	//Tool->planeRot = vec3(-45.0f, 0.0f, 0.0f);
	Tool->planeRot = vec3(-116.566, 90.0f, 0.0f);
}

void elipseCollisionShowUi(ldiApp* AppContext, ldiElipseCollisionTester* Tool) {
	ImGui::Text("%f %f %f", Tool->laserPlaneAngleDot, Tool->laserPlaneAngleDeg, Tool->laserPlaneAngleAspect);
	ImGui::DragFloat3("Plane origin", (float*)&Tool->planeOrigin, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("Plane rot", (float*)&Tool->planeRot, 0.1f, -360.0f, 360.0f);
}

void elipseCollisionRender(ldiApp* AppContext, ldiElipseCollisionTester* Tool) {
	ldiElipse e1 = {};
	elipseInit(&e1, vec3(1, 0, 1), vec3(1, 0, 1), 0.25f, 0.5f);

	ldiElipse e2 = {};
	elipseInit(&e2, vec3(2, 0, 1), vec3(0, 0, 1), 0.25f, 0.5f);

	elipseRender(AppContext, &e1);
	elipseRender(AppContext, &e2);

	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, 0), 0.25f, vec3(0, 1, 0), 32);
	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, 0.25f), 0.22f, vec3(0, 1, 0), 32);
	pushDebugSphere(&AppContext->defaultDebug, e2.origin + vec3(0, 0, -0.25f), 0.22f, vec3(0, 1, 0), 32);

	mat4 planeRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->planeRot.x), vec3(1, 0, 0));
	mat4 planeRotMatY = glm::rotate(mat4(1.0f), glm::radians(Tool->planeRot.y), vec3(0, 1, 0));
	mat4 planeRotMatZ = glm::rotate(mat4(1.0f), glm::radians(Tool->planeRot.z), vec3(0, 0, 1));
	planeRotMat = planeRotMatY * planeRotMat;
	planeRotMat = planeRotMat * planeRotMatZ;
	vec3 planeNormal = planeRotMat * vec4(0.0f, 0, 1.0f, 0);
	planeNormal = glm::normalize(planeNormal);

	pushDebugPlane(&AppContext->defaultDebug, Tool->planeOrigin, planeNormal, 4.0f, vec3(0, 1, 1));

	vec3 laserOrigin(2.5f, 4, 3);
	vec3 laserDir = vec3(0.5f, -1.0f, 0);
	laserDir = glm::normalize(laserDir);
	vec3 laserRight = glm::normalize(glm::cross(vec3Up, laserDir));
	vec3 laserUp = glm::normalize(glm::cross(laserRight, laserDir));

	pushDebugLine(&AppContext->defaultDebug, laserOrigin, laserOrigin + laserRight, vec3(1, 0, 0));
	pushDebugLine(&AppContext->defaultDebug, laserOrigin, laserOrigin + laserUp, vec3(0, 1, 0));

	vec3 laserTarget = laserOrigin + laserDir * 6.0f;
	float laserBeamWidth = 0.1f;
	int laserSegs = 24;

	Tool->laserPlaneAngleDot = glm::dot(-laserDir, planeNormal);
	Tool->laserPlaneAngleDeg = glm::degrees(acosf(Tool->laserPlaneAngleDot));
	Tool->laserPlaneAngleAspect = 1.0f / Tool->laserPlaneAngleDot;
	// 45 degs: 1.4141
	// 63 degs: 2.20269
	// 70 degs: 2.92

	// Cast ray for central laser beam.
	{
		vec3 rayOrigin = laserOrigin;
		//vec3 rayDir = laserTarget - rayOrigin;
		vec3 rayDir = laserDir;
		rayDir = glm::normalize(rayDir);

		ldiRaycastResult hitResult = physicsRaycastPlane(rayOrigin, rayDir, Tool->planeOrigin, planeNormal, 20.0f);

		if (hitResult.hit) {
			// TODO: If laserDir and planeNormal are co-incident, then use diff calc.
			vec3 laserHitUp = glm::normalize(glm::cross(laserDir, planeNormal));
			vec3 laserHitRight = glm::normalize(glm::cross(laserHitUp, planeNormal));
			/*pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + laserHitRight, vec3(1, 0, 0));
			pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + laserHitUp, vec3(0, 1, 0));*/
			pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + laserHitRight * laserBeamWidth * Tool->laserPlaneAngleAspect, vec3(1, 0, 0));
			pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + laserHitUp * laserBeamWidth, vec3(0, 1, 0));

			pushDebugElipse(&AppContext->defaultDebug, hitResult.pos, laserBeamWidth, laserBeamWidth * Tool->laserPlaneAngleAspect, vec3(0, 1, 1), 32, laserHitUp, laserHitRight);

			pushDebugLine(&AppContext->defaultDebug, rayOrigin, hitResult.pos, vec3(0, 1.0f, 0));
			//pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, 0.01f, vec3(1, 1, 0), 3);
			pushDebugBox(&AppContext->defaultDebug, hitResult.pos, vec3(0.004f, 0.004f, 0.004f), vec3(1, 1, 0));
			pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + hitResult.normal, vec3(1, 0, 1));

			// Dot collision.
			pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, laserBeamWidth, vec3(0, 1, 0), 32);

			float lobeLerp = (Tool->laserPlaneAngleAspect - 1.0f) / 2.0f;
			float lobeMin = laserBeamWidth * 0.5f;
			float lobeMax = laserBeamWidth * 1.25f;
			float lobeScale = (lobeMax - lobeMin) * lobeLerp + lobeMin;

			pushDebugSphere(&AppContext->defaultDebug, hitResult.pos - laserHitRight * laserBeamWidth * Tool->laserPlaneAngleAspect * 0.5f, lobeScale, vec3(0, 1, 0.5f), 32);
			pushDebugSphere(&AppContext->defaultDebug, hitResult.pos + laserHitRight * laserBeamWidth * Tool->laserPlaneAngleAspect * 0.5f, lobeScale, vec3(0, 1, 0.5f), 32);

			//pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, laserBeamWidth * Tool->laserPlaneAngleAspect, vec3(0, 1, 0.5f), 32);
		} else {
			pushDebugLine(&AppContext->defaultDebug, rayOrigin, rayOrigin + rayDir * 20.0f, vec3(1, 0, 0));
		}
	}

	// Cast all rays for laser beam.
	{
		float divInc = glm::radians(360.0f / laserSegs);
		
		for (int i = 0; i < laserSegs; ++i) {
			float rad = divInc * i;
			vec3 rayOrigin = laserRight * (float)sin(rad) * laserBeamWidth + laserUp * (float)cos(rad) * laserBeamWidth + laserOrigin;
			//vec3 rayDir = laserTarget - rayOrigin;
			vec3 rayDir = laserDir;
			rayDir = glm::normalize(rayDir);
		
			ldiRaycastResult hitResult = physicsRaycastPlane(rayOrigin, rayDir, Tool->planeOrigin, planeNormal, 20.0f);

			if (hitResult.hit) {
				pushDebugLine(&AppContext->defaultDebug, rayOrigin, hitResult.pos, vec3(0, 0.35f, 0));
				pushDebugBox(&AppContext->defaultDebug, hitResult.pos, vec3(0.004f, 0.004f, 0.004f), vec3(1, 1, 0));
				//pushDebugSphere(&AppContext->defaultDebug, hitResult.pos, 0.01f, vec3(1, 1, 0), 3);
				//pushDebugLine(&AppContext->defaultDebug, hitResult.pos, hitResult.pos + hitResult.normal, vec3(1, 0, 1));
			} else {
				pushDebugLine(&AppContext->defaultDebug, rayOrigin, rayOrigin + rayDir * 20.0f, vec3(0.35f, 0, 0));
			}
		}
	}
}
