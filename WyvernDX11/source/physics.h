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

struct ldiPhysicsMesh {
	PxTriangleMeshGeometry	cookedMesh;
};

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
	meshDesc.points.count = Model->verts.size();
	meshDesc.points.stride = sizeof(ldiMeshVertex);
	meshDesc.points.data = Model->verts.data();

	meshDesc.triangles.count = Model->indices.size() / 3;
	meshDesc.triangles.stride = 3 * sizeof(uint32_t);
	meshDesc.triangles.data = Model->indices.data();

	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	//bool status = Physics->cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);

	double t0 = _getTime(Physics->appContext);
	PxTriangleMesh* cookedMesh = Physics->cooking->createTriangleMesh(meshDesc, Physics->physics->getPhysicsInsertionCallback(), &result);
	t0 = _getTime(Physics->appContext) - t0;
	std::cout << "Mesh cooked " << result << " in " << t0 * 1000.0f << " ms\n";

	if (result != PxTriangleMeshCookingResult::Enum::eSUCCESS) {
		std::cout << "Failed to cook\n";
		return 1;
	}

	Result->cookedMesh.triangleMesh = cookedMesh;
	
	return 0;
}

struct ldiRaycastResult {
	bool hit;
	int faceIdx;
	vec3 pos;
	vec2 barry;
};

ldiRaycastResult physicsRaycast(ldiPhysics* Physics, ldiPhysicsMesh* Mesh, vec3 RayOrigin, vec3 RayDir) {
	ldiRaycastResult hit{};
	hit.hit = false;

	PxVec3 rayOrigin(RayOrigin.x, RayOrigin.y, RayOrigin.z);
	PxVec3 rayDir(RayDir.x, RayDir.y, RayDir.z);
	rayDir.normalize();
	PxTransform geomPose(0, 0, 0);

	PxRaycastHit rayHit;
	int hitCount = PxGeometryQuery::raycast(rayOrigin, rayDir, Mesh->cookedMesh, geomPose, 0.02, PxHitFlag::ePOSITION | PxHitFlag::eUV | PxHitFlag::eFACE_INDEX | PxHitFlag::eMESH_BOTH_SIDES, 1, &rayHit);


	if (hitCount > 0) {
		hit.hit = true;
		hit.faceIdx = Mesh->cookedMesh.triangleMesh->getTrianglesRemap()[rayHit.faceIndex];
		hit.pos = vec3(rayHit.position.x, rayHit.position.y, rayHit.position.z);
		hit.barry = vec2(rayHit.u, rayHit.v);
	}

	return hit;
}