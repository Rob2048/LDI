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

struct ldiQuadModel {
	std::vector<vec3> verts;
	std::vector<uint32_t> indices;
};

struct ldiSurfel {
	int id;
	vec3 position;
	vec3 normal;
	vec3 color;
	float scale;
};

struct ldiImage {
	int width;
	int height;
	uint8_t* data;
};

static void convertQuadToTriModel(ldiQuadModel* Source, ldiModel* Dest) {
	Dest->verts.resize(Source->verts.size());

	for (int i = 0; i < Source->verts.size(); ++i) {
		ldiMeshVertex vert;
		vert.pos = Source->verts[i];
		
		Dest->verts[i] = vert;
	}

	int quadCount = Source->indices.size() / 4;
	int triCount = quadCount * 2;
	int indCount = triCount * 3;

	Dest->indices.resize(indCount);

	for (int i = 0; i < quadCount; ++i) {
		int v0 = Source->indices[i * 4 + 0];
		int v1 = Source->indices[i * 4 + 1];
		int v2 = Source->indices[i * 4 + 2];
		int v3 = Source->indices[i * 4 + 3];

		Dest->indices[i * 6 + 0] = v3;
		Dest->indices[i * 6 + 1] = v1;
		Dest->indices[i * 6 + 2] = v0;
		Dest->indices[i * 6 + 3] = v2;
		Dest->indices[i * 6 + 4] = v1;
		Dest->indices[i * 6 + 5] = v3;
	}
}