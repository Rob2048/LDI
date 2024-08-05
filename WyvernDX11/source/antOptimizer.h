#pragma once

struct ldiAntPoint {
	vec3 position;
};

struct ldiAntEdges {
	float* edges;
	int totalEdges;
	int totalPoints;
};

struct ldiAntOptimizer {
	std::vector<ldiAntPoint> points;
	std::vector<float> costs;
	ldiAntEdges edges;
	ldiAntEdges edgesScratch;

	std::vector<int> initialPath;
};

void antEdgesInit(ldiAntEdges* Edges, int PointCount) {
	if (Edges->edges) {
		delete[] Edges->edges;
	}

	Edges->totalPoints = PointCount;
	Edges->totalEdges = (int)((double)PointCount * ((double)PointCount - 1) / 2.0);
	Edges->edges = new float[Edges->totalEdges];
	memset(Edges->edges, 0, sizeof(float) * Edges->totalEdges);

	std::cout << "Total edges: " << Edges->totalEdges << "\n";
}

inline int antEdgeGetIndex(ldiAntEdges* Edges, int P0, int P1) {
	if (P0 > P1) {
		int temp = P1;
		P1 = P0;
		P0 = temp;
	}

	return (Edges->totalPoints * P0) - (P0 * (P0 + 1) / 2) + (P1 - P0) - 1;
}

inline float antEdgesGet(ldiAntEdges* Edges, int P0, int P1) {
	int index = antEdgeGetIndex(Edges, P0, P1);
	return Edges->edges[index];
}

inline void antEdgesSet(ldiAntEdges* Edges, int P0, int P1, float value) {
	int index = antEdgeGetIndex(Edges, P0, P1);
	Edges->edges[index] = value;
}

void antInit(ldiAntOptimizer* Ant) {
	Ant->points.clear();

	vec3 minPos(0, 0, 0);
	vec3 maxPos(10, 10, 10);

	for (int i = 0; i < 100; ++i) {
		ldiAntPoint p;

		p.position.x = lerp(minPos.x, maxPos.x, getRandomValue());
		p.position.y = lerp(minPos.y, maxPos.y, getRandomValue());
		p.position.z = lerp(minPos.z, maxPos.z, getRandomValue());
		
		Ant->points.push_back(p);
	}

	Ant->costs.clear();
	Ant->costs.reserve(Ant->points.size() * Ant->points.size());

	double t0 = getTime();

	// Calculate the distances to each point.
	int numPoints = (int)Ant->points.size();
	for (int i = 0; i < numPoints; ++i) {
		for (int j = 0; j < numPoints; ++j) {
			float distance = glm::distance(Ant->points[i].position, Ant->points[j].position);
			Ant->costs.push_back(distance);
		}
	}

	// Edges table.
	antEdgesInit(&Ant->edges, (int)Ant->points.size());
	antEdgesInit(&Ant->edgesScratch, (int)Ant->points.size());

	// Set edges to initial pheromone level.
	float initialPheromones = 0.2;
	float pheromoneDecrease = 0.4;
	float q = 8.0;
	float a = 1.0;
	float b = 0.5; // Lower uses less cost, only pheromones.
	// TODO: Maybe a constant to scale the cost values?

	for (int i = 0; i < Ant->edges.totalEdges; ++i) {
		Ant->edges.edges[i] = initialPheromones;
	}

	t0 = getTime() - t0;
	std::cout << "Assign ant optimizer params: " << (t0 * 1000.0) << " ms\n";

	std::vector<int> bestPath(1000);
	float bestPathCost = FLT_MAX;

	for (int optIter = 0; optIter < 100; ++optIter) {
		// Reset pheromone additions.
		for (int i = 0; i < Ant->edgesScratch.totalEdges; ++i) {
			Ant->edgesScratch.edges[i] = 0.0f;
		}

		for (int antIter = 0; antIter < Ant->points.size(); ++antIter) {
			t0 = getTime();

			std::vector<int> visitList(Ant->points.size());

			for (size_t i = 0; i < Ant->points.size(); ++i) {
				visitList[i] = (int)i;
			}

			visitList[antIter] = visitList[visitList.size() - 1];
			visitList[visitList.size() - 1] = antIter;
			int currentPoint = antIter;
			float totalCost = 0.0f;

			for (size_t p = 1; p < Ant->points.size(); ++p) {
				float totalProbability = 0.0f;

				// Iterate all nodes and get total probability weighting.
				for (size_t i = 0; i < Ant->points.size() - p; ++i) {
					float cost = Ant->costs[currentPoint * Ant->points.size() + visitList[i]];
					float pheromones = antEdgesGet(&Ant->edges, currentPoint, visitList[i]);
					float probability = powf(pheromones, a) * powf(cost, b);
					totalProbability += probability;
				}

				// Pick random probability number.
				float selector = getRandomValue() * totalProbability;

				float selectProbability = 0.0f;

				bool didFindNext = false;

				// Iterate again and stop at node that was randomly selected.
				for (size_t i = 0; i < Ant->points.size() - p; ++i) {
					float cost = Ant->costs[currentPoint * Ant->points.size() + visitList[i]];
					float pheromones = antEdgesGet(&Ant->edges, currentPoint, visitList[i]);
					float probability = powf(pheromones, a) * powf(cost, b);
					selectProbability += probability;

					if (selectProbability >= selector) {
						didFindNext = true;
						currentPoint = visitList[i];
						visitList[i] = visitList[visitList.size() - 1 - p];
						visitList[visitList.size() - 1 - p] = currentPoint;

						totalCost += cost;

						break;
					}
				}

				if (!didFindNext) {
					std::cout << "WOOPS: " << selector << " " << totalProbability << " " << selectProbability << "\n";
					return;
				}
			}

			//for (size_t p = 1; p < Ant->points.size(); ++p) {
			//	float smallestCost = FLT_MAX;
			//	int smallestId = -1;

			//	for (size_t i = 0; i < Ant->points.size() - p; ++i) {
			//		float cost = Ant->costs[currentPoint * Ant->points.size() + visitList[i]];

			//		if (cost < smallestCost) {
			//			smallestCost = cost;
			//			smallestId = i;
			//		}
			//	}

			//	if (smallestId == -1) {
			//		// NOTE: Major error if this is reached.
			//		return;
			//	} else {
			//		currentPoint = visitList[smallestId];
			//		visitList[smallestId] = visitList[visitList.size() - 1 - p];
			//		visitList[visitList.size() - 1 - p] = currentPoint;

			//		totalCost += smallestCost;
			//	}
			//}
		
			if (totalCost < bestPathCost) {
				bestPath = visitList;
				bestPathCost = totalCost;
			}

			// Add pheromones at edges that ant travelled.
			float pheromoneIncrease = q / totalCost;

			for (size_t i = 1; i < visitList.size(); ++i) {
				int scratchIndex = antEdgeGetIndex(&Ant->edgesScratch, visitList[i - 1], visitList[i]);
				Ant->edgesScratch.edges[scratchIndex] += pheromoneIncrease;
			}

			t0 = getTime() - t0;
			std::cout << "Ant: " << antIter << " with cost: " << totalCost << " in " << (t0 * 1000.0) << " ms\n";
		}

		// Decrease pheromones at all edges.
		for (int i = 0; i < Ant->edges.totalEdges; ++i) {
			Ant->edges.edges[i] = Ant->edges.edges[i] * pheromoneDecrease + Ant->edgesScratch.edges[i];
		}

		std::cout << "Iter: " << optIter << " Best: " << bestPathCost << "\n";
	}

	//std::cout << "Initial greedy algo: " << (t0 * 1000.0) << " with cost: " << bestPathCost << " ms\n";
	//std::cout << "Final best cost: " << bestPathCost << "\n";

	for (int i = (int)bestPath.size() - 1; i >= 0; --i) {
		Ant->initialPath.push_back(bestPath[i]);
	}
}

void antUpdate(ldiAntOptimizer* Ant) {
	// Update particle positions.
	for (size_t pointIter = 0; pointIter < Ant->points.size(); ++pointIter) {
		ldiAntPoint* p = &Ant->points[pointIter];
	}
}

void antRender(ldiDebugPrims* DebugPrims, ldiAntOptimizer* Ant, mat4 World) {
	for (size_t pointIter = 0; pointIter < Ant->points.size(); ++pointIter) {
		ldiAntPoint* p = &Ant->points[pointIter];

		pushDebugSphere(DebugPrims, World * vec4(p->position, 1.0f), 0.1, vec3(1, 0, 0.5f), 4);
		
		//std::cout << "P" << pointIter << " " << GetStr(p->position) << "\n";
	}

	int numPoints = (int)Ant->points.size();
	for (int i = 0; i < numPoints; ++i) {
		for (int j = i + 1; j < numPoints; ++j) {
			//float dist = Ant->costs[i * Ant->points.size() + j] / 12.0;
			//vec3 col = glm::mix(vec3(1, 0, 0), vec3(0, 1, 0), dist);

			float pheromones = antEdgesGet(&Ant->edges, i, j);
			vec3 col = glm::mix(vec3(1, 0, 0), vec3(0, 1, 0), pheromones);
			
			pushDebugLine(DebugPrims, Ant->points[i].position, Ant->points[j].position, col);

			float pheromoneLevel = antEdgesGet(&Ant->edges, i, j);
		}
	}

	for (size_t i = 1; i < Ant->initialPath.size(); ++i) {
		int idx0 = Ant->initialPath[i - 1];
		int idx1 = Ant->initialPath[i - 0];

		pushDebugLine(DebugPrims, Ant->points[idx0].position + vec3(0, 0.01, 0), Ant->points[idx1].position + vec3(0, 0.01, 0), vec3(1, 1, 1));
		//pushDebugLine(DebugPrims, Ant->points[idx0].position, Ant->points[idx1].position, vec3(1, 1, 1));
	}
}