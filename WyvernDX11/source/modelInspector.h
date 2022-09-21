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

	ldiRenderLines				quadMeshWire;
	ldiRenderModel				surfelRenderModel;

	ldiRenderPointCloud			pointCloudRenderModel;

	ldiPhysicsMesh				cookedDergn;

	std::vector<ldiSurfel>		surfels;
	ldiImage					baseTexture;

	ldiSpatialGrid				spatialGrid;
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
	int triCount = SrcModel->indices.size() / 3;

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

int modelInspectorInit(ldiApp* AppContext, ldiModelInspector* ModelInspector) {
	ModelInspector->appContext = AppContext;
	ModelInspector->mainViewWidth = 0;
	ModelInspector->mainViewHeight = 0;
	
	ModelInspector->camera = {};
	ModelInspector->camera.position = vec3(0, 0, 10);
	ModelInspector->camera.rotation = vec3(0, 0, 0);

	ModelInspector->dergnModel = objLoadModel("../../assets/models/dergn.obj");
	//ModelInspector->dergnModel = objLoadModel("../../assets/models/materialball.obj");

	// Scale model.
	//float scale = 12.0 / 11.0;
	float scale = 5.0 / 11.0;
	for (int i = 0; i < ModelInspector->dergnModel.verts.size(); ++i) {
		ldiMeshVertex* vert = &ModelInspector->dergnModel.verts[i];
		vert->pos = vert->pos * scale;
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

	ModelInspector->surfelRenderModel = gfxCreateSurfelRenderModel(AppContext, ModelInspector->surfels);

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

	return 0;
}

vec3 projectPoint(ldiCamera* Camera, int ViewWidth, int ViewHeight, vec3 Pos) {
	// TODO: Should be cached in the camera after calculating final position.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)ViewWidth, (float)ViewHeight, 0.01f, 100.0f);
	
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
	int gridCount = 10;
	float gridCellWidth = 1.0f;
	vec3 gridColor = ModelInspector->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
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
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(ModelInspector->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(ModelInspector->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -ModelInspector->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)ModelInspector->mainViewWidth, (float)ModelInspector->mainViewHeight, 0.01f, 100.0f);
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
}

void modelInspectorShowUi(ldiModelInspector* tool) {
	ImGui::Begin("Model inspector controls");
	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Background", (float*)&tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid", (float*)&tool->gridColor);
		ImGui::SliderFloat("Camera speed", &tool->cameraSpeed, 0.0f, 1.0f);
		
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
		
		ImGui::Separator();
		ImGui::Text("Point cloud");
		ImGui::Checkbox("Show", &tool->showPointCloud);
		ImGui::SliderFloat("World size", &tool->pointWorldSize, 0.0f, 1.0f);
		ImGui::SliderFloat("Screen size", &tool->pointScreenSize, 0.0f, 32.0f);
		ImGui::SliderFloat("Screen blend", &tool->pointScreenSpaceBlend, 0.0f, 1.0f);
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


	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Model inspector", 0, ImGuiWindowFlags_NoCollapse);

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
		ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 25), false, ImGuiWindowFlags_NoScrollbar);

		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::EndChild();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}