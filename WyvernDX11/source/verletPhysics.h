#pragma once

struct ldiVerletPoint {
	vec3 position;
	vec3 oldPosition;
	vec3 acceleration;
	bool fixed;
};

struct ldiVerletDistanceConstraint {
	int p0Index;
	int p1Index;
	float distance;
};


struct ldiVerletAttractor {
	int pIndex;
	vec3 position;
};

struct ldiVerletPhysics {
	std::vector<ldiVerletPoint> points;
	std::vector<ldiVerletDistanceConstraint> distanceConstraints;
	std::vector<ldiVerletAttractor> attractors;
	float timestep;
};

int verletPhysicsCreatePoint(ldiVerletPhysics* Physics, vec3 Position, bool Fixed = false) {
	ldiVerletPoint p = {};
	p.position = Position;
	p.oldPosition = Position;
	p.acceleration = vec3(0.0f);
	p.fixed = Fixed;

	Physics->points.push_back(p);

	return (int)Physics->points.size() - 1;
}

int verletPhysicsCreateDistanceConstraint(ldiVerletPhysics* Physics, int P0Index, int P1Index, float Distance) {
	ldiVerletDistanceConstraint c = {};
	c.distance = Distance;
	c.p0Index = P0Index;
	c.p1Index = P1Index;

	Physics->distanceConstraints.push_back(c);

	return (int)Physics->distanceConstraints.size() - 1;
}

int verletPhysicsCreateAttractor(ldiVerletPhysics* Physics, int PointIndex, vec3 Position) {
	ldiVerletAttractor a = {};
	a.pIndex = PointIndex;
	a.position = Position;

	Physics->attractors.push_back(a);

	return (int)Physics->attractors.size() - 1;
}

void verletPhysicsInit(ldiVerletPhysics* Physics) {
	Physics->points.clear();
	Physics->distanceConstraints.clear();
	Physics->attractors.clear();
	Physics->timestep = 1.0 / 60.0;
}

float verletPhysicsUpdate(ldiVerletPhysics* Physics) {
	// Update attractors.
	for (size_t i = 0; i < Physics->attractors.size(); ++i) {
		ldiVerletAttractor* a = &Physics->attractors[i];
		ldiVerletPoint* p0 = &Physics->points[a->pIndex];

		float stiffness = 100.0;
		float damping = 1000.5;
		
		// Force of the spring.
		vec3 x = p0->position - a->position;
		vec3 v = (p0->position - p0->oldPosition);
		vec3 f = (stiffness * -x) + (damping * -v);

		float mass = 1.0;
		p0->acceleration += f / mass;
	}

	// Update particle positions.
	for (size_t pointIter = 0; pointIter < Physics->points.size(); ++pointIter) {
		ldiVerletPoint* p = &Physics->points[pointIter];

		if (p->fixed) {
			continue;
		}

		vec3 velocity = p->position - p->oldPosition;
		p->oldPosition = p->position;
		p->position += velocity + p->acceleration * Physics->timestep * Physics->timestep;
		p->acceleration = vec3Zero;
	}

	// Update constraints.
	for (int i = 0; i < 128; ++i) {
		for (size_t constIter = 0; constIter < Physics->distanceConstraints.size(); ++constIter) {
			ldiVerletDistanceConstraint* c = &Physics->distanceConstraints[constIter];
			ldiVerletPoint* p0 = &Physics->points[c->p0Index];
			ldiVerletPoint* p1 = &Physics->points[c->p1Index];

			vec3 p0p1 = p1->position - p0->position;
			float currentDist = glm::length(p0p1);

			vec3 dir = vec3Up;
			if (currentDist != 0.0f) {
				dir = glm::normalize(p0p1);
			}

			float error = currentDist - c->distance;

			if (!p0->fixed) {
				p0->position += dir * error * 0.5f;
			}

			if (!p1->fixed) {
				p1->position -= dir * error * 0.5f;
			}
		}
	}

	float movementEnergy = 0.0f;
	for (size_t pointIter = 0; pointIter < Physics->points.size(); ++pointIter) {
		ldiVerletPoint* p = &Physics->points[pointIter];

		if (p->fixed) {
			continue;
		}

		vec3 velocity = p->position - p->oldPosition;
		movementEnergy += glm::length(velocity);
	}

	return movementEnergy;
}

void verletPhysicsRender(ldiDebugPrims* DebugPrims, ldiVerletPhysics* Physics, mat4 World) {
	for (size_t pointIter = 0; pointIter < Physics->points.size(); ++pointIter) {
		ldiVerletPoint* p = &Physics->points[pointIter];

		if (p->fixed) {
			pushDebugSphere(DebugPrims, World * vec4(p->position, 1.0f), 0.002, vec3(1, 0, 0.5f), 4);
		} else {
			pushDebugSphere(DebugPrims, World * vec4(p->position, 1.0f), 0.001, vec3(1, 1, 1), 4);
		}

		//std::cout << "P" << pointIter << " " << GetStr(p->position) << "\n";
	}

	for (size_t constIter = 0; constIter < Physics->distanceConstraints.size(); ++constIter) {
		ldiVerletDistanceConstraint* c = &Physics->distanceConstraints[constIter];
		ldiVerletPoint* p0 = &Physics->points[c->p0Index];
		ldiVerletPoint* p1 = &Physics->points[c->p1Index];

		pushDebugLine(DebugPrims, World * vec4(p0->position, 1.0f), World * vec4(p1->position, 1.0f), vec3(1, 1, 1));
	}

	for (size_t i = 0; i < Physics->attractors.size(); ++i) {
		ldiVerletAttractor* a = &Physics->attractors[i];
		ldiVerletPoint* p0 = &Physics->points[a->pIndex];

		pushDebugSphere(DebugPrims, World * vec4(a->position, 1.0f), 0.001, vec3(1, 0, 0.5), 4);
		pushDebugLine(DebugPrims, World * vec4(p0->position, 1.0f), World * vec4(a->position, 1.0f), vec3(1, 0, 0.5));
	}
}