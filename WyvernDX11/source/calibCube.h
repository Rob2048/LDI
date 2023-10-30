#pragma once

struct ldiCalibCubeSide {
	int id;
	vec3 corners[4];
	ldiPlane plane;
};

struct ldiCalibCube {
	float cellSize;
	int gridSize;
	int pointsPerSide;
	int totalPoints;

	float scaleFactor;

	std::vector<vec3> points;

	std::vector<ldiCalibCubeSide> sides;
	vec3 corners[8];
};

bool calibCubeCalculateMetrics(ldiCalibCube* Cube) {
	//----------------------------------------------------------------------------------------------------
	// Centroid.
	//----------------------------------------------------------------------------------------------------
	vec3 centroidAccum(0.0f, 0.0f, 0.0f);
	int centroidCount = 0;

	for (size_t i = 0; i < Cube->points.size(); ++i) {
		if (i >= 18 && i <= 26) {
			continue;
		}

		centroidAccum += Cube->points[i];
		++centroidCount;
	}
	vec3 centroid = centroidAccum / (float)centroidCount;

	//----------------------------------------------------------------------------------------------------
	// Sides.
	//----------------------------------------------------------------------------------------------------
	Cube->sides.resize(6);
	for (int i = 0; i < 6; ++i) {
		Cube->sides[i].id = i;

		if (i == 2) {
			continue;
		}

		std::vector<vec3> points;

		for (int j = i * 9; j < (i + 1) * 9; ++j) {
			points.push_back(Cube->points[j]);
		}

		computerVisionFitPlane(points, &Cube->sides[i].plane);
	}

	// NOTE: Correct plane directions.
	if (Cube->sides[1].plane.normal.y > 0.0f) {
		Cube->sides[1].plane.normal = -Cube->sides[1].plane.normal;
	}

	ldiCalibCubeSide bottom = {};
	bottom.id = 2;
	bottom.plane = Cube->sides[1].plane;
	// NOTE: Using a guess of 40mm, but doesn't take refined scaling into account.
	bottom.plane.point += bottom.plane.normal * 4.0f;
	Cube->sides[2] = bottom;

	

	//----------------------------------------------------------------------------------------------------
	// Corners.
	//----------------------------------------------------------------------------------------------------
	if (!getPointAtIntersectionOfPlanes(Cube->sides[1].plane, Cube->sides[0].plane, Cube->sides[3].plane, &Cube->corners[0])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[1].plane, Cube->sides[0].plane, Cube->sides[5].plane, &Cube->corners[1])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[1].plane, Cube->sides[4].plane, Cube->sides[5].plane, &Cube->corners[2])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[1].plane, Cube->sides[4].plane, Cube->sides[3].plane, &Cube->corners[3])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[2].plane, Cube->sides[0].plane, Cube->sides[3].plane, &Cube->corners[4])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[2].plane, Cube->sides[0].plane, Cube->sides[5].plane, &Cube->corners[5])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[2].plane, Cube->sides[4].plane, Cube->sides[5].plane, &Cube->corners[6])) { return false; }
	if (!getPointAtIntersectionOfPlanes(Cube->sides[2].plane, Cube->sides[4].plane, Cube->sides[3].plane, &Cube->corners[7])) { return false; }

	Cube->sides[0].corners[0] = Cube->corners[0];
	Cube->sides[0].corners[1] = Cube->corners[1];
	Cube->sides[0].corners[2] = Cube->corners[5];
	Cube->sides[0].corners[3] = Cube->corners[4];

	Cube->sides[1].corners[0] = Cube->corners[0];
	Cube->sides[1].corners[1] = Cube->corners[3];
	Cube->sides[1].corners[2] = Cube->corners[2];
	Cube->sides[1].corners[3] = Cube->corners[1];

	Cube->sides[2].corners[0] = Cube->corners[4];
	Cube->sides[2].corners[1] = Cube->corners[5];
	Cube->sides[2].corners[2] = Cube->corners[6];
	Cube->sides[2].corners[3] = Cube->corners[7];

	Cube->sides[3].corners[0] = Cube->corners[3];
	Cube->sides[3].corners[1] = Cube->corners[0];
	Cube->sides[3].corners[2] = Cube->corners[4];
	Cube->sides[3].corners[3] = Cube->corners[7];

	Cube->sides[4].corners[0] = Cube->corners[2];
	Cube->sides[4].corners[1] = Cube->corners[3];
	Cube->sides[4].corners[2] = Cube->corners[7];
	Cube->sides[4].corners[3] = Cube->corners[6];

	Cube->sides[5].corners[0] = Cube->corners[1];
	Cube->sides[5].corners[1] = Cube->corners[2];
	Cube->sides[5].corners[2] = Cube->corners[6];
	Cube->sides[5].corners[3] = Cube->corners[5];

	std::cout << "Calibration cube corner metrics\n";
	std::cout << glm::distance(Cube->corners[0], Cube->corners[3]) << "\n";
	std::cout << glm::distance(Cube->corners[3], Cube->corners[2]) << "\n";
	std::cout << glm::distance(Cube->corners[2], Cube->corners[1]) << "\n";
	std::cout << glm::distance(Cube->corners[1], Cube->corners[0]) << "\n";

	//----------------------------------------------------------------------------------------------------
	// Scale factor.
	//----------------------------------------------------------------------------------------------------
	float distAccum = 0.0f;
	int distCount = 0;

	int size = Cube->gridSize;

	for (int b = 0; b < 6; ++b) {
		if (b == 2) {
			continue;
		}

		for (int r = 0; r < size; ++r) {
			for (int c = 0; c < (size - 1); ++c) {
				int id0 = c + (r * size) + b * (size * size);
				int id1 = c + 1 + (r * size) + b * (size * size);
				float d = glm::distance(Cube->points[id0], Cube->points[id1]);
				std::cout << id0 << ":" << id1 << " = " << d << "\n";
				distAccum += d;
				++distCount;

				id0 = c * size + r + b * (size * size);
				id1 = (c + 1) * size + r + b * (size * size);
				d = glm::distance(Cube->points[id0], Cube->points[id1]);
				std::cout << id0 << ":" << id1 << " = " << d << "\n";
				distAccum += d;
				++distCount;
			}
		}
	}

	float distAvg = distAccum / (float)distCount;
	Cube->scaleFactor = 0.9 / distAvg;
	std::cout << "Avg dist: " << distAvg << " Scale factor: " << Cube->scaleFactor << "\n";

	std::cout << glm::distance(Cube->corners[0], Cube->corners[3]) * Cube->scaleFactor << "\n";
	std::cout << glm::distance(Cube->corners[3], Cube->corners[2]) * Cube->scaleFactor << "\n";
	std::cout << glm::distance(Cube->corners[2], Cube->corners[1]) * Cube->scaleFactor << "\n";
	std::cout << glm::distance(Cube->corners[1], Cube->corners[0]) * Cube->scaleFactor << "\n";

	// TODO: Normalize to match scale?

	return true;
}

// Generate default cube points.
void calibCubeInit(ldiCalibCube* Cube) {
	Cube->gridSize = 3;
	Cube->cellSize = 0.9f;
	Cube->pointsPerSide = Cube->gridSize * Cube->gridSize;
	Cube->totalPoints = Cube->pointsPerSide * 6;

	Cube->points.resize(Cube->totalPoints, vec3(0.0f, 0.0f, 0.0f));

	float ps = Cube->cellSize;
	// Board 0.
	Cube->points[0] = { 0.0f, ps * 0, -ps * 0 };
	Cube->points[3] = { 0.0f, ps * 0, -ps * 1 };
	Cube->points[6] = { 0.0f, ps * 0, -ps * 2 };
	Cube->points[1] = { 0.0f, ps * 1, -ps * 0 };
	Cube->points[4] = { 0.0f, ps * 1, -ps * 1 };
	Cube->points[7] = { 0.0f, ps * 1, -ps * 2 };
	Cube->points[2] = { 0.0f, ps * 2, -ps * 0 };
	Cube->points[5] = { 0.0f, ps * 2, -ps * 1 };
	Cube->points[8] = { 0.0f, ps * 2, -ps * 2 };

	// Board 1.
	Cube->points[15] = { -ps * 0 - 1.1f, 2.9f, -ps * 0 };
	Cube->points[16] = { -ps * 0 - 1.1f, 2.9f, -ps * 1 };
	Cube->points[17] = { -ps * 0 - 1.1f, 2.9f, -ps * 2 };
	Cube->points[12] = { -ps * 1 - 1.1f, 2.9f, -ps * 0 };
	Cube->points[13] = { -ps * 1 - 1.1f, 2.9f, -ps * 1 };
	Cube->points[14] = { -ps * 1 - 1.1f, 2.9f, -ps * 2 };
	Cube->points[ 9] = { -ps * 2 - 1.1f, 2.9f, -ps * 0 };
	Cube->points[10] = { -ps * 2 - 1.1f, 2.9f, -ps * 1 };
	Cube->points[11] = { -ps * 2 - 1.1f, 2.9f, -ps * 2 };

	// Board 3.
	Cube->points[27] = { -ps * 0 - 1.1f, ps * 0, -2.9f + 4.0f };
	Cube->points[28] = { -ps * 1 - 1.1f, ps * 0, -2.9f + 4.0f };
	Cube->points[29] = { -ps * 2 - 1.1f, ps * 0, -2.9f + 4.0f };
	Cube->points[30] = { -ps * 0 - 1.1f, ps * 1, -2.9f + 4.0f };
	Cube->points[31] = { -ps * 1 - 1.1f, ps * 1, -2.9f + 4.0f };
	Cube->points[32] = { -ps * 2 - 1.1f, ps * 1, -2.9f + 4.0f };
	Cube->points[33] = { -ps * 0 - 1.1f, ps * 2, -2.9f + 4.0f };
	Cube->points[34] = { -ps * 1 - 1.1f, ps * 2, -2.9f + 4.0f };
	Cube->points[35] = { -ps * 2 - 1.1f, ps * 2, -2.9f + 4.0f };

	// Board 4.
	Cube->points[44] = { -4.0f, ps * 0, -ps * 0 };
	Cube->points[43] = { -4.0f, ps * 0, -ps * 1 };
	Cube->points[42] = { -4.0f, ps * 0, -ps * 2 };
	Cube->points[41] = { -4.0f, ps * 1, -ps * 0 };
	Cube->points[40] = { -4.0f, ps * 1, -ps * 1 };
	Cube->points[39] = { -4.0f, ps * 1, -ps * 2 };
	Cube->points[38] = { -4.0f, ps * 2, -ps * 0 };
	Cube->points[37] = { -4.0f, ps * 2, -ps * 1 };
	Cube->points[36] = { -4.0f, ps * 2, -ps * 2 };

	// Board 5.
	Cube->points[45] = { -ps * 0 - 1.1f, ps * 0, -2.9f };
	Cube->points[48] = { -ps * 1 - 1.1f, ps * 0, -2.9f };
	Cube->points[51] = { -ps * 2 - 1.1f, ps * 0, -2.9f };
	Cube->points[46] = { -ps * 0 - 1.1f, ps * 1, -2.9f };
	Cube->points[49] = { -ps * 1 - 1.1f, ps * 1, -2.9f };
	Cube->points[52] = { -ps * 2 - 1.1f, ps * 1, -2.9f };
	Cube->points[47] = { -ps * 0 - 1.1f, ps * 2, -2.9f };
	Cube->points[50] = { -ps * 1 - 1.1f, ps * 2, -2.9f };
	Cube->points[53] = { -ps * 2 - 1.1f, ps * 2, -2.9f };

	calibCubeCalculateMetrics(Cube);
}
