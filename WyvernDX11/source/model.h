#pragma once

#include <vector>
#include "glm.h"

struct ldiMeshVertex {
	vec3 pos;
	vec3 normal;
	vec2 uv;
};

struct ldiModel {
	std::vector<ldiMeshVertex> verts;
	std::vector<uint32_t> indices;
};
