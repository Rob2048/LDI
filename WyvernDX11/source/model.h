#pragma once

#include <iostream>
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

struct ldiCoveragePointVertex {
	vec3 position;
	int id;
	vec3 normal;
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
	vec4 color;
	vec3 smoothedNormal;
	float scale;
	float viewAngle;

	vec3 axisForward;
	vec3 axisSide;
	float aspect;
};

static void convertQuadToTriModel(ldiQuadModel* Source, ldiModel* Dest) {
	Dest->verts.resize(Source->verts.size());

	for (int i = 0; i < Source->verts.size(); ++i) {
		ldiMeshVertex vert;
		vert.pos = Source->verts[i];
		
		Dest->verts[i] = vert;
	}

	int quadCount = (int)(Source->indices.size() / 4);
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

static void modelCreateFaceNormals(ldiModel* Model) {
	for (size_t iterVerts = 0; iterVerts < Model->verts.size(); ++iterVerts) {
		Model->verts[iterVerts].normal = vec3(0, 0, 0);
	}

	for (size_t iterInds = 0; iterInds < Model->indices.size(); iterInds += 3) {
		int i0 = Model->indices[iterInds + 0];
		int i1 = Model->indices[iterInds + 1];
		int i2 = Model->indices[iterInds + 2];

		ldiMeshVertex* v0 = &Model->verts[i0];
		ldiMeshVertex* v1 = &Model->verts[i1];
		ldiMeshVertex* v2 = &Model->verts[i2];

		vec3 e0 = v0->pos - v1->pos;
		vec3 e1 = v2->pos - v1->pos;
		vec3 normal = glm::cross(e0, e1);

		v0->normal += normal;
		v1->normal += normal;
		v2->normal += normal;
	}

	for (size_t iterVerts = 0; iterVerts < Model->verts.size(); ++iterVerts) {
		Model->verts[iterVerts].normal = glm::normalize(Model->verts[iterVerts].normal);
	}
}

static ldiModel modelGetTransformed(ldiModel* Model, mat4 Transform) {
	ldiModel result;
	result.indices = Model->indices;
	result.verts = Model->verts;

	for (int i = 0; i < result.verts.size(); ++i) {
		ldiMeshVertex* vert = &result.verts[i];

		vert->pos = Transform * vec4(vert->pos, 1.0f);
		vert->normal = glm::normalize(Transform * vec4(vert->normal, 0.0f));
	}

	return result;
}