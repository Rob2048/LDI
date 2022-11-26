#pragma once

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

struct ldiSpatialGrid {
	vec3 min;
	vec3 max;
	vec3 origin;
	vec3 size;
	float cellSize;
	int countX;
	int countY;
	int countZ;
	int countTotal;
	int* index;
	int* data;
};

struct ldiProbabilityGrid {
	vec3 min;
	vec3 max;
	vec3 origin;
	vec3 size;
	int totalCells;
	float cellSize;
	int cellCount;
	float* data;
};

void spatialGridDestroy(ldiSpatialGrid* Grid) {
	if (Grid->index) {
		delete[] Grid->index;
	}

	if (Grid->data) {
		delete[] Grid->data;
	}

	memset(Grid, 0, sizeof(ldiSpatialGrid));
}

void spatialGridInit(ldiSpatialGrid* Grid, vec3 MinBounds, vec3 MaxBounds, float CellSize) {
	spatialGridDestroy(Grid);

	Grid->cellSize = CellSize;

	Grid->origin = MinBounds + (MaxBounds - MinBounds) * 0.5f;
	Grid->size = MaxBounds - MinBounds;

	Grid->countX = ceilf(Grid->size.x / CellSize + 0.5f) + 2;
	Grid->countY = ceilf(Grid->size.y / CellSize + 0.5f) + 2;
	Grid->countZ = ceilf(Grid->size.z / CellSize + 0.5f) + 2;
	Grid->countTotal = Grid->countX * Grid->countY * Grid->countZ;

	std::cout << "Spatial cell counts: " << Grid->countX << ", " << Grid->countY << ", " << Grid->countZ << " (" << Grid->countTotal << ")\n";

	Grid->size = vec3(Grid->countX * CellSize, Grid->countY * CellSize, Grid->countZ * CellSize);

	Grid->min = Grid->origin - Grid->size * 0.5f;
	Grid->max = Grid->origin + Grid->size * 0.5f;

	assert(Grid->countTotal > 0);
	Grid->index = new int[Grid->countTotal];
	memset(Grid->index, 0, sizeof(int) * Grid->countTotal);

	std::cout << "Spatial grid cells: " << ((Grid->countTotal * 4) / 1024.0 / 1024.0) << " MB\n";
}

vec3 spatialGridGetCellFromWorldPosition(ldiSpatialGrid* Grid, vec3 Position) {
	vec3 result;

	vec3 local = Position - Grid->min;

	result = local / Grid->cellSize;

	return result;
}

inline int spatialGridGetCellId(ldiSpatialGrid* Grid, int CellX, int CellY, int CellZ) {
	return CellX + CellY * Grid->countX + CellZ * Grid->countX * Grid->countY;
}

int spatialGridGetCellIdFromWorldPosition(ldiSpatialGrid* Grid, vec3 Position) {
	vec3 local = Position - Grid->min;

	int cellX = (int)(local.x / Grid->cellSize);
	int cellY = (int)(local.y / Grid->cellSize);
	int cellZ = (int)(local.z / Grid->cellSize);

	return spatialGridGetCellId(Grid, cellX, cellY, cellZ);
}

void spatialGridPrepEntry(ldiSpatialGrid* Grid, vec3 Position) {
	int cellId = spatialGridGetCellIdFromWorldPosition(Grid, Position);
	assert(cellId >= 0 && cellId < Grid->countTotal);

	Grid->index[cellId]++;
}

void spatialGridCompile(ldiSpatialGrid* Grid) {
	int totalEntries = 0;
	int totalOccupedCells = 0;

	for (int i = 0; i < Grid->countTotal; ++i) {
		if (Grid->index[i] > 0) {
			totalOccupedCells++;
			totalEntries += Grid->index[i];
		}
	}

	Grid->data = new int[totalOccupedCells + totalEntries];

	std::cout << "Spatial grid data: " << ((totalOccupedCells + totalEntries * 4) / 1024.0 / 1024.0) << " MB\n";

	int currentOffset = 0;

	for (int i = 0; i < Grid->countTotal; ++i) {
		int entryCount = Grid->index[i];

		if (entryCount == 0) {
			Grid->index[i] = -1;
		} else {
			Grid->index[i] = currentOffset;
			Grid->data[currentOffset] = 0;
			currentOffset += 1 + entryCount;
		}
	}
}

void spatialGridAddEntry(ldiSpatialGrid* Grid, vec3 Position, int Value) {
	int cellId = spatialGridGetCellIdFromWorldPosition(Grid, Position);
	assert(cellId >= 0 && cellId < Grid->countTotal);

	int dataOffset = Grid->index[cellId];
	
	int cellCount = Grid->data[dataOffset];
	Grid->data[dataOffset + 1 + cellCount] = Value;
	Grid->data[dataOffset] += 1;
}

void spatialGridRenderOccupied(ldiApp* AppContext, ldiSpatialGrid* Grid) {
	for (int i = 0; i < Grid->countTotal; ++i) {
		int dataOffset = Grid->index[i];
		
		if (dataOffset == -1) {
			continue;
		}

		int cellX = i % Grid->countX;
		int cellY = (i % (Grid->countX * Grid->countY)) / Grid->countX;
		int cellZ = i / (Grid->countX * Grid->countY);

		vec3 localSpace = vec3(cellX, cellY, cellZ) * Grid->cellSize;
		vec3 worldSpace = localSpace + Grid->min;
		vec3 max = worldSpace + Grid->cellSize;

		pushDebugBoxMinMax(AppContext, worldSpace, max, vec3(1, 0, 1));
	}
}

void spatialGridRenderDebug(ldiApp* AppContext, ldiSpatialGrid* Grid, bool Bounds, bool CellView) {
	if (Bounds) {
		pushDebugBox(AppContext, Grid->origin, Grid->size, vec3(1, 0, 1));
	}

	if (CellView) {
		vec3 color = vec3(0, 0, 1); 
		
		// Along X axis.
		for (int iY = 0; iY < Grid->countY; ++iY) {
			for (int iZ = 0; iZ < Grid->countZ; ++iZ) {
				pushDebugLine(AppContext, vec3(Grid->min.x, Grid->min.y + iY * Grid->cellSize, Grid->min.z + iZ * Grid->cellSize), vec3(Grid->max.x, Grid->min.y + iY * Grid->cellSize, Grid->min.z + iZ * Grid->cellSize), color);
			}
		}
	}
}

struct ldiSpatialCellResult {
	int count;
	int* data;
};

ldiSpatialCellResult spatialGridGetCell(ldiSpatialGrid* Grid, int CellX, int CellY, int CellZ) {
	ldiSpatialCellResult result{};

	int cellId = spatialGridGetCellId(Grid, CellX, CellY, CellZ);

	int dataOffset = Grid->index[cellId];

	if (dataOffset != -1) {
		result.count = Grid->data[dataOffset];
		result.data = &Grid->data[dataOffset + 1];
	}

	return result;
}

struct ldiLaserViewPathPos {
	vec3 worldPos;
	vec3 worldDir;
	
	vec3 surfacePos;
	vec3 surfaceNormal;
	mat4 modelToView;
	// Machine coords

	// Surfels
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
	bool						quadMeshShowDebug = true;
	bool						showSurfels = true;
	bool						showSpatialCells = false;
	bool						showSpatialBounds = true;
	bool						showSampleSites = false;

	float						pointWorldSize = 0.01f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	float						cameraSpeed = 0.5f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	ldiModel					dergnModel;
	ldiRenderModel				dergnRenderModel;
	ldiRenderModel				dergnDebugModel;
	
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

	ldiRenderPointCloud			pointCloudRenderModel;

	ldiPhysicsMesh				cookedDergn;

	std::vector<ldiSurfel>		surfels;
	ldiImage					baseTexture;

	ldiSpatialGrid				spatialGrid;

	ldiPointCloud				pointDistrib;
	ldiRenderPointCloud			pointDistribCloud;
	vec3						laserViewRot;
	vec3						laserViewPos;

	int							selectedSurfel = 0;

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

	std::vector<ldiLaserViewPathPos> laserViewPath;
};

void geoSmoothSurfels(std::vector<ldiSurfel>* Surfels) {
	std::cout << "Run smooth\n";
	for (size_t i = 0; i < Surfels->size(); ++i) {
		(*Surfels)[i].normal = vec3(0, 1, 0);
	}
}

std::vector<ldiSurfel> geoCreateSurfels(ldiQuadModel* Model) {
	int quadCount = Model->indices.size() / 4;

	std::vector<ldiSurfel> result;
	result.reserve(quadCount);

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
		surfel.position = center + normal * normalAdjust;
		surfel.normal = normal;
		surfel.color = vec3(0, 0, 0);
		surfel.scale = 1.0f;

		result.push_back(surfel);
	}

	return result;
}

void geoTransferColorToSurfels(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiImage* Image, std::vector<ldiSurfel>* Surfels) {
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
			int pixY = (Image->height - 1) - (uv.y * Image->height);

			uint8_t r = Image->data[(pixX + pixY * Image->width) * 4 + 0];
			uint8_t g = Image->data[(pixX + pixY * Image->width) * 4 + 1];
			uint8_t b = Image->data[(pixX + pixY * Image->width) * 4 + 2];

			s->color = vec3(r / 255.0, g / 255.0, b / 255.0);
		} else {
			s->color = vec3(1, 0, 0);
		}
	}
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

	char args[] = "PolyMender-clean.exe ../../bin/cache/source.stl 10 0.9 ../../bin/cache/voxel.ply";

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
	char args[] = "\"Instant Meshes.exe\" ../../bin/cache/voxel.ply -o ../../bin/cache/output.ply --scale 0.015";
	
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

	// Convert world transform to machine movement.
	vec3 modelLocalTargetPos = SurfacePos;
	vec3 actualModelLocalCamPos = SurfacePos + SurfaceNormal * 20.0f;
	vec3 viewDir = glm::normalize(-SurfaceNormal);

	ModelInspector->laserViewRot.y = viewDir.y * -90.0f;
	ModelInspector->laserViewRot.x = glm::degrees(atan2(-viewDir.z, -viewDir.x));

	mat4 modelLocalMat = glm::rotate(mat4(1.0f), glm::radians(ModelInspector->laserViewRot.x), vec3Up);
	vec3 modelWorldTargetPos = modelLocalMat * vec4(vec3(0.0f, 7.5, 0.0f) - modelLocalTargetPos, 1.0f);

	// Laser tool head movement.
	mat4 laserHeadMat = glm::translate(mat4(1.0f), vec3(0, 7.5f, 0.0f));
	laserHeadMat = glm::rotate(laserHeadMat, glm::radians(ModelInspector->laserViewRot.y), -vec3Forward);
	laserHeadMat = glm::translate(laserHeadMat, vec3(20.0f, 0, 0.0f));
	laserHeadMat = glm::rotate(laserHeadMat, glm::radians(90.0f), vec3Up);

	ModelInspector->laserViewPos = modelWorldTargetPos;

	// Model movement.
	mat4 modelMat = glm::translate(mat4(1.0f), ModelInspector->laserViewPos);
	modelMat = glm::rotate(modelMat, glm::radians(ModelInspector->laserViewRot.x), vec3Up);

	mat4 camViewMat = glm::inverse(laserHeadMat);
	mat4 camViewModelMat = camViewMat * modelMat;
	vec3 camModelLocaPos = vec4(0, 0, 0, 1) * camViewModelMat;
	mat4 projViewModelMat = Prep->projMat * camViewModelMat;

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(ModelInspector->coverageConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiCoverageConstantBuffer* constantBuffer = (ldiCoverageConstantBuffer*)ms.pData;
		constantBuffer->slotIndex = SlotId;
		AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageConstantBuffer, 0);
	}

	D3D11_MAPPED_SUBRESOURCE ms;
	AppContext->d3dDeviceContext->Map(AppContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
	constantBuffer->mvp = projViewModelMat;
	constantBuffer->color = vec4(actualModelLocalCamPos, 1);
	AppContext->d3dDeviceContext->Unmap(AppContext->mvpConstantBuffer, 0);

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

	ID3D11UnorderedAccessView* uavs[] = {
		ModelInspector->coverageBufferUav,
		ModelInspector->coverageAreaBufferUav
	};

	AppContext->d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(1, renderTargets, NULL, 1, 2, uavs, 0);

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

bool modelInspectorCalculateLaserViewPath(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->laserViewPath.clear();

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

		for (size_t i = 0; i < ModelInspector->surfels.size(); ++i) {
			// NOTE: 707 == 45degrees to laser view.
			if (ModelInspector->surfelMask[i] == 0 && coverageBufferData[i] >= 707) {
				++activeSurfels;
				ModelInspector->surfelMask[i] = viewIter + 1;
			}
		}

		std::cout << "Active surfels: " << activeSurfels << "\n";

		AppContext->d3dDeviceContext->Unmap(ModelInspector->coverageStagingBuffer, 0);

		if (activeSurfels == 0) {
			break;
		}

		ldiLaserViewPathPos pathPos = {};
		pathPos.surfacePos = vert->position;
		pathPos.surfaceNormal = vert->normal;

		ModelInspector->laserViewPath.push_back(pathPos);
	}

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
	ModelInspector->dergnModel = objLoadModel("../../assets/models/dergn.obj");
	//ModelInspector->dergnModel = objLoadModel("../../assets/models/materialball.obj");

	float globalScale = 2.0f;
	vec3 globalTranslate(0.0f, 3.0f, 0.0f);
	
	// Scale model.
	//float scale = 12.0 / 11.0;
	float scale = 5.0 / 11.0 * globalScale;
	for (int i = 0; i < ModelInspector->dergnModel.verts.size(); ++i) {
		ldiMeshVertex* vert = &ModelInspector->dergnModel.verts[i];
		vert->pos = vert->pos * scale + globalTranslate;
	}

	physicsCookMesh(AppContext->physics, &ModelInspector->dergnModel, &ModelInspector->cookedDergn);

	ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &ModelInspector->dergnModel);

	// Import PLY quad mesh.
	ldiQuadModel quadModel;
	if (!plyLoadQuadMesh("../cache/output.ply", &quadModel)) {
		return 1;
	}

	int quadCount = quadModel.indices.size() / 4;

	double totalSideLengths = 0.0;
	int totalSideCount = 0;
	float maxSide = FLT_MIN;
	float minSide = FLT_MAX;

	for (size_t i = 0; i < quadModel.verts.size(); ++i) {
		vec3* vert = &quadModel.verts[i];
		*vert = *vert * globalScale + globalTranslate;
	}

	// Side lengths.
	for (int i = 0; i < quadCount; ++i) {
		vec3 v0 = quadModel.verts[quadModel.indices[i * 4 + 0]];
		vec3 v1 = quadModel.verts[quadModel.indices[i * 4 + 1]];
		vec3 v2 = quadModel.verts[quadModel.indices[i * 4 + 2]];
		vec3 v3 = quadModel.verts[quadModel.indices[i * 4 + 3]];

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

	ModelInspector->dergnDebugModel = gfxCreateRenderQuadModelDebug(AppContext, &quadModel);

	ModelInspector->surfels = geoCreateSurfels(&quadModel);

	double t0 = _getTime(AppContext);
	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/dergnTexture.png", &x, &y, &n);
	//uint8_t* imageRawPixels = imageLoadRgba("../../assets/models/materialballTextureGrid.png", &x, &y, &n);

	ModelInspector->baseTexture.width = x;
	ModelInspector->baseTexture.height = y;
	ModelInspector->baseTexture.data = imageRawPixels;

	t0 = _getTime(AppContext) - t0;
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ") " << t0 * 1000.0f << " ms\n";

	t0 = _getTime(ModelInspector->appContext);
	geoTransferColorToSurfels(ModelInspector->appContext, &ModelInspector->cookedDergn, &ModelInspector->dergnModel, &ModelInspector->baseTexture, &ModelInspector->surfels);
	t0 = _getTime(ModelInspector->appContext) - t0;
	std::cout << "Transfer color: " << t0 * 1000.0f << " ms\n";

	// Create spatial structure for surfels.
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

	ModelInspector->coveragePointModel = gfxCreateCoveragePointRenderModel(AppContext, &ModelInspector->surfels, &quadModel);
	
	//ldiModel quadMeshTriModel;
	//convertQuadToTriModel(&quadModel, &quadMeshTriModel);

	//std::cout << "Quad tri mesh stats: " << quadMeshTriModel.verts.size() << 

	//ModelInspector->dergnRenderModel = gfxCreateRenderModel(AppContext, &quadMeshTriModel);

	ldiPointCloud pointCloud;
	if (!plyLoadPoints("../../assets/models/dergnScan.ply", &pointCloud)) {
		return 1;
	}

	ModelInspector->pointCloudRenderModel = gfxCreateRenderPointCloud(AppContext, &pointCloud);

	ModelInspector->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &quadModel);

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

	return 0;
}

vec3 projectPoint(ldiCamera* Camera, int ViewWidth, int ViewHeight, vec3 Pos) {
	// TODO: Should be cached in the camera after calculating final position.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(Camera->fov), (float)ViewWidth, (float)ViewHeight, 0.01f, 100.0f);
	
	mat4 projViewModelMat = projMat * camViewMat;
	
	vec4 worldPos = vec4(Pos.x, Pos.y, Pos.z, 1);
	vec4 clipPos = projViewModelMat * worldPos;
	clipPos.x /= clipPos.w;
	clipPos.y /= clipPos.w;

	vec3 screenPos;
	screenPos.x = (clipPos.x * 0.5 + 0.5) * ViewWidth;
	screenPos.y = (1.0f - (clipPos.y * 0.5 + 0.5)) * ViewHeight;
	screenPos.z = clipPos.z;

	return screenPos;
}

void modelInspectorRender(ldiModelInspector* ModelInspector, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = ModelInspector->appContext;

	if (Width != ModelInspector->mainViewWidth || Height != ModelInspector->mainViewHeight) {
		ModelInspector->mainViewWidth = Width;
		ModelInspector->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &ModelInspector->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	pushDebugLine(appContext, vec3(3, 0, 0), vec3(3, 5, 0), vec3(0.25, 0, 0.25));
	pushDebugLine(appContext, vec3(3, 5, 0), vec3(3, 10, 0), vec3(0.5, 0, 0.5));
	pushDebugLine(appContext, vec3(3, 10, 0), vec3(3, 12, 0), vec3(0.75, 0, 0.75));
	pushDebugLine(appContext, vec3(3, 12, 0), vec3(3, 15, 0), vec3(1, 0, 1));
	
	pushDebugBox(appContext, vec3(2.5, 10, 0), vec3(0.05, 0.05, 0.05), vec3(1, 0, 1));
	pushDebugBoxSolid(appContext, vec3(1, 10, 0), vec3(0.005, 0.005, 0.005), vec3(1, 0, 1));

	pushDebugBox(appContext, vec3(2.5, 10, -1.0), vec3(0.3, 0.3, 0.3), vec3(1, 0, 1));

	ldiTextInfo text0 = {};

	vec3 projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 0, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "0 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 5, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "50 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 10, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "100 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 12, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "120 mm";
		TextBuffer->push_back(text0);
	}

	projPos = projectPoint(&ModelInspector->camera, ModelInspector->mainViewWidth, ModelInspector->mainViewHeight, vec3(3, 15, 0));
	if (projPos.z > 0) {
		text0.position = vec2(projPos.x, projPos.y);
		text0.text = "150 mm";
		TextBuffer->push_back(text0);
	}

	/*vec3 rayOrigin(sin(_getTime(appContext)), 10, 0);
	vec3 rayDir(0, -1, 0);

	ldiRaycastResult rayHit = physicsRaycast(appContext->physics, &ModelInspector->cookedDergn, rayOrigin, rayDir);

	if (rayHit.hit) {
		pushDebugLine(appContext, rayOrigin, rayHit.pos, vec3(0, 1, 0));
	} else {
		pushDebugLine(appContext, rayOrigin, rayOrigin + rayDir * vec3(10, 10, 10), vec3(1, 0, 0));
	}*/
	
	// Grid.
	int gridCount = 60;
	float gridCellWidth = 1.0f;
	vec3 gridColor = ModelInspector->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	pushDebugSphere(appContext, vec3(0, 7.5f, 0), 7.5f, vec3(0.8f, 0.8f, 0.0f), 64);

	pushDebugSphere(appContext, vec3(0, 7.5f, 0), 7.5f + 5.0f, vec3(0.5f, 0.8f, 0.0f), 64);
	pushDebugSphere(appContext, vec3(0, 7.5f, 0), 27.5f, vec3(0.0f, 0.8f, 0.0f), 64);

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
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(ModelInspector->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(ModelInspector->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -ModelInspector->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(ModelInspector->camera.fov), (float)ModelInspector->mainViewWidth, (float)ModelInspector->mainViewHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

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
	}

	{
		ldiPointCloudVertex* v = &ModelInspector->pointDistrib.points[ModelInspector->selectedSurfel];
		pushDebugLine(appContext, v->position, v->position + v->normal * 20.0f, vec3(1.0f, 0, 0));
	}

	{
		for (size_t i = 0; i < ModelInspector->laserViewPath.size(); ++i) {
			ldiLaserViewPathPos* p = &ModelInspector->laserViewPath[i];
			vec3 pos = p->surfacePos + p->surfaceNormal * 20.0f;
			pushDebugBox(appContext, pos, vec3(0.1f, 0.1f, 0.1f), vec3(1, 0, 1));
			pushDebugLine(appContext, pos, pos + -p->surfaceNormal, vec3(1.0f, 0, 0));
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Render debug primitives.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
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
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			constantBuffer->view = camViewMat;
			constantBuffer->proj = projMat;
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
		gfxRenderModel(appContext, &ModelInspector->dergnRenderModel, false, ModelInspector->shaderResourceViewTest, ModelInspector->texSamplerState);
	}

	if (ModelInspector->primaryModelShowWireframe) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
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
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			constantBuffer->view = camViewMat;
			constantBuffer->proj = projMat;
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
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderDebugModel(appContext, &ModelInspector->dergnDebugModel);
	}

	if (ModelInspector->quadMeshShowWireframe) {	
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(0, 0, 0, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderWireModel(appContext, &ModelInspector->quadMeshWire);
	}

	if (ModelInspector->showSurfels) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(ModelInspector->camera.position, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderSurfelModel(appContext, &ModelInspector->surfelRenderModel, appContext->dotShaderResourceView, appContext->dotSamplerState);
	}

	//if (ModelInspector->showSurfels) {
	if (true) {
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
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
		ImGui::Checkbox("Area debug", &tool->quadMeshShowDebug);
		ImGui::Checkbox("Quad wireframe", &tool->quadMeshShowWireframe);
		ImGui::Checkbox("Surfels", &tool->showSurfels);
		ImGui::Checkbox("Spatial bounds", &tool->showSpatialBounds);
		ImGui::Checkbox("Spatial occupied cells", &tool->showSpatialCells);

		ImGui::Checkbox("Sample sites", &tool->showSampleSites);
		
		ImGui::Separator();
		ImGui::Text("Point cloud");
		ImGui::Checkbox("Show", &tool->showPointCloud);
		ImGui::SliderFloat("World size", &tool->pointWorldSize, 0.0f, 1.0f);
		ImGui::SliderFloat("Screen size", &tool->pointScreenSize, 0.0f, 32.0f);
		ImGui::SliderFloat("Screen blend", &tool->pointScreenSpaceBlend, 0.0f, 1.0f);

		ImGui::Separator();
		ImGui::Text("Laser view");
		ImGui::DragFloat3("Cam position", (float*)&tool->laserViewPos, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat2("Cam rotation", (float*)&tool->laserViewRot, 0.1f, -360.0f, 360.0f);
		ImGui::SliderInt("Surfel", &tool->selectedSurfel, 0, tool->pointDistrib.points.size() - 1);
		
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
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
		}

		// Viewport overlay.
		// TODO: Use command buffer of text locations/strings.
		/*{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();

			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::Begin("__viewport", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs);

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			if (clipPos.z > 0) {
				drawList->AddText(ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 255, 255), "X");
			}

			ImGui::End();
		}*/

		{
			// Viewport overlay widgets.
		
			//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
			//{
			//	const float PAD = 10.0f;
			//	const ImGuiViewport* viewport = ImGui::GetMainViewport();
			//	ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			//	ImVec2 work_size = viewport->WorkSize;
			//	ImVec2 window_pos, window_pos_pivot;
			//	window_pos.x = (work_pos.x + PAD);
			//	window_pos.y = (work_pos.y + PAD);
			//	window_pos_pivot.x = 0.0f;
			//	window_pos_pivot.y = 0.0f;
			//	ImGui::SetNextWindowPos(ImVec2(startPos.x + 10, startPos.y + 10), ImGuiCond_Always, window_pos_pivot);
			//	//ImGui::SetNextWindowViewport(viewport->ID);
			//	window_flags |= ImGuiWindowFlags_NoMove;
			//	
			//}
			//ImGui::Begin("_simpleOverlayMainView", 0, window_flags);
			//ImGui::End();
		
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