#pragma once

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#include "spatialGrid.h"

struct ldiLaserViewSurfel {
	int id;
	vec2 screenPos;
	float angle;
};

struct ldiLaserViewPathPos {
	vec3 worldPos;
	vec3 worldDir;
	
	vec3 surfacePos;
	vec3 surfaceNormal;
	mat4 modelToView;
	// Machine coords

	// Surfels
	std::vector<int> surfelIds;
	std::vector<ldiLaserViewSurfel> surfels;
};

struct ldiCoverageConstantBuffer {
	int slotIndex;
	int __pad[3];
};

struct ldiModelInspector {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;

	bool						primaryModelShowWireframe = false;
	bool						primaryModelShowShaded = false;
	bool						showPointCloud = false;
	bool						quadMeshShowWireframe = false;
	bool						quadMeshShowDebug = false;
	bool						showSurfels = false;
	bool						showPoissonSamples = true;
	bool						showSpatialCells = false;
	bool						showSpatialBounds = false;
	bool						showSampleSites = false;
	bool						showBounds = false;
	bool						showScaleHelper = false;
	bool						showQuadMeshWhite = true;

	float						pointWorldSize = 0.01f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	float						cameraSpeed = 0.5f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };
	vec4						modelCanvasColor = { 1.0f, 1.0f, 1.0f, 1.00f };

	ldiQuadModel				quadModel;
	ldiModel					dergnModel;
	ldiRenderModel				dergnRenderModel;
	ldiRenderModel				dergnDebugModel;
	ldiRenderModel				quadModelWhite;
	
	ID3D11Texture2D*			baseImageTexture;
	ID3D11ShaderResourceView*	shaderResourceViewTest;
	ID3D11SamplerState*			texSamplerState;

	ID3D11Texture2D*			laserViewTex;
	ID3D11ShaderResourceView*	laserViewTexView;
	ID3D11RenderTargetView*		laserViewTexRtView;
	ID3D11Texture2D*			laserViewDepthStencil;
	ID3D11DepthStencilView*		laserViewDepthStencilView;
	ID3D11ShaderResourceView*	laserViewDepthStencilSrv;
	
	int							laserViewSize = 1024;
	
	ldiRenderLines				quadMeshWire;
	ldiRenderModel				surfelRenderModel;
	ldiRenderModel				surfelHighRenderModel;
	ldiRenderModel				poissonSamplesRenderModel;

	ldiRenderPointCloud			pointCloudRenderModel;

	ldiPhysicsMesh				cookedDergn;

	// NOTE: Low rank surfels.
	std::vector<ldiSurfel>		surfels;
	// NOTE: High rank surfels.
	std::vector<ldiSurfel>		surfelsHigh;

	ldiImage					baseTexture;

	ldiSpatialGrid				spatialGrid;
	ldiPoissonSpatialGrid		poissonSpatialGrid;

	ldiPointCloud				pointDistrib;
	ldiRenderPointCloud			pointDistribCloud;
	vec3						laserViewRot;
	vec3						laserViewPos;

	int							selectedSurfel = 0;
	int							selectedLaserView = -1;
	bool						laserViewShowBackground = false;

	ID3D11ComputeShader*		calcPointScoreComputeShader;
	ID3D11ComputeShader*		accumulatePointScoreComputeShader;
	ID3D11Buffer*				coverageBuffer;
	ID3D11UnorderedAccessView*	coverageBufferUav;
	ID3D11ShaderResourceView*	coverageBufferSrv;
	ID3D11Buffer*				coverageStagingBuffer;
	ldiRenderModel				coveragePointModel;
	ID3D11VertexShader*			coveragePointVertexShader;
	ID3D11PixelShader*			coveragePointPixelShader;
	ID3D11InputLayout*			coveragePointInputLayout;
	ID3D11VertexShader*			coveragePointWriteVertexShader;
	ID3D11PixelShader*			coveragePointWritePixelShader;
	ID3D11PixelShader*			coveragePointWriteNoCoveragePixelShader;
	ID3D11Buffer*				coverageAreaBuffer;
	ID3D11UnorderedAccessView*	coverageAreaBufferUav;
	ID3D11Buffer*				coverageAreaStagingBuffer;
	ID3D11Buffer*				coverageConstantBuffer;
	ID3D11Buffer*				surfelMaskBuffer;
	ID3D11ShaderResourceView*	surfelMaskBufferSrv;
	int							coverageValue;
	int*						surfelMask;

	std::vector<ldiLaserViewPathPos> laserViewPathPositions;
	std::vector<int>			laserViewPath;
};

void geoSmoothSurfels(std::vector<ldiSurfel>* Surfels) {
	std::cout << "Run smooth\n";
	for (size_t i = 0; i < Surfels->size(); ++i) {
		(*Surfels)[i].normal = vec3(0, 1, 0);
	}
}

void geoCreateSurfels(ldiQuadModel* Model, std::vector<ldiSurfel>* Result) {
	int quadCount = Model->indices.size() / 4;

	Result->clear();
	Result->reserve(quadCount);

	float normalAdjust = 0.0;

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 p1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 p2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 p3 = Model->verts[Model->indices[i * 4 + 3]];

		vec3 center = p0 + p1 + p2 + p3;
		center /= 4;

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		vec3 normal0 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		vec3 normal1 = glm::normalize(glm::cross(p3 - p2, p1 - p2));

		vec3 normal = normal0 + normal1;
		normal = glm::normalize(normal);

		ldiSurfel surfel{};
		surfel.id = i;
		surfel.position = center;// + normal * normalAdjust;
		surfel.normal = normal;
		surfel.color = vec3(0, 0, 0);
		surfel.scale = 0.0075f;

		Result->push_back(surfel);
	}
}

void geoCreateSurfelsHigh(ldiQuadModel* Model, std::vector<ldiSurfel>* Result) {
	int quadCount = Model->indices.size() / 4;

	Result->clear();
	Result->reserve(quadCount);

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 p1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 p2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 p3 = Model->verts[Model->indices[i * 4 + 3]];

		vec3 center = p0 + p1 + p2 + p3;
		center /= 4;

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		vec3 normal0 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		vec3 normal1 = glm::normalize(glm::cross(p3 - p2, p1 - p2));

		vec3 normal = normal0 + normal1;
		normal = glm::normalize(normal);

		vec3 s[4];

		// Bilinear interpolation of corners.
		vec3 a = (p0 + (p1 - p0) * 0.25f);
		vec3 b = (p3 + (p2 - p3) * 0.25f);
		s[0] = (a + (b - a) * 0.25f);
		s[1] = (a + (b - a) * 0.75f);

		a = (p0 + (p1 - p0) * 0.75f);
		b = (p3 + (p2 - p3) * 0.75f);
		s[2] = (a + (b - a) * 0.25f);
		s[3] = (a + (b - a) * 0.75f);

		for (int sIter = 0; sIter < 4; ++sIter) {
			ldiSurfel surfel{};
			surfel.id = i;
			surfel.normal = normal;
			surfel.color = vec3(0.5f, 0.5f, 0.5f);
			surfel.scale = 0.0075f;
			surfel.position = s[sIter];

			Result->push_back(surfel);
		}
	}
}

struct ldiColorTransferThreadContext {
	ldiPhysics* physics;
	ldiPhysicsMesh* cookedMesh;
	ldiModel* srcModel;
	ldiImage* image;
	std::vector<ldiSurfel>* surfels;
	int	startIdx;
	int	endIdx;
};

void geoTransferThreadBatch(ldiColorTransferThreadContext Context) {
	float normalAdjust = 0.01;

	for (size_t i = Context.startIdx; i < Context.endIdx; ++i) {
		ldiSurfel* s = &(*Context.surfels)[i];
		ldiRaycastResult result = physicsRaycast(Context.physics, Context.cookedMesh, s->position + s->normal * normalAdjust, -s->normal);

		if (result.hit) {
			ldiMeshVertex v0 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 0]];
			ldiMeshVertex v1 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 1]];
			ldiMeshVertex v2 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 2]];

			float u = result.barry.x;
			float v = result.barry.y;
			float w = 1.0 - (u + v);
			vec2 uv = w * v0.uv + u * v1.uv + v * v2.uv;

			// TODO: Clamp/Wrap lookup index on each axis.
			int pixX = uv.x * Context.image->width;
			int pixY = Context.image->height - (uv.y * Context.image->height);

			uint8_t r = Context.image->data[(pixX + pixY * Context.image->width) * 4 + 0];
			uint8_t g = Context.image->data[(pixX + pixY * Context.image->width) * 4 + 1];
			uint8_t b = Context.image->data[(pixX + pixY * Context.image->width) * 4 + 2];

			s->color = vec3(r / 255.0, g / 255.0, b / 255.0);
		}
		else {
			s->color = vec3(1, 0, 0);
		}
	}
}

void _geoTransferColorToSurfels(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiImage* Image, std::vector<ldiSurfel>* Surfels) {
	float normalAdjust = 0.01;
	
	for (size_t i = 0; i < Surfels->size(); ++i) {
		ldiSurfel* s = &(*Surfels)[i];
		ldiRaycastResult result = physicsRaycast(AppContext->physics, CookedMesh, s->position + s->normal * normalAdjust, -s->normal);

		if (result.hit) {
			ldiMeshVertex v0 = SrcModel->verts[SrcModel->indices[result.faceIdx * 3 + 0]];
			ldiMeshVertex v1 = SrcModel->verts[SrcModel->indices[result.faceIdx * 3 + 1]];
			ldiMeshVertex v2 = SrcModel->verts[SrcModel->indices[result.faceIdx * 3 + 2]];

			float u = result.barry.x;
			float v = result.barry.y;
			float w = 1.0 - (u + v);
			vec2 uv = w * v0.uv + u * v1.uv + v * v2.uv;

			// TODO: Clamp/Wrap lookup index on each axis.
			int pixX = uv.x * Image->width;
			int pixY = Image->height - (uv.y * Image->height);

			uint8_t r = Image->data[(pixX + pixY * Image->width) * 4 + 0];
			uint8_t g = Image->data[(pixX + pixY * Image->width) * 4 + 1];
			uint8_t b = Image->data[(pixX + pixY * Image->width) * 4 + 2];

			s->color = vec3(r / 255.0, g / 255.0, b / 255.0);
		} else {
			s->color = vec3(1, 0, 0);
		}
	}
}

void geoTransferColorToSurfels(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiImage* Image, std::vector<ldiSurfel>* Surfels) {
	double t0 = _getTime(AppContext);
	
	const int threadCount = 20;
	int batchSize = Surfels->size() / threadCount;
	int batchRemainder = Surfels->size() - (batchSize * threadCount);
	std::thread workerThread[threadCount];

	for (int t = 0; t < threadCount; ++t) {
		ldiColorTransferThreadContext tc{};
		tc.physics = AppContext->physics;
		tc.cookedMesh = CookedMesh;
		tc.srcModel = SrcModel;
		tc.image = Image;
		tc.surfels = Surfels;
		tc.startIdx = t * batchSize;
		tc.endIdx = (t + 1) * batchSize;

		if (t == threadCount - 1) {
			tc.endIdx += batchRemainder;
		}

		workerThread[t] = std::move(std::thread(geoTransferThreadBatch, tc));
	}

	for (int t = 0; t < threadCount; ++t) {
		workerThread[t].join();
	}

	t0 = _getTime(AppContext) - t0;
	std::cout << "Transfer color: " << t0 * 1000.0f << " ms\n";
}

void modelInspectorVoxelize(ldiModelInspector* ModelInspector) {
	double t0 = _getTime(ModelInspector->appContext);
	stlSaveModel("../cache/source.stl", &ModelInspector->dergnModel);
	t0 = _getTime(ModelInspector->appContext) - t0;
	std::cout << "Save STL: " << t0 * 1000.0f << " ms\n";

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char args[] = "PolyMender-clean.exe ../../bin/cache/source.stl 11 0.9 ../../bin/cache/voxel.ply";

	CreateProcessA(
		"../../assets/bin/PolyMender-clean.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		"../../assets/bin/",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);
}

void modelInspectorCreateQuadMesh(ldiModelInspector* ModelInspector) {
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// 100um sides and smoothing.
	//char args[] = "\"Instant Meshes.exe\" ../../bin/cache/voxel.ply -o ../../bin/cache/output.ply --scale 0.02 --smooth 2";
	// 75um sides.
	//char args[] = "\"Instant Meshes.exe\" ../../bin/cache/voxel.ply -o ../../bin/cache/output.ply --scale 0.015";
	// 150um sides for multi-resolution surfels.
	char args[] = "\"Instant Meshes.exe\" ../../bin/cache/voxel.ply -o ../../bin/cache/output.ply --scale 0.030";
	
	CreateProcessA(
		"../../assets/bin/Instant Meshes.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0, //CREATE_NEW_CONSOLE,
		NULL,
		"../../assets/bin",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);
}

struct ldiSmoothNormalsThreadContext {
	ldiSpatialGrid*			grid;
	std::vector<ldiSurfel>*	surfels;
	int						startIdx;
	int						endIdx;
};

void surfelsSmoothNormalsThreadBatch(ldiSmoothNormalsThreadContext Context) {
	for (int i = Context.startIdx; i < Context.endIdx; ++i) {
		ldiSurfel* srcSurfel = &(*Context.surfels)[i];
		vec3 avgNorm = srcSurfel->normal;
		int avgCount = 1;

		vec3 cell = spatialGridGetCellFromWorldPosition(Context.grid, srcSurfel->position);

		int sX = (int)(cell.x - 0.5f);
		int eX = sX + 1;

		int sY = (int)(cell.y - 0.5f);
		int eY = sY + 1;

		int sZ = (int)(cell.z - 0.5f);
		int eZ = sZ + 1;

		for (int iZ = sZ; iZ <= eZ; ++iZ) {
			for (int iY = sY; iY <= eY; ++iY) {
				for (int iX = sX; iX <= eX; ++iX) {
					ldiSpatialCellResult cellResult = spatialGridGetCell(Context.grid, iX, iY, iZ);
					for (int s = 0; s < cellResult.count; ++s) {
						int surfelId = cellResult.data[s];

						if (surfelId != i) {
							ldiSurfel* dstSurfel = &(*Context.surfels)[surfelId];

							float dist = glm::length(dstSurfel->position - srcSurfel->position);

							if (dist < 0.02f) {
								avgNorm += dstSurfel->normal * (1.0f - (dist / 0.02f));
								avgCount += 1;
							}
						}
					}
				}
			}
		}

		avgNorm = glm::normalize(avgNorm);
		srcSurfel->smoothedNormal = avgNorm;
	}
}

void surfelsCreateDistribution(ldiSpatialGrid* Grid, std::vector<ldiSurfel>* Surfels, ldiPointCloud* Result, int* SurfelMask) {
	bool* mask = new bool[Surfels->size()];
	memset(mask, 0, Surfels->size() * sizeof(bool));
	Result->points.clear();

	for (size_t i = 0; i < Surfels->size(); ++i) {
	//for (size_t i = 10000; i < 10001; ++i) {
		if (SurfelMask[i] > 0 || mask[i] == true) {
			continue;
		}

		ldiSurfel* srcSurfel = &(*Surfels)[i];

		vec3 avgNorm = srcSurfel->normal;

		vec3 cell = spatialGridGetCellFromWorldPosition(Grid, srcSurfel->position);

		int sX = (int)(cell.x - 0.5f) - 4;
		int eX = sX + 9;

		int sY = (int)(cell.y - 0.5f) - 4;
		int eY = sY + 9;

		int sZ = (int)(cell.z - 0.5f) - 4;
		int eZ = sZ + 9;

		sX = max(0, sX);
		eX = min(Grid->countX - 1, eX);

		sY = max(0, sY);
		eY = min(Grid->countY - 1, eY);

		sZ = max(0, sZ);
		eZ = min(Grid->countZ - 1, eZ);

		float distVal = 0.2f + 0.5f + 0.5f + 0.5f;

		for (int iZ = sZ; iZ <= eZ; ++iZ) {
			for (int iY = sY; iY <= eY; ++iY) {
				for (int iX = sX; iX <= eX; ++iX) {
					ldiSpatialCellResult cellResult = spatialGridGetCell(Grid, iX, iY, iZ);
					for (int s = 0; s < cellResult.count; ++s) {
						int surfelId = cellResult.data[s];

						if (surfelId != i && SurfelMask[surfelId] == 0) {
							ldiSurfel* dstSurfel = &(*Surfels)[surfelId];

							float dist = glm::length(dstSurfel->position - srcSurfel->position);

							if (dist < distVal) {
								mask[surfelId] = true;
								avgNorm += dstSurfel->normal * (1.0f - (dist / distVal));
							}
						}
					}
				}
			}
		}

		ldiPointCloudVertex newPoint = {};
		newPoint.normal = glm::normalize(avgNorm);
		newPoint.position = srcSurfel->position;// + newPoint.normal * 20.0f;
		newPoint.color = vec3(1, 0, 0);// newPoint.normal * 0.5f + 0.5f;
		Result->points.push_back(newPoint);
	}

	delete[] mask;
}

int modelInspectorInit(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->appContext = AppContext;
	ModelInspector->mainViewWidth = 0;
	ModelInspector->mainViewHeight = 0;
	
	ModelInspector->camera = {};
	ModelInspector->camera.position = vec3(0, 0, 10);
	ModelInspector->camera.rotation = vec3(0, 0, 0);
	ModelInspector->camera.fov = 60.0f;

	return 0;
}

struct ldiMachineTransform {
	vec3 viewPos;
	vec3 viewRot;
	mat4 projViewModelMat;
	vec3 modelLocalCamPos;
};

ldiMachineTransform modelInspectorGetLaserViewModelToView(mat4 ProjMat, vec3 SurfacePos, vec3 SurfaceNormal) {
	ldiMachineTransform result = {};

	// Convert world transform to machine movement.
	vec3 modelLocalTargetPos = SurfacePos;
	result.modelLocalCamPos = SurfacePos + SurfaceNormal * 20.0f;
	vec3 viewDir = glm::normalize(-SurfaceNormal);

	result.viewRot.y = viewDir.y * -90.0f;
	result.viewRot.x = glm::degrees(atan2(-viewDir.z, -viewDir.x));

	mat4 modelLocalMat = glm::rotate(mat4(1.0f), glm::radians(result.viewRot.x), vec3Up);
	vec3 modelWorldTargetPos = modelLocalMat * vec4(vec3(0.0f, 7.5, 0.0f) - modelLocalTargetPos, 1.0f);

	// Laser tool head movement.
	mat4 laserHeadMat = glm::translate(mat4(1.0f), vec3(0, 7.5f, 0.0f));
	laserHeadMat = glm::rotate(laserHeadMat, glm::radians(result.viewRot.y), -vec3Forward);
	laserHeadMat = glm::translate(laserHeadMat, vec3(20.0f, 0, 0.0f));
	laserHeadMat = glm::rotate(laserHeadMat, glm::radians(90.0f), vec3Up);

	result.viewPos = modelWorldTargetPos;

	// Model movement.
	mat4 modelMat = glm::translate(mat4(1.0f), result.viewPos);
	modelMat = glm::rotate(modelMat, glm::radians(result.viewRot.x), vec3Up);

	mat4 camViewMat = glm::inverse(laserHeadMat);
	mat4 camViewModelMat = camViewMat * modelMat;

	result.projViewModelMat = ProjMat * camViewModelMat;

	return result;
}

struct ldiLaserViewCoveragePrep {
	mat4 projMat;
};

ldiLaserViewCoveragePrep modelInspectorLaserViewCoveragePrep(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ID3D11RenderTargetView* renderTargets[] = {
		ModelInspector->laserViewTexRtView
	};

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = ModelInspector->laserViewSize;
	viewport.Height = ModelInspector->laserViewSize;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	AppContext->d3dDeviceContext->RSSetViewports(1, &viewport);

	ldiLaserViewCoveragePrep result = {};
	result.projMat = glm::perspectiveFovRH_ZO(glm::radians(11.478f), (float)ModelInspector->laserViewSize, (float)ModelInspector->laserViewSize, 0.1f, 100.0f);

	AppContext->d3dDeviceContext->OMSetRenderTargets(1, renderTargets, ModelInspector->laserViewDepthStencilView);
	float bkgColor[] = { 0.5, 0.5, 0.5, 1.0 };
	AppContext->d3dDeviceContext->ClearRenderTargetView(ModelInspector->laserViewTexRtView, (float*)&bkgColor);
	AppContext->d3dDeviceContext->ClearDepthStencilView(ModelInspector->laserViewDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	UINT initValue[] = { 0, 0, 0, 0 };
	AppContext->d3dDeviceContext->ClearUnorderedAccessViewUint(ModelInspector->coverageAreaBufferUav, initValue);

	return result;
}

void modelInspectorLaserViewCoverageRender(ldiApp* AppContext, ldiModelInspector* ModelInspector, ldiLaserViewCoveragePrep* Prep, vec3 SurfacePos, vec3 SurfaceNormal, int SlotId, bool UpdateCoverage) {
	ID3D11RenderTargetView* renderTargets[] = {
		ModelInspector->laserViewTexRtView
	};

	AppContext->d3dDeviceContext->OMSetRenderTargets(1, renderTargets, ModelInspector->laserViewDepthStencilView);
	AppContext->d3dDeviceContext->ClearDepthStencilView(ModelInspector->laserViewDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// TODO: Transform should be passed into this function.
	ldiMachineTransform machineTransform = modelInspectorGetLaserViewModelToView(Prep->projMat, SurfacePos, SurfaceNormal);
	ModelInspector->laserViewRot = machineTransform.viewRot;
	ModelInspector->laserViewPos = machineTransform.viewPos;
	mat4 projViewModelMat = machineTransform.projViewModelMat;

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(ModelInspector->coverageConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCoverageConstantBuffer* constantBuffer = (ldiCoverageConstantBuffer*)ms.pData;
		constantBuffer->slotIndex = SlotId;
		AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageConstantBuffer, 0);
	}

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(AppContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(machineTransform.modelLocalCamPos, 1);
		AppContext->d3dDeviceContext->Unmap(AppContext->mvpConstantBuffer, 0);
	}

	{
		ldiRenderModel* Model = &ModelInspector->dergnDebugModel;
		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;

		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
		AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);
		AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
		AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
	}

	{	
		UINT lgStride = sizeof(ldiCoveragePointVertex);
		UINT lgOffset = 0;

		ID3D11Buffer* cbfrs[] = {
			AppContext->mvpConstantBuffer,
			ModelInspector->coverageConstantBuffer
		};

		ID3D11ShaderResourceView* psSrvs[] = {
			ModelInspector->laserViewDepthStencilSrv,
			ModelInspector->surfelMaskBufferSrv
		};
		
		ID3D11UnorderedAccessView* uavs[] = {
			ModelInspector->coverageBufferUav,
			ModelInspector->coverageAreaBufferUav
		};

		AppContext->d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(1, renderTargets, NULL, 1, 2, uavs, 0);

		AppContext->d3dDeviceContext->IASetInputLayout(ModelInspector->coveragePointInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &ModelInspector->coveragePointModel.vertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetIndexBuffer(ModelInspector->coveragePointModel.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->VSSetShader(ModelInspector->coveragePointWriteVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->VSSetShaderResources(0, 1, &ModelInspector->coverageBufferSrv);

		if (UpdateCoverage) {
			AppContext->d3dDeviceContext->PSSetShader(ModelInspector->coveragePointWritePixelShader, 0, 0);
		} else {
			AppContext->d3dDeviceContext->PSSetShader(ModelInspector->coveragePointWriteNoCoveragePixelShader, 0, 0);
		}

		AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 2, cbfrs);
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 2, psSrvs);

		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &AppContext->defaultPointSamplerState);
		AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->noDepthState, 0);
		AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
		AppContext->d3dDeviceContext->DrawIndexed(ModelInspector->coveragePointModel.indexCount, 0, 0);

		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, nullSRV);
	}
}

bool modelInspectorCalculateLaserViewCoverage(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	double t0 = _getTime(AppContext);

	D3D11_QUERY_DESC queryDesc = {};
	queryDesc.Query = D3D11_QUERY_EVENT;

	ID3D11Query* query;
	if (AppContext->d3dDevice->CreateQuery(&queryDesc, &query) != S_OK) {
		return false;
	}

	ldiLaserViewCoveragePrep prep = modelInspectorLaserViewCoveragePrep(AppContext, ModelInspector);

	for (size_t i = 0; i < ModelInspector->pointDistrib.points.size(); ++i) {
	//for (size_t i = 0; i < 100; ++i) {
		ldiPointCloudVertex* vert = &ModelInspector->pointDistrib.points[i];
		modelInspectorLaserViewCoverageRender(AppContext, ModelInspector, &prep, vert->position, vert->normal, i, true);
	}

	AppContext->d3dDeviceContext->End(query);
	BOOL queryData;

	while (S_OK != AppContext->d3dDeviceContext->GetData(query, &queryData, sizeof(BOOL), 0)) {
	}

	if (queryData != TRUE) {
		return false;
	}

	t0 = _getTime(AppContext) - t0;
	std::cout << "Coverage: " << t0 * 1000.0f << " ms\n";

	AppContext->d3dDeviceContext->CopyResource(ModelInspector->coverageAreaStagingBuffer, ModelInspector->coverageAreaBuffer);

	// Find best view.
	int maxArea = 0;
	int maxAreaId = -1;

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT result = AppContext->d3dDeviceContext->Map(ModelInspector->coverageAreaStagingBuffer, 0, D3D11_MAP_READ, 0, &ms);
	int* areaResults = (int*)ms.pData;

	for (size_t i = 0; i < ModelInspector->pointDistrib.points.size(); ++i) {
		int area = areaResults[i];
		//std::cout << i << ": " << area << "\n";

		if (area > maxArea) {
			maxArea = area;
			maxAreaId = i;
		}
	}

	std::cout << "Max: " << maxAreaId << " - " << maxArea << "\n";

	AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageAreaStagingBuffer, 0);

	return true;
}

float GammaToLinear(float In) {
	float linearRGBLo = In / 12.92f;
	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
}

float LinearToGamma(float In) {
	float sRGBLo = In * 12.92f;
	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
}

bool modelInspectorCalculateLaserViewPath(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->laserViewPathPositions.clear();

	struct ldiSurfelViewCoverage {
		int viewId;
		float angle;

	};

	struct ldiViewInfo {
		mat4 modelToView;
	};

	std::vector<ldiSurfel> poissonSamples;

	for (int viewIter = 0; viewIter < 200; ++viewIter) {
		std::cout << "Pass: " << viewIter << "\n";

		//----------------------------------------------------------------------------------------------------
		// Generate view sites.
		//----------------------------------------------------------------------------------------------------
		double t0 = _getTime(AppContext);
		surfelsCreateDistribution(&ModelInspector->spatialGrid, &ModelInspector->surfels, &ModelInspector->pointDistrib, ModelInspector->surfelMask);
		t0 = _getTime(AppContext) - t0;
		std::cout << "Surfel distribution: " << t0 * 1000.0f << " ms Points: " << ModelInspector->pointDistrib.points.size() << "\n";

		// Upload mask to GPU.
		AppContext->d3dDeviceContext->UpdateSubresource(ModelInspector->surfelMaskBuffer, 0, NULL, ModelInspector->surfelMask, 0, 0);

		//----------------------------------------------------------------------------------------------------
		// Get 'best' view site.
		//----------------------------------------------------------------------------------------------------
		t0 = _getTime(AppContext);

		// TODO: Move query outside loop.
		D3D11_QUERY_DESC queryDesc = {};
		queryDesc.Query = D3D11_QUERY_EVENT;

		ID3D11Query* query;
		if (AppContext->d3dDevice->CreateQuery(&queryDesc, &query) != S_OK) {
			return false;
		}

		ldiLaserViewCoveragePrep prep = modelInspectorLaserViewCoveragePrep(AppContext, ModelInspector);

		for (size_t i = 0; i < ModelInspector->pointDistrib.points.size(); ++i) {
			ldiPointCloudVertex* vert = &ModelInspector->pointDistrib.points[i];
			modelInspectorLaserViewCoverageRender(AppContext, ModelInspector, &prep, vert->position, vert->normal, i, false);
		}

		AppContext->d3dDeviceContext->End(query);
		BOOL queryData;

		while (S_OK != AppContext->d3dDeviceContext->GetData(query, &queryData, sizeof(BOOL), 0)) {
		}

		if (queryData != TRUE) {
			return false;
		}

		t0 = _getTime(AppContext) - t0;
		std::cout << "Coverage: " << t0 * 1000.0f << " ms\n";

		AppContext->d3dDeviceContext->CopyResource(ModelInspector->coverageAreaStagingBuffer, ModelInspector->coverageAreaBuffer);

		// Find best view.
		int maxArea = 0;
		int maxAreaId = -1;

		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT result = AppContext->d3dDeviceContext->Map(ModelInspector->coverageAreaStagingBuffer, 0, D3D11_MAP_READ, 0, &ms);
		int* areaResults = (int*)ms.pData;

		for (size_t i = 0; i < ModelInspector->pointDistrib.points.size(); ++i) {
			int area = areaResults[i];
			//std::cout << i << ": " << area << "\n";

			if (area > maxArea) {
				maxArea = area;
				maxAreaId = i;
			}
		}

		std::cout << "Max: " << maxAreaId << " - " << maxArea << "\n";

		AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageAreaStagingBuffer, 0);

		if (maxAreaId == -1) {
			std::cout << "No more area to fill\n";
			break;
		}

		// TODO: TEMP
		//maxAreaId = 347;

		//----------------------------------------------------------------------------------------------------
		// Re-render best view and get all samples related to it.
		//----------------------------------------------------------------------------------------------------
		UINT clearValue[] = { 0, 0, 0, 0 };
		AppContext->d3dDeviceContext->ClearUnorderedAccessViewUint(ModelInspector->coverageBufferUav, clearValue);

		prep = modelInspectorLaserViewCoveragePrep(AppContext, ModelInspector);
	
		ldiPointCloudVertex* vert = &ModelInspector->pointDistrib.points[maxAreaId];
		modelInspectorLaserViewCoverageRender(AppContext, ModelInspector, &prep, vert->position, vert->normal, 0, true);

		AppContext->d3dDeviceContext->CopyResource(ModelInspector->coverageStagingBuffer, ModelInspector->coverageBuffer);
		AppContext->d3dDeviceContext->Map(ModelInspector->coverageStagingBuffer, 0, D3D11_MAP_READ, 0, &ms);
		int* coverageBufferData = (int*)ms.pData;

		int activeSurfels = 0;

		ldiLaserViewPathPos pathPos = {};
		pathPos.surfacePos = vert->position;
		pathPos.surfaceNormal = vert->normal;

		for (size_t i = 0; i < ModelInspector->surfels.size(); ++i) {
			// NOTE: 707 == 45degrees to laser view.
			// NOTE: 500 == 60degrees to laser view.
			if (ModelInspector->surfelMask[i] == 0 && coverageBufferData[i] >= 500) {
				++activeSurfels;
				ModelInspector->surfelMask[i] = viewIter + 1;
				pathPos.surfelIds.push_back(i);
			}
		}

		// NOTE: Unmap happens later.

		std::cout << "Active surfels: " << activeSurfels << "\n";

		if (activeSurfels == 0) {
			AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageStagingBuffer, 0);
			break;
		}

		//----------------------------------------------------------------------------------------------------
		// Calculate surfel positions
		//----------------------------------------------------------------------------------------------------
		if (false) {
			ldiMachineTransform machineTransform = modelInspectorGetLaserViewModelToView(prep.projMat, pathPos.surfacePos, pathPos.surfaceNormal);

			t0 = _getTime(AppContext);

			for (size_t i = 0; i < pathPos.surfelIds.size(); ++i) {
				// NOTE: Each low rank surfel has 4 high rankers.
				int surfelId = pathPos.surfelIds[i];
				float angle = coverageBufferData[surfelId] / 1000.0f;

				for (size_t j = 0; j < 4; ++j) {
					//ldiSurfel* s = &ModelInspector->surfels[surfelId];
					ldiSurfel* s = &ModelInspector->surfelsHigh[surfelId * 4 + j];

					ldiLaserViewSurfel viewSurfel = {};
					viewSurfel.id = surfelId;
					viewSurfel.angle = angle;

					vec4 clipSpace = machineTransform.projViewModelMat * vec4(s->position, 1.0f);
					clipSpace.x /= clipSpace.w;
					clipSpace.y /= clipSpace.w;
					clipSpace.z /= clipSpace.w;

					clipSpace = clipSpace * 0.5f + 0.5f;

					viewSurfel.screenPos = vec2(clipSpace.x * 1024.0f, (1.0f - clipSpace.y) * 1024.0f);
			
					pathPos.surfels.push_back(viewSurfel);
				}
			}

			t0 = _getTime(AppContext) - t0;
			std::cout << "Transform surfels: " << t0 * 1000.0f << " ms\n";

			ModelInspector->laserViewPathPositions.push_back(pathPos);
		}

		AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageStagingBuffer, 0);

		//----------------------------------------------------------------------------------------------------
		// Distribute poisson samples.
		//----------------------------------------------------------------------------------------------------
		{
			ldiMachineTransform machineTransform = modelInspectorGetLaserViewModelToView(prep.projMat, pathPos.surfacePos, pathPos.surfaceNormal);

			t0 = _getTime(AppContext);

			// Create sample candidates.
			int candidatesPerSide = 8;
			int candidatesPerSurfel = candidatesPerSide * candidatesPerSide;

			int totalCandidates = pathPos.surfelIds.size() * candidatesPerSurfel;

			std::cout << "Total candidates: " << totalCandidates << "\n";

			ldiPoissonSpatialGrid* sampleGrid = &ModelInspector->poissonSpatialGrid;

			std::vector<ldiSurfel> poissonCandidates;
			poissonCandidates.reserve(totalCandidates);

			// Randomly walk valid low rank surfels.
			for (size_t i = 0; i < pathPos.surfelIds.size(); ++i) {
				int surfelId = pathPos.surfelIds[i];

				// Get quad for this surfel.
				vec3 v0 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[surfelId * 4 + 0]];
				vec3 v1 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[surfelId * 4 + 1]];
				vec3 v2 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[surfelId * 4 + 2]];
				vec3 v3 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[surfelId * 4 + 3]];

				vec3 mid = (v0 + v1 + v2 + v3) / 4.0f;

				vec3 normal = ModelInspector->surfels[surfelId].normal;

				float s0 = glm::length(v0 - v1);
				float s1 = glm::length(v1 - v2);
				float s2 = glm::length(v2 - v3);
				float s3 = glm::length(v3 - v0);

				// For each required surfel, get the position.
				float divSize = 1.0f / (candidatesPerSide);
				float divHalf = divSize * 0.5f;

				for (int iY = 0; iY < candidatesPerSide; ++iY) {
					float lerpY = divSize * iY + divHalf;

					for (int iX = 0; iX < candidatesPerSide; ++iX) {
						float lerpX = divSize * iX + divHalf;
						vec3 a = (v0 + (v1 - v0) * lerpX);
						vec3 b = (v3 + (v2 - v3) * lerpX);

						vec3 pos = (a + (b - a) * lerpY);

						int idX = lerpX * 2.0f;
						int idY = lerpY * 2.0f;
						vec3 color = ModelInspector->surfelsHigh[surfelId * 4 + (idX * 2 + idY)].color;

						//float lum = 1.0f;
						float lum = 1.0f - (color.r * 0.2126f + color.g * 0.7152f + color.b * 0.0722f);
						//float lum = 1.0f - pow(GammaToLinear(color.r), 1.4);
						//float lum = 1.0f - GammaToLinear(color.r);

						// NOTE: Luminance cutoff. Can't represent values this light.
						if (lum < 0.025f) {
							continue;
						}

						ldiSurfel candidateSurfel = {};
						candidateSurfel.color = vec3(lum, lum, lum);
						candidateSurfel.position = pos;
						candidateSurfel.normal = normal;
						candidateSurfel.scale = 0.0075f * lum;
						//candidateSurfel.scale = 0.0075f;

						poissonCandidates.push_back(candidateSurfel);
					}
				}
			}

			std::vector<int> candidateShuffles;
			candidateShuffles.resize(poissonCandidates.size());

			for (size_t iterShuff = 0; iterShuff < poissonCandidates.size(); ++iterShuff) {
				candidateShuffles[iterShuff] = iterShuff;
			}

			for (size_t iterShuff = 0; iterShuff < poissonCandidates.size(); ++iterShuff) {
				int swapIdx = rand() % poissonCandidates.size();

				int temp = candidateShuffles[iterShuff];
				candidateShuffles[iterShuff] = candidateShuffles[swapIdx];
				candidateShuffles[swapIdx] = temp;
			}

			vec3 rndCol;
			rndCol.x = (rand() % 255) / 255.0f;
			rndCol.y = (rand() % 255) / 255.0f;
			rndCol.z = (rand() % 255) / 255.0f;

			//----------------------------------------------------------------------------------------------------
			// Turn candidates into samples.
			//----------------------------------------------------------------------------------------------------
			for (size_t iterCandidate = 0; iterCandidate < candidateShuffles.size(); ++iterCandidate) {
				ldiSurfel* cand = &poissonCandidates[candidateShuffles[iterCandidate]];
				vec3 cellPos = poissonSpatialGridGetCellFromWorldPosition(sampleGrid, cand->position);

				int cXs = (int)(cellPos.x - 0.5f);
				int cXe = cXs + 1;
				int cYs = (int)(cellPos.y - 0.5f);
				int cYe = cYs + 1;
				int cZs = (int)(cellPos.z - 0.5f);
				int cZe = cZs + 1;

				bool noconflicts = true;

				for (int iterCz = cZs; iterCz <= cZe; ++iterCz) {
					for (int iterCy = cYs; iterCy <= cYe; ++iterCy) {
						for (int iterCx = cXs; iterCx <= cXe; ++iterCx) {
							int cellId = poissonSpatialGridGetCellId(sampleGrid, iterCx, iterCy, iterCz);

							size_t sampleCount = sampleGrid->cells[cellId].samples.size();

							for (size_t iterSamples = 0; iterSamples < sampleCount; ++iterSamples) {
								// Compare candidate against sample;

								ldiPoissonSample* ps = &sampleGrid->cells[cellId].samples[iterSamples];

								float mag = glm::distance(ps->position, cand->position);

								//float checkDist = 0.02f * cand->color.r + 0.0060f;
								//float checkDist = 0.0075f;
								float checkDist = 0.0060f;

								if (mag <= checkDist) {
									noconflicts = false;
									break;
								}
							}

							if (!noconflicts) {
								break;
							}
						}
						if (!noconflicts) {
							break;
						}
					}
					if (!noconflicts) {
						break;
					}
				}

				if (noconflicts) {
					//cand->color = vec3(0.0f, 0.72f, 0.92f);
					//cand->color = vec3(0.8f, 0.0f, 0.56f);
					//cand->color = vec3(0.96f, 0.9f, 0.09f);
					//cand->color = vec3(0.0f, 0.0f, 0.0f);
					cand->color = rndCol;
					
					poissonSamples.push_back(*cand);

					ldiPoissonSample ps = {};
					ps.position = cand->position;
					ps.value = cand->color.r;

					sampleGrid->cells[poissonSpatialGridGetCellId(sampleGrid, cellPos.x, cellPos.y, cellPos.z)].samples.push_back(ps);

					ldiLaserViewSurfel viewSurfel = {};
					viewSurfel.id = 0;
					viewSurfel.angle = 0;

					vec4 clipSpace = machineTransform.projViewModelMat * vec4(ps.position, 1.0f);
					clipSpace.x /= clipSpace.w;
					clipSpace.y /= clipSpace.w;
					clipSpace.z /= clipSpace.w;

					clipSpace = clipSpace * 0.5f + 0.5f;

					viewSurfel.screenPos = vec2(clipSpace.x * 1024.0f, (1.0f - clipSpace.y) * 1024.0f);
					
					pathPos.surfels.push_back(viewSurfel);
				}
			}

			ModelInspector->laserViewPathPositions.push_back(pathPos);

			t0 = _getTime(AppContext) - t0;
			std::cout << "Poisson sampling: " << t0 * 1000.0f << " ms\n";
		}
	}

	ModelInspector->poissonSamplesRenderModel = gfxCreateSurfelRenderModel(AppContext, &poissonSamples);

	std::cout << "Total poisson samples: " << poissonSamples.size() << "\n";

	//----------------------------------------------------------------------------------------------------
	// AFTER ALL PASSES ONLY.
	//----------------------------------------------------------------------------------------------------
	/*double t0 = _getTime(AppContext);
	surfelsCreateDistribution(&ModelInspector->spatialGrid, &ModelInspector->surfels, &ModelInspector->pointDistrib, ModelInspector->surfelMask);
	t0 = _getTime(AppContext) - t0;
	std::cout << "Surfel distribution: " << t0 * 1000.0f << " ms Points: " << ModelInspector->pointDistrib.points.size() << "\n";*/

	AppContext->d3dDeviceContext->UpdateSubresource(ModelInspector->surfelMaskBuffer, 0, NULL, ModelInspector->surfelMask, 0, 0);

	return true;
}

float modelInspectorGetLaserViewPath(std::vector<int>* ViewPath, std::vector<ldiLaserViewPathPos>* PathPositions, int* NodeMask, int RootNode) {
	if (ViewPath) {
		ViewPath->clear();
	}

	int pathNodes = PathPositions->size();
	int currentNode = RootNode;
	float pathLength = 0.0f;
	memset(NodeMask, 0, sizeof(int) * pathNodes);

	while (true) {
		NodeMask[currentNode] = 1;

		if (ViewPath) {
			ViewPath->push_back(currentNode);
		}

		vec3 rootPos = (*PathPositions)[currentNode].surfacePos + (*PathPositions)[currentNode].surfaceNormal * 20.0f;

		float closest = 100000.0f;
		currentNode = -1;

		for (int i = 0; i < pathNodes; ++i) {
			if (NodeMask[i] != 0) {
				continue;
			}

			vec3 checkPos = (*PathPositions)[i].surfacePos + (*PathPositions)[i].surfaceNormal * 20.0f;

			float dist = glm::length(checkPos - rootPos);

			if (dist < closest) {
				closest = dist;
				currentNode = i;
			}
		}

		if (currentNode != -1) {
			pathLength += closest;
		} else {
			break;
		}
	}

	return pathLength;
}

void modelInspectorCalcLaserPath(ldiModelInspector* ModelInspector) {
	double t0 = _getTime(ModelInspector->appContext);

	ModelInspector->laserViewPath.clear();
	
	int pathNodes = ModelInspector->laserViewPathPositions.size();

	if (pathNodes == 0) {
		return;
	}

	int* nodeMask = new int[pathNodes];

	int bestRoot = 0;
	float bestPath = 100000.0f;

	for (int i = 0; i < pathNodes; ++i) {
		float pathLength = modelInspectorGetLaserViewPath(NULL, &ModelInspector->laserViewPathPositions, nodeMask, i);

		if (pathLength < bestPath) {
			bestPath = pathLength;
			bestRoot = i;
		}

		std::cout << "Start node: " << i << " Path length: " << pathLength << "\n";
	}

	float pathLength = modelInspectorGetLaserViewPath(&ModelInspector->laserViewPath, &ModelInspector->laserViewPathPositions, nodeMask, bestRoot);
	std::cout << "Best start node: " << bestRoot << " Path length: " << pathLength << "\n";

	delete[] nodeMask;

	t0 = _getTime(ModelInspector->appContext) - t0;
	std::cout << "Calc path: " << t0 * 1000.0f << " ms\n";
}

void modelInspectorRenderLaserView(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ldiLaserViewCoveragePrep prep = modelInspectorLaserViewCoveragePrep(AppContext, ModelInspector);
	ldiPointCloudVertex* vert = &ModelInspector->pointDistrib.points[ModelInspector->selectedSurfel];
	modelInspectorLaserViewCoverageRender(AppContext, ModelInspector, &prep, vert->position, vert->normal, 0, false);

	AppContext->d3dDeviceContext->CopyResource(ModelInspector->coverageAreaStagingBuffer, ModelInspector->coverageAreaBuffer);

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT result = AppContext->d3dDeviceContext->Map(ModelInspector->coverageAreaStagingBuffer, 0, D3D11_MAP_READ, 0, &ms);

	int* areaResults = (int*)ms.pData;
	ModelInspector->coverageValue = areaResults[0];
	//std::cout << areaResults[0] << "\n";

	AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageAreaStagingBuffer, 0);
}

int modelInspectorLoad(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->dergnModel = objLoadModel("../../assets/models/taryk.obj");
	//ModelInspector->dergnModel = objLoadModel("../../assets/models/dergn.obj");
	//ModelInspector->dergnModel = objLoadModel("../../assets/models/materialball.obj");

	float globalScale = 1.0f;
	vec3 globalTranslate(0.0f, 0.0f, 0.0f);
	
	// Scale model.
	/*for (int i = 0; i < ModelInspector->dergnModel.verts.size(); ++i) {
		ldiMeshVertex* vert = &ModelInspector->dergnModel.verts[i];
		vert->pos = vert->pos * globalScale + globalTranslate;
	}*/

	physicsCookMesh(AppContext->physics, &ModelInspector->dergnModel, &ModelInspector->cookedDergn);

	ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &ModelInspector->dergnModel);

	// Import PLY quad mesh.
	if (!plyLoadQuadMesh("../cache/output.ply", &ModelInspector->quadModel)) {
		return 1;
	}

	int quadCount = ModelInspector->quadModel.indices.size() / 4;

	double totalSideLengths = 0.0;
	int totalSideCount = 0;
	float maxSide = FLT_MIN;
	float minSide = FLT_MAX;

	// NOTE: Should already be correctly sized after quad bake.
	/*for (size_t i = 0; i < ModelInspector->quadModel.verts.size(); ++i) {
		vec3* vert = &ModelInspector->quadModel.verts[i];
		*vert = *vert * globalScale + globalTranslate;
	}*/

	// Side lengths.
	for (int i = 0; i < quadCount; ++i) {
		vec3 v0 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[i * 4 + 0]];
		vec3 v1 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[i * 4 + 1]];
		vec3 v2 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[i * 4 + 2]];
		vec3 v3 = ModelInspector->quadModel.verts[ModelInspector->quadModel.indices[i * 4 + 3]];

		float s0 = glm::length(v0 - v1);
		float s1 = glm::length(v1 - v2);
		float s2 = glm::length(v2 - v3);
		float s3 = glm::length(v3 - v0);

		maxSide = max(maxSide, s0);
		maxSide = max(maxSide, s1);
		maxSide = max(maxSide, s2);
		maxSide = max(maxSide, s3);

		minSide = min(minSide, s0);
		minSide = min(minSide, s1);
		minSide = min(minSide, s2);
		minSide = min(minSide, s3);

		totalSideLengths += s0 + s1 + s2 + s3;
		totalSideCount += 4;
	}

	totalSideLengths /= double(totalSideCount);

	std::cout << "Quad model - quads: " << quadCount << " sideAvg: " << (totalSideLengths * 10000.0) << " um \n";
	std::cout << "Min: " << (minSide * 10000.0) << " um Max: " << (maxSide * 10000.0) << " um \n";

	ModelInspector->dergnDebugModel = gfxCreateRenderQuadModelDebug(AppContext, &ModelInspector->quadModel);

	// TODO: Share verts and normals.
	ModelInspector->quadModelWhite = gfxCreateRenderQuadModelWhite(AppContext, &ModelInspector->quadModel);

	geoCreateSurfels(&ModelInspector->quadModel, &ModelInspector->surfels);
	geoCreateSurfelsHigh(&ModelInspector->quadModel, &ModelInspector->surfelsHigh);

	std::cout << "High surfels: " << ModelInspector->surfelsHigh.size() << "\n";

	double t0 = _getTime(AppContext);
	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/tarykTexture.png", &x, &y, &n);
	//uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/dergnTexture.png", &x, &y, &n);
	//uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/dergn_k.png", &x, &y, &n);
	//uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/materialballTextureGrid.png", &x, &y, &n);

	ModelInspector->baseTexture.width = x;
	ModelInspector->baseTexture.height = y;
	ModelInspector->baseTexture.data = imageRawPixels;

	t0 = _getTime(AppContext) - t0;
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ") " << t0 * 1000.0f << " ms\n";

	geoTransferColorToSurfels(ModelInspector->appContext, &ModelInspector->cookedDergn, &ModelInspector->dergnModel, &ModelInspector->baseTexture, &ModelInspector->surfelsHigh);
	
	//----------------------------------------------------------------------------------------------------
	// Create spatial structure for surfels.
	//----------------------------------------------------------------------------------------------------
	t0 = _getTime(AppContext) - t0;
	vec3 surfelsMin(10000, 10000, 10000);
	vec3 surfelsMax(-10000, -10000, -10000);

	for (size_t i = 0; i < ModelInspector->surfels.size(); ++i) {
		ldiSurfel* s = &ModelInspector->surfels[i];

		surfelsMin.x = min(surfelsMin.x, s->position.x);
		surfelsMin.y = min(surfelsMin.y, s->position.y);
		surfelsMin.z = min(surfelsMin.z, s->position.z);

		surfelsMax.x = max(surfelsMax.x, s->position.x);
		surfelsMax.y = max(surfelsMax.y, s->position.y);
		surfelsMax.z = max(surfelsMax.z, s->position.z);
	}

	ldiSpatialGrid spatialGrid{};
	spatialGridInit(&spatialGrid, surfelsMin, surfelsMax, 0.05f);

	for (size_t i = 0; i < ModelInspector->surfels.size(); ++i) {
		spatialGridPrepEntry(&spatialGrid, ModelInspector->surfels[i].position);
	}

	spatialGridCompile(&spatialGrid);

	for (size_t i = 0; i < ModelInspector->surfels.size(); ++i) {
		spatialGridAddEntry(&spatialGrid, ModelInspector->surfels[i].position, (int)i);
	}

	ModelInspector->spatialGrid = spatialGrid;

	t0 = _getTime(AppContext) - t0;
	std::cout << "Build spatial grid: " << t0 * 1000.0f << " ms\n";

	//----------------------------------------------------------------------------------------------------
	// Create spatial structure for poisson samples.
	//----------------------------------------------------------------------------------------------------
	poissonSpatialGridInit(&ModelInspector->poissonSpatialGrid, surfelsMin, surfelsMax, 0.05f);

	t0 = _getTime(AppContext);
	{
		const int threadCount = 20;
		int batchSize = ModelInspector->surfels.size() / threadCount;
		int batchRemainder = ModelInspector->surfels.size() - (batchSize * threadCount);
		std::thread workerThread[threadCount];

		for (int n = 0; n < 4; ++n) {
			for (int t = 0; t < threadCount; ++t) {
				ldiSmoothNormalsThreadContext tc{};
				tc.grid = &spatialGrid;
				tc.surfels = &ModelInspector->surfels;
				tc.startIdx = t * batchSize;
				tc.endIdx = (t + 1) * batchSize;

				if (t == threadCount - 1) {
					tc.endIdx += batchRemainder;
				}

				workerThread[t] = std::move(std::thread(surfelsSmoothNormalsThreadBatch, tc));
			}

			for (int t = 0; t < threadCount; ++t) {
				workerThread[t].join();
			}

			// Apply smoothed normal back to normal normal.
			for (int i = 0; i < ModelInspector->surfels.size(); ++i) {
				ldiSurfel* srcSurfel = &ModelInspector->surfels[i];
				srcSurfel->normal = srcSurfel->smoothedNormal;
			}
		}
	}
	t0 = _getTime(AppContext) - t0;
	std::cout << "Normal smoothing: " << t0 * 1000.0f << " ms\n";

	
	ModelInspector->surfelRenderModel = gfxCreateSurfelRenderModel(AppContext, &ModelInspector->surfels);
	ModelInspector->surfelHighRenderModel = gfxCreateSurfelRenderModel(AppContext, &ModelInspector->surfelsHigh);
	ModelInspector->coveragePointModel = gfxCreateCoveragePointRenderModel(AppContext, &ModelInspector->surfels, &ModelInspector->quadModel);
	
	ldiPointCloud pointCloud;
	if (!plyLoadPoints("../../assets/models/dergnScan.ply", &pointCloud)) {
		return 1;
	}

	ModelInspector->pointCloudRenderModel = gfxCreateRenderPointCloud(AppContext, &pointCloud);
	ModelInspector->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &ModelInspector->quadModel);

	{
		D3D11_SUBRESOURCE_DATA texData = {};
		texData.pSysMem = imageRawPixels;
		texData.SysMemPitch = x * 4;

		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = x;
		tex2dDesc.Height = y;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;

		// TODO: Move this to app context.
		if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &ModelInspector->baseImageTexture) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}
		
		//imageFree(imageRawPixels);
		if (AppContext->d3dDevice->CreateShaderResourceView(ModelInspector->baseImageTexture, NULL, &ModelInspector->shaderResourceViewTest) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // D3D11_FILTER_MIN_MAG_MIP_LINEAR
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &ModelInspector->texSamplerState);
	}

	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = ModelInspector->laserViewSize;
		tex2dDesc.Height = ModelInspector->laserViewSize;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;

		if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &ModelInspector->laserViewTex))) {
			std::cout << "Texture failed to create\n";
			return false;
		}

		if (AppContext->d3dDevice->CreateRenderTargetView(ModelInspector->laserViewTex, NULL, &ModelInspector->laserViewTexRtView) != S_OK) {
			std::cout << "CreateRenderTargetView failed\n";
			return 1;
		}

		if (AppContext->d3dDevice->CreateShaderResourceView(ModelInspector->laserViewTex, NULL, &ModelInspector->laserViewTexView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}

		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = ModelInspector->laserViewSize;
		depthStencilDesc.Height = ModelInspector->laserViewSize;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		AppContext->d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &ModelInspector->laserViewDepthStencil);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;// DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = 0;
		dsvDesc.Texture2D.MipSlice = 0;

		if (AppContext->d3dDevice->CreateDepthStencilView(ModelInspector->laserViewDepthStencil, &dsvDesc, &ModelInspector->laserViewDepthStencilView) != S_OK) {
			std::cout << "CreateDepthStencilView failed\n";
			return 1;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		if (AppContext->d3dDevice->CreateShaderResourceView(ModelInspector->laserViewDepthStencil, &srvDesc, &ModelInspector->laserViewDepthStencilSrv) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Coverage buffer compute shaders.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/calcPointScore.hlsl", "mainCs", &ModelInspector->calcPointScoreComputeShader)) {
		return 1;
	}

	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/accumulatePointScore.hlsl", "mainCs", &ModelInspector->accumulatePointScoreComputeShader)) {
		return 1;
	}

	int numSurfels = (int)ModelInspector->surfels.size();
	std::cout << "Create coverage buffer: " << numSurfels << "\n";

	ModelInspector->surfelMask = new int[numSurfels];
	memset(ModelInspector->surfelMask, 0, sizeof(int) * numSurfels);
	
	/*if (!gfxCreateStructuredBuffer(AppContext, 1, numSurfels, ModelInspector->surfelMask, &ModelInspector->surfelMaskBuffer)) {
		std::cout << "Could not gfxCreateStructuredBuffer\n";
		return 1;
	}*/

	{
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.ByteWidth = sizeof(int) * numSurfels;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(int);

		if (AppContext->d3dDevice->CreateBuffer(&desc, NULL, &ModelInspector->surfelMaskBuffer) != S_OK) {
			std::cout << "Could not gfxCreateStructuredBuffer\n";
			return 1;
		}
	}

	if (!gfxCreateBufferShaderResourceView(AppContext, ModelInspector->surfelMaskBuffer, &ModelInspector->surfelMaskBufferSrv)) {
		std::cout << "Could not gfxCreateBufferShaderResourceView\n";
		return 1;
	}

	if (!gfxCreateStructuredBuffer(AppContext, 4, numSurfels, NULL, &ModelInspector->coverageBuffer)) {
		return 1;
	}

	if (!gfxCreateBufferUav(AppContext, ModelInspector->coverageBuffer, &ModelInspector->coverageBufferUav)) {
		return 1;
	}

	if (!gfxCreateBufferShaderResourceView(AppContext, ModelInspector->coverageBuffer, &ModelInspector->coverageBufferSrv)) {
		return 1;
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = 4 * numSurfels;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		HRESULT r;
		if ((r = AppContext->d3dDevice->CreateBuffer(&desc, NULL, &ModelInspector->coverageStagingBuffer)) != S_OK) {
			return 1;
		}
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = 4 * 5000;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		HRESULT r;
		if ((r = AppContext->d3dDevice->CreateBuffer(&desc, NULL, &ModelInspector->coverageAreaStagingBuffer)) != S_OK) {
			return 1;
		}
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(ldiCoverageConstantBuffer);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		HRESULT r;
		if ((r = AppContext->d3dDevice->CreateBuffer(&desc, NULL, &ModelInspector->coverageConstantBuffer)) != S_OK) {
			std::cout << "Could not CreateBuffer\n";
			return 1;
		}
	}

	if (!gfxCreateStructuredBuffer(AppContext, 4, 5000, NULL, &ModelInspector->coverageAreaBuffer)) {
		return 1;
	}
	
	if (!gfxCreateBufferUav(AppContext, ModelInspector->coverageAreaBuffer, &ModelInspector->coverageAreaBufferUav)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Coverage point shaders.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC coveragePointLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32_UINT,			0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	16,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/coveragePoint.hlsl", "showCoverageVs", &ModelInspector->coveragePointVertexShader, coveragePointLayout, 3, &ModelInspector->coveragePointInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/coveragePoint.hlsl", "showCoveragePs", &ModelInspector->coveragePointPixelShader)) {
		return 1;
	}

	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/coveragePoint.hlsl", "writeCoverageVs", &ModelInspector->coveragePointWriteVertexShader)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/coveragePoint.hlsl", "writeCoveragePs", &ModelInspector->coveragePointWritePixelShader)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/coveragePoint.hlsl", "writeNoCoveragePs", &ModelInspector->coveragePointWriteNoCoveragePixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Run full coverage test.
	//----------------------------------------------------------------------------------------------------
	/*if (!modelInspectorCalculateLaserViewCoverage(AppContext, ModelInspector)) {
		return 1;
	}*/

	/*{
		t0 = _getTime(AppContext);
		surfelsCreateDistribution(&ModelInspector->spatialGrid, &ModelInspector->surfels, &ModelInspector->pointDistrib);
		t0 = _getTime(AppContext) - t0;
		std::cout << "Surfel distribution: " << t0 * 1000.0f << " ms Points: " << ModelInspector->pointDistrib.points.size() << "\n";
	}*/

	t0 = _getTime(AppContext);

	if (!modelInspectorCalculateLaserViewPath(AppContext, ModelInspector)) {
		return 1;
	}

	t0 = _getTime(AppContext) - t0;
	std::cout << "View path: " << t0 * 1000.0f << " ms\n";

	ModelInspector->pointDistribCloud = gfxCreateRenderPointCloud(AppContext, &ModelInspector->pointDistrib);

	modelInspectorCalcLaserPath(ModelInspector);

	return 0;
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

void modelInspectorRender(ldiModelInspector* ModelInspector, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = ModelInspector->appContext;

	if (Width != ModelInspector->mainViewWidth || Height != ModelInspector->mainViewHeight) {
		ModelInspector->mainViewWidth = Width;
		ModelInspector->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &ModelInspector->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	updateCamera3dBasicFps(&ModelInspector->camera, (float)ModelInspector->mainViewWidth, (float)ModelInspector->mainViewHeight);

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);

	// Axis gizmo.
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	if (ModelInspector->showScaleHelper) {
		pushDebugLine(appContext, vec3(3, 0, 0), vec3(3, 5, 0), vec3(0.25, 0, 0.25));
		pushDebugLine(appContext, vec3(3, 5, 0), vec3(3, 10, 0), vec3(0.5, 0, 0.5));
		pushDebugLine(appContext, vec3(3, 10, 0), vec3(3, 12, 0), vec3(0.75, 0, 0.75));
		pushDebugLine(appContext, vec3(3, 12, 0), vec3(3, 15, 0), vec3(1, 0, 1));
	
		pushDebugBox(appContext, vec3(2.5, 10, 0), vec3(0.05, 0.05, 0.05), vec3(1, 0, 1));
		pushDebugBoxSolid(appContext, vec3(1, 10, 0), vec3(0.005, 0.005, 0.005), vec3(1, 0, 1));

		pushDebugBox(appContext, vec3(2.5, 10, -1.0), vec3(0.3, 0.3, 0.3), vec3(1, 0, 1));
	
		displayTextAtPoint(&ModelInspector->camera, vec3(3, 0, 0), "0 mm", vec4(1.0f, 1.0f, 1.0f, 1.0f), TextBuffer);
		displayTextAtPoint(&ModelInspector->camera, vec3(3, 5, 0), "50 mm", vec4(1.0f, 1.0f, 1.0f, 1.0f), TextBuffer);
		displayTextAtPoint(&ModelInspector->camera, vec3(3, 10, 0), "100 mm", vec4(1.0f, 1.0f, 1.0f, 1.0f), TextBuffer);
		displayTextAtPoint(&ModelInspector->camera, vec3(3, 12, 0), "120 mm", vec4(1.0f, 1.0f, 1.0f, 1.0f), TextBuffer);
		displayTextAtPoint(&ModelInspector->camera, vec3(3, 15, 0), "150 mm", vec4(1.0f, 1.0f, 1.0f, 1.0f), TextBuffer);
	}
	
	// Grid.
	int gridCount = 60;
	float gridCellWidth = 1.0f;
	vec3 gridColor = ModelInspector->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	if (ModelInspector->showBounds) {
		pushDebugSphere(appContext, vec3(0, 7.5f, 0), 7.5f, vec3(0.8f, 0.8f, 0.0f), 64);
		pushDebugSphere(appContext, vec3(0, 7.5f, 0), 7.5f + 5.0f, vec3(0.5f, 0.8f, 0.0f), 64);
		pushDebugSphere(appContext, vec3(0, 7.5f, 0), 27.5f, vec3(0.0f, 0.8f, 0.0f), 64);
	}

	//----------------------------------------------------------------------------------------------------
	// Spatial accel structure.
	//----------------------------------------------------------------------------------------------------
	if (ModelInspector->showSpatialCells) {
		spatialGridRenderOccupied(appContext, &ModelInspector->spatialGrid);
	}

	if (ModelInspector->showSpatialBounds) {
		spatialGridRenderDebug(appContext, &ModelInspector->spatialGrid, true, false);
	}

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &ModelInspector->renderViewBuffers.mainViewRtView, ModelInspector->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = ModelInspector->mainViewWidth;
	viewport.Height = ModelInspector->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(ModelInspector->renderViewBuffers.mainViewRtView, (float*)&ModelInspector->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(ModelInspector->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (ModelInspector->showSampleSites) {
		for (size_t i = 0; i < ModelInspector->pointDistrib.points.size(); ++i) {
			ldiPointCloudVertex* v = &ModelInspector->pointDistrib.points[i];
			pushDebugLine(appContext, v->position, v->position + v->normal * 0.1f, vec3(1.0f, 0, 0));
		}
	
		ldiPointCloudVertex* v = &ModelInspector->pointDistrib.points[ModelInspector->selectedSurfel];
		pushDebugLine(appContext, v->position, v->position + v->normal * 20.0f, vec3(1.0f, 0, 0));
	
		for (size_t i = 0; i < ModelInspector->laserViewPathPositions.size(); ++i) {
			ldiLaserViewPathPos* p = &ModelInspector->laserViewPathPositions[i];
			vec3 pos = p->surfacePos + p->surfaceNormal * 20.0f;
			pushDebugBox(appContext, pos, vec3(0.1f, 0.1f, 0.1f), vec3(1, 0, 1));
			pushDebugLine(appContext, pos, pos + -p->surfaceNormal, vec3(1.0f, 0, 0));
		}
	
		for (size_t i = 1; i < ModelInspector->laserViewPath.size(); ++i) {
			ldiLaserViewPathPos* a = &ModelInspector->laserViewPathPositions[ModelInspector->laserViewPath[i - 1]];
			ldiLaserViewPathPos* b = &ModelInspector->laserViewPathPositions[ModelInspector->laserViewPath[i - 0]];

			vec3 aPos = a->surfacePos + a->surfaceNormal * 20.0f;
			vec3 bPos = b->surfacePos + b->surfaceNormal * 20.0f;

			pushDebugLine(appContext, aPos, bPos, vec3(0.0f, 1.0f, 1.0f));
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
	}

	renderDebugPrimitives(appContext);

	//----------------------------------------------------------------------------------------------------
	// Render models.
	//----------------------------------------------------------------------------------------------------
	if (ModelInspector->showSampleSites) {
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->screenSize = vec4(ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, 0, 0);
			constantBuffer->mvp = ModelInspector->camera.projViewMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			constantBuffer->view = ModelInspector->camera.viewMat;
			constantBuffer->proj = ModelInspector->camera.projMat;
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
		}
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->pointcloudConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiPointCloudConstantBuffer* constantBuffer = (ldiPointCloudConstantBuffer*)ms.pData;
			//constantBuffer->size = vec4(0.1f, 0.1f, 0, 0);
			//constantBuffer->size = vec4(16, 16, 1, 0);
			constantBuffer->size = vec4(ModelInspector->pointWorldSize, ModelInspector->pointScreenSize, ModelInspector->pointScreenSpaceBlend, 0);
			appContext->d3dDeviceContext->Unmap(appContext->pointcloudConstantBuffer, 0);
		}

		gfxRenderPointCloud(appContext, &ModelInspector->pointDistribCloud);
	}

	if (ModelInspector->primaryModelShowShaded) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderModel(appContext, &ModelInspector->dergnRenderModel, false, ModelInspector->shaderResourceViewTest, ModelInspector->texSamplerState);
	}

	if (ModelInspector->primaryModelShowWireframe) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderModel(appContext, &ModelInspector->dergnRenderModel, true);
	}

	if (ModelInspector->showPointCloud) {
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->screenSize = vec4(ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, 0, 0);
			constantBuffer->mvp = ModelInspector->camera.projViewMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			constantBuffer->view = ModelInspector->camera.viewMat;
			constantBuffer->proj = ModelInspector->camera.projMat;
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
		}
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->pointcloudConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiPointCloudConstantBuffer* constantBuffer = (ldiPointCloudConstantBuffer*)ms.pData;
			//constantBuffer->size = vec4(0.1f, 0.1f, 0, 0);
			//constantBuffer->size = vec4(16, 16, 1, 0);
			constantBuffer->size = vec4(ModelInspector->pointWorldSize, ModelInspector->pointScreenSize, ModelInspector->pointScreenSpaceBlend, 0);
			appContext->d3dDeviceContext->Unmap(appContext->pointcloudConstantBuffer, 0);
		}

		gfxRenderPointCloud(appContext, &ModelInspector->pointCloudRenderModel);
	}

	if (ModelInspector->quadMeshShowDebug) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderDebugModel(appContext, &ModelInspector->dergnDebugModel);
	}

	if (ModelInspector->quadMeshShowWireframe) {	
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderWireModel(appContext, &ModelInspector->quadMeshWire);
	}

	if (ModelInspector->showQuadMeshWhite) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = ModelInspector->modelCanvasColor;
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderDebugModel(appContext, &ModelInspector->quadModelWhite);
	}

	if (ModelInspector->showSurfels) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(ModelInspector->camera.position, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		//gfxRenderSurfelModel(appContext, &ModelInspector->surfelRenderModel, appContext->dotShaderResourceView, appContext->dotSamplerState);
		gfxRenderSurfelModel(appContext, &ModelInspector->surfelHighRenderModel, appContext->dotShaderResourceView, appContext->dotSamplerState);
	}

	if (ModelInspector->showPoissonSamples) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(ModelInspector->camera.position, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderSurfelModel(appContext, &ModelInspector->poissonSamplesRenderModel, appContext->dotShaderResourceView, appContext->dotSamplerState);
	}

	// Area coverage result.
	if (false) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = ModelInspector->camera.projViewMat;
		constantBuffer->color = vec4(ModelInspector->camera.position, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		UINT lgStride = sizeof(ldiCoveragePointVertex);
		UINT lgOffset = 0;

		appContext->d3dDeviceContext->IASetInputLayout(ModelInspector->coveragePointInputLayout);
		appContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &ModelInspector->coveragePointModel.vertexBuffer, &lgStride, &lgOffset);
		appContext->d3dDeviceContext->IASetIndexBuffer(ModelInspector->coveragePointModel.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		appContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		appContext->d3dDeviceContext->VSSetShader(ModelInspector->coveragePointVertexShader, 0, 0);
		appContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);

		ID3D11ShaderResourceView* vsSrvs[] = {
			ModelInspector->coverageBufferSrv,
			ModelInspector->surfelMaskBufferSrv
		};
		appContext->d3dDeviceContext->VSSetShaderResources(0, 2, vsSrvs);

		appContext->d3dDeviceContext->PSSetShader(ModelInspector->coveragePointPixelShader, 0, 0);
		appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);

		appContext->d3dDeviceContext->OMSetBlendState(appContext->defaultBlendState, NULL, 0xffffffff);
		appContext->d3dDeviceContext->RSSetState(appContext->defaultRasterizerState);

		appContext->d3dDeviceContext->OMSetDepthStencilState(appContext->defaultDepthStencilState, 0);

		/*appContext->d3dDeviceContext->PSSetShaderResources(0, 1, &ResourceView);
		appContext->d3dDeviceContext->PSSetSamplers(0, 1, &Sampler);*/
		
		appContext->d3dDeviceContext->DrawIndexed(ModelInspector->coveragePointModel.indexCount, 0, 0);
	}
}

void _guiPointFilterCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ldiApp* appContext = (ldiApp*)cmd->UserCallbackData;
	appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultPointSamplerState);
}

void modelInspectorShowUi(ldiModelInspector* tool) {
	ImGui::Begin("Model inspector controls");
	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Background", (float*)&tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid", (float*)&tool->gridColor);
		ImGui::SliderFloat("Camera speed", &tool->cameraSpeed, 0.0f, 4.0f);
		ImGui::SliderFloat("Camera FOV", &tool->camera.fov, 1.0f, 180.0f);
		
		ImGui::Separator();
		ImGui::Text("Primary model");
		ImGui::Checkbox("Shaded", &tool->primaryModelShowShaded);
		ImGui::Checkbox("Wireframe", &tool->primaryModelShowWireframe);

		ImGui::Separator();
		ImGui::Text("Quad model");
		ImGui::Checkbox("Canvas model", &tool->showQuadMeshWhite);
		ImGui::ColorEdit3("Canvas", (float*)&tool->modelCanvasColor);
		ImGui::Checkbox("Area debug", &tool->quadMeshShowDebug);
		ImGui::Checkbox("Quad wireframe", &tool->quadMeshShowWireframe);
		ImGui::Checkbox("Surfels", &tool->showSurfels);
		ImGui::Checkbox("Spatial bounds", &tool->showSpatialBounds);
		ImGui::Checkbox("Spatial occupied cells", &tool->showSpatialCells);

		ImGui::Checkbox("Sample sites", &tool->showSampleSites);

		ImGui::Checkbox("Bounds", &tool->showBounds);

		ImGui::Checkbox("Scale helpers", &tool->showScaleHelper);

		ImGui::Checkbox("Poisson samples", &tool->showPoissonSamples);
		
		
		ImGui::Separator();
		ImGui::Text("Point cloud");
		ImGui::Checkbox("Show", &tool->showPointCloud);
		ImGui::SliderFloat("World size", &tool->pointWorldSize, 0.0f, 1.0f);
		ImGui::SliderFloat("Screen size", &tool->pointScreenSize, 0.0f, 32.0f);
		ImGui::SliderFloat("Screen blend", &tool->pointScreenSpaceBlend, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Laser view")) {
		ImGui::DragFloat3("Cam position", (float*)&tool->laserViewPos, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat2("Cam rotation", (float*)&tool->laserViewRot, 0.1f, -360.0f, 360.0f);
		ImGui::SliderInt("Surfel", &tool->selectedSurfel, 0, tool->pointDistrib.points.size() - 1);
		ImGui::SliderInt("Laser view", &tool->selectedLaserView, 0, tool->laserViewPathPositions.size() - 1);
		ImGui::Checkbox("Laser view show background", &tool->laserViewShowBackground);
		
		ImGui::Text("Coverage area: %d", tool->coverageValue);
		ImGui::Image(tool->laserViewTexView, ImVec2(512, 512));
	}
		
	if (ImGui::CollapsingHeader("Processing")) {
		if (ImGui::Button("Process model")) {
			modelInspectorVoxelize(tool);
			modelInspectorCreateQuadMesh(tool);
		}

		if (ImGui::Button("Voxelize")) {
			modelInspectorVoxelize(tool);
		}

		if (ImGui::Button("Quadralize")) {
			modelInspectorCreateQuadMesh(tool);
		}
	}

	ImGui::End();

	modelInspectorRenderLaserView(tool->appContext, tool);

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Laser view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImGui::Image(tool->laserViewTexView, ImVec2(tool->laserViewSize, tool->laserViewSize));
	}
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Surfel view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// This will catch our interactions.
		ImGui::InvisibleButton("__imgInspecViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

		static float imgScale = 1.0f;
		static vec2 imgOffset(0.0f, 0.0f);

		// Convert canvas pos to world pos.
		vec2 worldPos;
		worldPos.x = mouseCanvasPos.x;
		worldPos.y = mouseCanvasPos.y;
		worldPos *= (1.0 / imgScale);
		worldPos -= imgOffset;

		if (isHovered) {
			float wheel = ImGui::GetIO().MouseWheel;

			if (wheel) {
				imgScale += wheel * 0.2f * imgScale;

				vec2 newWorldPos;
				newWorldPos.x = mouseCanvasPos.x;
				newWorldPos.y = mouseCanvasPos.y;
				newWorldPos *= (1.0 / imgScale);
				newWorldPos -= imgOffset;

				vec2 deltaWorldPos = newWorldPos - worldPos;

				imgOffset.x += deltaWorldPos.x;
				imgOffset.y += deltaWorldPos.y;
			}
		}

		if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= (1.0 / imgScale);

			imgOffset += vec2(mouseDelta.x, mouseDelta.y);
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float areaX = screenStartPos.x + (imgOffset.x + 0) * imgScale;
		float areaY = screenStartPos.y + (imgOffset.y + 0) * imgScale;

		ImVec2 imgMin;
		imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
		imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

		ImVec2 imgMax;
		imgMax.x = imgMin.x + 1024 * imgScale;
		imgMax.y = imgMin.y + 1024 * imgScale;

		int laserViewSurfelCount = 0;

		if (tool->selectedLaserView != -1) {
			if (tool->laserViewShowBackground) {
				draw_list->AddCallback(_guiPointFilterCallback, tool->appContext);
				draw_list->AddImage(tool->laserViewTexView, imgMin, imgMax);
				draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);
			} else {
				draw_list->AddRectFilled(imgMin, imgMax, ImColor(0.5f, 0.5f, 0.5f, 1.0f));
			}

			draw_list->AddRect(imgMin, imgMax, ImColor(1.0f, 0.0f, 0.0f, 1.0f));

			ldiLaserViewPathPos* laserView = &tool->laserViewPathPositions[tool->selectedLaserView];

			laserViewSurfelCount = laserView->surfels.size();

			float umToPixel = 1.0f / 40.96f;
			float surfelSize = 75 * umToPixel;
			float surfelHalfSize = surfelSize * 0.5f;

			for (size_t i = 0; i < laserView->surfels.size(); ++i) {
				vec2 surfelPos = laserView->surfels[i].screenPos;
				float angle = laserView->surfels[i].angle;

				ImVec2 rMin;
				rMin.x = screenStartPos.x + (imgOffset.x + surfelPos.x - surfelHalfSize) * imgScale;
				rMin.y = screenStartPos.y + (imgOffset.y + surfelPos.y - surfelHalfSize) * imgScale;

				ImVec2 rMax;
				rMax.x = rMin.x + (surfelSize * imgScale);
				rMax.y = rMin.y + (surfelSize * imgScale);

				ImVec2 center;
				center.x = screenStartPos.x + (imgOffset.x + surfelPos.x) * imgScale;
				center.y = screenStartPos.y + (imgOffset.y + surfelPos.y) * imgScale;

				ImColor col(255, 255, 255);

				//draw_list->AddCircle(center, surfelHalfSize* imgScale, ImColor((int)(62 * angle), (int)(101 * angle), (int)(156 * angle)), 8);

				if (angle >= 0.9) {
					// 30
					col = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
				}
				else if (angle >= 0.8) {
					// 45
					col = ImColor(0.0f, 0.75f, 0.0f, 1.0f);
				}
				else if (angle >= 0.75) {
					// 60
					col = ImColor(0.0f, 0.5f, 0.0f, 1.0f);
				}
				else if (angle >= 0.7) {
					// 70
					col = ImColor(1.0f, 0.5f, 0.0f, 1.0f);
				}
			
				draw_list->AddCircle(center, surfelHalfSize* imgScale, col, 16);
				//draw_list->AddRectFilled(rMin, rMax, col);
			}
		}

		if (isHovered) {
			vec2 pixelPos;
			pixelPos.x = (int)(worldPos.x * 4.0f) / 4.0f;
			pixelPos.y = (int)(worldPos.y * 4.0f) / 4.0f;

			ImVec2 rMin;
			rMin.x = screenStartPos.x + (imgOffset.x + pixelPos.x) * imgScale;
			rMin.y = screenStartPos.y + (imgOffset.y + pixelPos.y) * imgScale;

			ImVec2 rMax = rMin;
			rMax.x += 0.25f * imgScale;
			rMax.y += 0.25f * imgScale;

			ImVec2 center;
			center.x = screenStartPos.x + (imgOffset.x + pixelPos.x + 0.125f) * imgScale;
			center.y = screenStartPos.y + (imgOffset.y + pixelPos.y + 0.125f) * imgScale;

			float umToPixel = 1.0f / 40.96f;
			float surfelSize = 75 * umToPixel;
			float surfelHalfSize = surfelSize * 0.5f;

			draw_list->AddRect(rMin, rMax, ImColor(255, 0, 255));
			draw_list->AddCircle(center, surfelHalfSize * imgScale, ImColor(255, 0, 255), 24);
		}

		// Viewport overlay.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_surfelViewOverlay", ImVec2(200, 70), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("%.3f %.3f - %.3f", imgOffset.x, imgOffset.y, imgScale);
			ImGui::Text("%.3f %.3f", worldPos.x, worldPos.y);
			ImGui::Text("Surfels: %d", laserViewSurfelCount);

			ImGui::EndChild();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();


	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Model inspector", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// NOTE: ImDrawList API uses screen coordinates!
		/*ImVec2 tempStartPos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(tempStartPos, ImVec2(tempStartPos.x + 500, tempStartPos.y + 500), IM_COL32(200, 200, 200, 255));*/

		// This will catch our interactions.
		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held
		//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
		//const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

		{
			vec3 camMove(0, 0, 0);
			ldiCamera* camera = &tool->camera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			if (isActive && (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				if (ImGui::IsKeyDown(ImGuiKey_W)) {
					camMove += vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_S)) {
					camMove -= vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_A)) {
					camMove -= vec3(vec4(vec3Right, 0.0f) * viewRotMat);
				}

				if (ImGui::IsKeyDown(ImGuiKey_D)) {
					camMove += vec3(vec4(vec3Right, 0.0f) * viewRotMat);
				}
			}

			if (glm::length(camMove) > 0.0f) {
				camMove = glm::normalize(camMove);
				float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime * tool->cameraSpeed;
				camera->position += camMove * cameraSpeed;
			}
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.15f;
			tool->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.025f;

			ldiCamera* camera = &tool->camera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
			camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
		}

		ImGui::SetCursorPos(startPos);
		std::vector<ldiTextInfo> textBuffer;
		modelInspectorRender(tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(tool->renderViewBuffers.mainViewResourceView, viewSize);

		// NOTE: ImDrawList API uses screen coordinates!
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int i = 0; i < textBuffer.size(); ++i) {
			ldiTextInfo* info = &textBuffer[i];
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), ImColor(info->color.r, info->color.g, info->color.b, info->color.a), info->text.c_str());
		}

		// Viewport overlay widgets.
		{
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 35), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("%.3f %.3f %.3f", tool->camera.position.x, tool->camera.position.y, tool->camera.position.z);

			ImGui::EndChild();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}