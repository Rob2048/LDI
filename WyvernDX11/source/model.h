#pragma once

#include <vector>
#include "glm.h"

struct ldiPointCloudVertex {
	vec3 position;
	vec3 normal;
	vec3 color;
};

struct ldiSimpleVertex {
	vec3 position;
	vec3 color;
};

struct ldiBasicVertex {
	vec3 position;
	vec3 color;
	vec2 uv;
};

struct ldiMeshVertex {
	vec3 pos;
	vec3 normal;
	vec2 uv;
};

struct ldiModel {
	std::vector<ldiMeshVertex> verts;
	std::vector<uint32_t> indices;
};

struct ldiPointCloud {
	std::vector<ldiPointCloudVertex> points;
};