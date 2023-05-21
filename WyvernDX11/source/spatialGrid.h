#pragma once

//----------------------------------------------------------------------------------------------------
// Basic spatial grid.
//----------------------------------------------------------------------------------------------------
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
		}
		else {
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

		pushDebugBoxMinMax(&AppContext->defaultDebug, worldSpace, max, vec3(1, 0, 1));
	}
}

void spatialGridRenderDebug(ldiApp* AppContext, ldiSpatialGrid* Grid, bool Bounds, bool CellView) {
	if (Bounds) {
		pushDebugBox(&AppContext->defaultDebug, Grid->origin, Grid->size, vec3(1, 0, 1));
	}

	if (CellView) {
		vec3 color = vec3(0, 0, 1);

		// Along X axis.
		for (int iY = 0; iY < Grid->countY; ++iY) {
			for (int iZ = 0; iZ < Grid->countZ; ++iZ) {
				pushDebugLine(&AppContext->defaultDebug, vec3(Grid->min.x, Grid->min.y + iY * Grid->cellSize, Grid->min.z + iZ * Grid->cellSize), vec3(Grid->max.x, Grid->min.y + iY * Grid->cellSize, Grid->min.z + iZ * Grid->cellSize), color);
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

//----------------------------------------------------------------------------------------------------
// Poisson disk spatial grid.
//----------------------------------------------------------------------------------------------------
struct ldiPoissonSample {
	vec3 position;
	vec3 rotation;
	float value;
};

struct ldiPoissonSpatialGridCell {
	std::vector<ldiPoissonSample> samples;
};

struct ldiPoissonSpatialGrid {
	vec3 min;
	vec3 max;
	vec3 origin;
	vec3 size;
	float cellSize;
	int countX;
	int countY;
	int countZ;
	int countTotal;
	ldiPoissonSpatialGridCell* cells;
};

void poissonSpatialGridDestroy(ldiPoissonSpatialGrid* Grid) {
	if (Grid->cells) {
		delete[] Grid->cells;
	}

	memset(Grid, 0, sizeof(ldiPoissonSpatialGrid));
}

void poissonSpatialGridInit(ldiPoissonSpatialGrid* Grid, vec3 MinBounds, vec3 MaxBounds, float CellSize) {
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
	Grid->cells = new ldiPoissonSpatialGridCell[Grid->countTotal];
}

vec3 poissonSpatialGridGetCellFromWorldPosition(ldiPoissonSpatialGrid* Grid, vec3 Position) {
	vec3 local = Position - Grid->min;
	return local / Grid->cellSize;
}

void poissonSpatialGridGetCellFromWorldPositionInt(ldiPoissonSpatialGrid* Grid, vec3 Position, int* X, int* Y, int* Z) {
	vec3 local = Position - Grid->min;
	vec3 result = local / Grid->cellSize;

	*X = (int)(local.x / Grid->cellSize);
	*Y = (int)(local.y / Grid->cellSize);
	*Z = (int)(local.z / Grid->cellSize);
}

inline int poissonSpatialGridGetCellId(ldiPoissonSpatialGrid* Grid, int CellX, int CellY, int CellZ) {
	return CellX + CellY * Grid->countX + CellZ * Grid->countX * Grid->countY;
}

int poissonSpatialGridGetCellIdFromWorldPosition(ldiPoissonSpatialGrid* Grid, vec3 Position) {
	vec3 local = Position - Grid->min;

	int cellX = (int)(local.x / Grid->cellSize);
	int cellY = (int)(local.y / Grid->cellSize);
	int cellZ = (int)(local.z / Grid->cellSize);

	return poissonSpatialGridGetCellId(Grid, cellX, cellY, cellZ);
}