#pragma once

#include "PxPhysicsAPI.h"

using namespace physx;

class ldiPhysicsErrorCallback : public PxErrorCallback {
public:
	ldiPhysicsErrorCallback() {}
	~ldiPhysicsErrorCallback() {}

	virtual void reportError(PxErrorCode::Enum e, const char* message, const char* file, int line) {
		const char* errorCode = NULL;

		switch (e) {
			case PxErrorCode::eNO_ERROR: errorCode = "no error"; break;
			case PxErrorCode::eINVALID_PARAMETER: errorCode = "invalid parameter"; break;
			case PxErrorCode::eINVALID_OPERATION: errorCode = "invalid operation"; break;
			case PxErrorCode::eOUT_OF_MEMORY: errorCode = "out of memory"; break;
			case PxErrorCode::eDEBUG_INFO: errorCode = "info"; break;
			case PxErrorCode::eDEBUG_WARNING: errorCode = "warning"; break;
			case PxErrorCode::ePERF_WARNING: errorCode = "performance warning"; break;
			case PxErrorCode::eABORT: errorCode = "abort"; break;
			case PxErrorCode::eINTERNAL_ERROR: errorCode = "internal error"; break;
			case PxErrorCode::eMASK_ALL: errorCode = "unknown error"; break;
		}

		PX_ASSERT(errorCode);
		if (errorCode) {
			char buffer[1024];
			sprintf_s(buffer, sizeof(buffer), "%s (%d) : %s : %s\n", file, line, errorCode, message);
			std::cout << buffer;

			// in debug builds halt execution for abort codes
			PX_ASSERT(e != PxErrorCode::eABORT);

			// in release builds we also want to halt execution 
			// and make sure that the error message is flushed  
			while (e == PxErrorCode::eABORT) {
				std::cout << buffer;
				//physx::shdfnd::Thread::sleep(1000);
			}
		}
	}
};

static ldiPhysicsErrorCallback gDefaultErrorCallback;
static PxDefaultAllocator gDefaultAllocatorCallback;

struct ldiPhysics {
	ldiApp*				appContext;
	PxFoundation*		foundation;
	PxPhysics*			physics;
	PxCooking*			cooking;
};

struct ldiPhysicsCone {
	PxConvexMeshGeometry coneGeo;
};

struct ldiPhysicsMesh {
	PxTriangleMeshGeometry cookedMesh;
};

inline vec3 physicsPv3ToGv3(PxVec3* Vec) {
	return vec3(Vec->x, Vec->y, Vec->z);
}

inline PxVec3 physicsGv3ToPv3(vec3* Vec) {
	return PxVec3(Vec->x, Vec->y, Vec->z);
}

bool physicsCreateCone(ldiPhysics* Physics, float Length, float Radius, int Sides, ldiPhysicsCone* Cone) {
	// Cone aligned with X axis, apex at origin, base at (Length, 0, 0).

	std::vector<PxVec3> verts;
	verts.reserve(Sides + 1);
	verts.push_back(PxVec3(0, 0, 0)); // apex

	for (int i = 0; i < Sides; ++i) {
		float angle = (float)i / (float)Sides * 2.0f * M_PI;
		float y = cos(angle) * Radius;
		float z = sin(angle) * Radius;
		verts.push_back(PxVec3(Length, y, z)); // base
	}

	// Create convex mesh from vertices
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = (uint32_t)verts.size();
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = verts.data();
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	convexDesc.vertexLimit = max(8, Sides + 1);

	PxConvexMeshCookingResult::Enum result;
	PxConvexMesh* convexMesh = Physics->cooking->createConvexMesh(convexDesc, Physics->physics->getPhysicsInsertionCallback(), &result);
	if (result != PxConvexMeshCookingResult::Enum::eSUCCESS) {
		std::cout << "Failed to create convex mesh\n";
		return false;
	}

	Cone->coneGeo = PxConvexMeshGeometry(convexMesh);

	std::cout << "Created cone convex mesh\n";
	return true;
}

//void physicsCreateConeModel(ldiPhysicsCone* Cone, ldiModel* Model) {
//	Model->verts.resize(Cone->coneMesh->getNbVertices());
//	const PxVec3* verts = Cone->coneMesh->getVertices();
//	const PxU8* inds = Cone->coneMesh->getIndexBuffer();
//
//	std::cout << Model->verts.size() << "\n";
//
//	for (size_t i = 0; i < Model->verts.size(); ++i) {
//		Model->verts[i].pos = vec3(verts[i].x, verts[i].y, verts[i].z);
//		std::cout << GetStr(Model->verts[i].pos) << "\n";
//	}
//
//	for (int i = 0; i < Cone->coneMesh->getNbPolygons(); ++i) {
//		PxHullPolygon poly;
//		Cone->coneMesh->getPolygonData(i, poly);
//
//		std::cout << "Poly " << i << ": " << poly.mIndexBase << " -> " <<  poly.mNbVerts << "\n";
//
//		for (int j = poly.mIndexBase; j < poly.mIndexBase + poly.mNbVerts; ++j) {
//			std::cout << "  " << j << " -> " << (int)inds[j] << "\n";
//		}
//	}
//}

int physicsInit(ldiApp* AppContext, ldiPhysics* Physics) {
	Physics->appContext = AppContext;

	Physics->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	if (!Physics->foundation) {
		std::cout << "Could not init PhysX\n";
		return 1;
	}

	Physics->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *(Physics->foundation), PxTolerancesScale(), false);
	if (!Physics->physics) {
		std::cout << "Could not create PhysX physics object\n";
		return 1;
	}

	PxTolerancesScale scaleParams;
	scaleParams.length = 10;
	scaleParams.speed = 98;

	PxCookingParams cookingParams(scaleParams);

	Physics->cooking = PxCreateCooking(PX_PHYSICS_VERSION, *(Physics->foundation), cookingParams);
	if (!Physics->cooking) {
		std::cout << "Could not create PhysX cooking object\n";
		return 1;
	}

	return 0;
}

int physicsCookMesh(ldiPhysics* Physics, ldiModel* Model, ldiPhysicsMesh* Result) {
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = (uint32_t)Model->verts.size();
	meshDesc.points.stride = sizeof(ldiMeshVertex);
	meshDesc.points.data = Model->verts.data();

	meshDesc.triangles.count = (uint32_t)(Model->indices.size() / 3);
	meshDesc.triangles.stride = 3 * sizeof(uint32_t);
	meshDesc.triangles.data = Model->indices.data();

	PxTriangleMeshCookingResult::Enum result;
	
	double t0 = getTime();
	PxTriangleMesh* cookedMesh = Physics->cooking->createTriangleMesh(meshDesc, Physics->physics->getPhysicsInsertionCallback(), &result);
	t0 = getTime() - t0;
	std::cout << "Mesh cooked " << result << " in " << t0 * 1000.0f << " ms\n";

	if (result != PxTriangleMeshCookingResult::Enum::eSUCCESS) {
		std::cout << "Failed to cook\n";
		return 1;
	}

	Result->cookedMesh.triangleMesh = cookedMesh;
	
	return 0;
}

void physicsDestroyCookedMesh(ldiPhysics* Physics, ldiPhysicsMesh* CookedMesh) {
	if (CookedMesh->cookedMesh.triangleMesh) {
		CookedMesh->cookedMesh.triangleMesh->release();
	}
}

struct ldiRaycastResult {
	bool hit;
	int faceIdx;
	vec3 pos;
	vec2 barry;
	vec3 normal;
	float dist;
};

ldiRaycastResult physicsRaycast(ldiPhysicsMesh* Mesh, vec3 RayOrigin, vec3 RayDir, float MaxDist = 0.02f) {
	ldiRaycastResult hit{};
	hit.hit = false;

	PxVec3 rayOrigin(RayOrigin.x, RayOrigin.y, RayOrigin.z);
	PxVec3 rayDir(RayDir.x, RayDir.y, RayDir.z);
	rayDir.normalize();
	PxTransform geomPose(0, 0, 0);

	PxRaycastHit rayHit;
	int hitCount = PxGeometryQuery::raycast(rayOrigin, rayDir, Mesh->cookedMesh, geomPose, MaxDist, PxHitFlag::ePOSITION | PxHitFlag::eUV | PxHitFlag::eFACE_INDEX | PxHitFlag::eMESH_BOTH_SIDES | PxHitFlag::eNORMAL, 1, &rayHit);

	if (hitCount > 0) {
		hit.hit = true;
		hit.faceIdx = Mesh->cookedMesh.triangleMesh->getTrianglesRemap()[rayHit.faceIndex];
		hit.pos = vec3(rayHit.position.x, rayHit.position.y, rayHit.position.z);
		hit.barry = vec2(rayHit.u, rayHit.v);
		hit.dist = rayHit.distance;
		hit.normal = vec3(rayHit.normal.x, rayHit.normal.y, rayHit.normal.z);
	}

	return hit;
}

bool physicsBeamOverlap(ldiPhysicsMesh* Mesh, vec3 BeamOrigin, vec3 BeamDir, float BeamRadius, float BeamLength) {
	float halfHeight = BeamLength * 0.5f;
	PxCapsuleGeometry beam = PxCapsuleGeometry(BeamRadius, halfHeight);
	
	PxVec3 up(1.0f, 0.0f, 0.0f);
	PxVec3 normalizedDir = physicsGv3ToPv3(&BeamDir);
	PxVec3 rotationAxis = up.cross(normalizedDir);
	rotationAxis.normalize();
	float angle = acos(up.dot(normalizedDir));

	PxQuat rotation(angle, rotationAxis);
	vec3 pos = BeamOrigin + BeamDir * (halfHeight + BeamRadius);
	PxTransform beamPose(physicsGv3ToPv3(&pos), rotation);

	PxTransform geomPose(0, 0, 0);

	return PxGeometryQuery::overlap(beam, beamPose, Mesh->cookedMesh, geomPose);
}

bool physicsConeOverlap(ldiPhysicsMesh* Mesh, ldiPhysicsCone* Cone, vec3 ConeOrigin, vec3 ConeDir) {
	PxVec3 up(1.0f, 0.0f, 0.0f);
	PxVec3 normalizedDir = physicsGv3ToPv3(&ConeDir);
	PxVec3 rotationAxis = up.cross(normalizedDir);
	rotationAxis.normalize();
	float angle = acos(up.dot(normalizedDir));

	PxQuat rotation(angle, rotationAxis);
	vec3 pos = ConeOrigin;
	PxTransform beamPose(physicsGv3ToPv3(&pos), rotation);

	PxTransform geomPose(0, 0, 0);

	return PxGeometryQuery::overlap(Cone->coneGeo, beamPose, Mesh->cookedMesh, geomPose);
}

// NOTE: Creates a plane representation each time, maybe cache for faster?
ldiRaycastResult physicsRaycastPlane(vec3 RayOrigin, vec3 RayDir, vec3 PlanePoint, vec3 PlaneNormal, float MaxDist) {
	ldiRaycastResult hit{};
	hit.hit = false;

	PxRaycastHit rayHit;
	PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
	PxTransform pose = PxTransformFromPlaneEquation(PxPlane(physicsGv3ToPv3(&PlanePoint), physicsGv3ToPv3(&PlaneNormal)));
	PxU32 hitCount = PxGeometryQuery::raycast(physicsGv3ToPv3(&RayOrigin), physicsGv3ToPv3(&RayDir), PxPlaneGeometry(), pose, MaxDist, hitFlags, 1, &rayHit);

	if (hitCount > 0) {
		hit.hit = true;
		hit.pos = physicsPv3ToGv3(&rayHit.position);
		hit.normal = physicsPv3ToGv3(&rayHit.normal);
	}

	return hit;
}