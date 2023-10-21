#pragma once

#include <vector>
#include "glm.h"

struct ldiCircle {
	vec3 origin;
	float radius;
	vec3 normal;
};

ldiCircle circleFit(const std::vector<vec2>& Points);