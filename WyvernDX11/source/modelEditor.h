#pragma once

#include "voxelGrid.h"

vec3 vmin(vec3 A, vec3 B) {
	vec3 result;
	result.x = min(A.x, B.x);
	result.y = min(A.y, B.y);
	result.z = min(A.z, B.z);

	return result;
}

vec3 vmax(vec3 A, vec3 B) {
	vec3 result;
	result.x = max(A.x, B.x);
	result.y = max(A.y, B.y);
	result.z = max(A.z, B.z);

	return result;
}

struct ldiDisplacementPlane {
	ldiModel model;
	ldiRenderModel renderModel;
	int sizeX;
	int sizeY;
	float scale;
	float* map;
};

struct ldiModelEditor {
	ldiApp*						appContext;
	
	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	ldiCamera					camera;
	float						cameraSpeed = 0.5f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };

	bool						showRaymarchView = false;
	bool						showWireframe = false;

	ID3D11VertexShader*			raymarchVertexShader;
	ID3D11PixelShader*			raymarchPixelShader;
	ID3D11InputLayout*			raymarchInputLayout;

	ID3D11VertexShader*			displaceVertexShader;
	ID3D11PixelShader*			displacePixelShader;
	ID3D11InputLayout*			displaceInputLayout;

	ID3D11VertexShader*			templateVertexShader;
	ID3D11PixelShader*			templatePixelShader;
	ID3D11InputLayout*			templateInputLayout;

	ID3D11VertexShader*			targetVertexShader;
	ID3D11PixelShader*			targetPixelShader;
	ID3D11InputLayout*			targetInputLayout;

	ID3D11VertexShader*			projectionVertexShader;
	ID3D11PixelShader*			projectionPixelShader;
	ID3D11InputLayout*			projectionInputLayout;

	ldiRenderModel				fullScreenQuad;

	ID3D11Texture3D*			sdfVolumeTexture;
	ID3D11ShaderResourceView*	sdfVolumeResourceView;
	ID3D11UnorderedAccessView*	sdfVolumeUav;
	ID3D11SamplerState*			sdfVolumeSamplerState;
	ID3D11ComputeShader*		sdfVolumeCompute;

	ldiDisplacementPlane		displacePlane;
	ldiImage					decal0;

	ldiRenderModel				sphereTemplate;
	ldiRenderModel				sphereTarget1;

	ID3D11Texture2D*			dispTex;
	ID3D11ShaderResourceView*	dispView;

	ID3D11Texture2D*			dispNormTex;
	ID3D11ShaderResourceView*	dispNormView;

	ID3D11Texture2D*			dispRenderTex;
	ID3D11ShaderResourceView*	dispRenderTexView;
	ID3D11RenderTargetView*		dispRenderTexRtView;

	ID3D11Texture2D*			tempRenderTex;
	ID3D11ShaderResourceView*	tempRenderTexView;
	ID3D11RenderTargetView*		tempRenderTexRtView;
	ID3D11UnorderedAccessView*	tempRenderTexUav;

	ID3D11Texture2D*			worldPosRenderTex;
	ID3D11ShaderResourceView*	worldPosRenderTexView;
	ID3D11RenderTargetView*		worldPosRenderTexRtView;

	ID3D11Texture2D*			decal0Tex;
	ID3D11ShaderResourceView*	decal0TexView;

	ID3D11ComputeShader*		dilateComputeShader;

	ID3D11VertexShader*			tessVertexShader;
	ID3D11InputLayout*			tessInputLayout;
	ID3D11HullShader*			tessHullShader;
	ID3D11DomainShader*			tessDomainShader;

	vec3						decal0Pos = vec3(1, 1, 0);
	float						decal0Scale = 1.0f;

	float						dispBlend = 1.0f;
	float						target1Blend = 1.0f;
	float						target2Blend = 1.0f;

	ldiVoxelGrid				voxelGrid;
	ldiRenderModel				voxelRenderModel;
	ID3D11VertexShader*			voxelVertexShader;
	ID3D11PixelShader*			voxelPixelShader;
	ID3D11InputLayout*			voxelInputLayout;
	bool						showVoxelDebug = true;
};

void copyImage(ldiImage* Src, ldiDisplacementPlane* Dst, int Dx, int Dy, int Dw, int Dh) {
	ldiImage* src = Src;
	int dstW = Dw;
	int dstH = Dh;

	for (int iY = 0; iY < dstH; ++iY) {
		for (int iX = 0; iX < dstW; ++iX) {
			int srcX = iX * (float)src->width / dstW;
			int srcY = iY * (float)src->height / dstH;
			float srcP = src->data[(srcX + srcY * src->width) * 4] / 255.0f;
			srcP *= 0.5f;

			int dstIdx = (iX + Dx) + (iY + Dy) * Dst->sizeX;
			float curr = Dst->map[dstIdx];

			if (srcP > curr) {
				Dst->map[dstIdx] = srcP;
			}
		}
	}
}

// NOTE: From Christer Ericson's Real-Time Collision Detection
// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
bool getPointInTri(vec2 p, vec2 a, vec2 b, vec2 c, float& u, float& v, float& w) {
	vec2 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;

	return (u >= 0 && u <= 1) && (v >= 0 && v <= 1) && (w >= 0 && w <= 1);
}

// Faster 2D only method?
//void Barycentric(Point p, Point a, Point b, Point c, float& u, float& v, float& w)
//{
//	Vector v0 = b - a, v1 = c - a, v2 = p - a;
//	float den = v0.x * v1.y - v1.x * v0.y;
//	v = (v2.x * v1.y - v1.x * v2.y) / den;
//	w = (v0.x * v2.y - v2.x * v0.y) / den;
//	u = 1.0f - v - w;
//}

void geoBakeDisplacementTexture(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* BaseModel, ldiModel* TargetModel, ldiDispImage* OutTex) {
	float normalAdjust = -0.4;

	struct ldiTracePoint {
		int index;
		vec3 world;
		vec3 normal;
		vec2 uv;
	};

	memset(OutTex->data, 0, 4 * OutTex->width * OutTex->width);
	memset(OutTex->normalData, 0, 4 * OutTex->width * OutTex->width);

	// Create spatial structure for base model tris.

	int cellW = 128;
	double sizePerCell = 1.0 / (double)cellW;

	struct ldiSpatialCell {
		std::vector<int> tris;
	};

	ldiSpatialCell* cells = new ldiSpatialCell[cellW * cellW];

	for (size_t iterTris = 0; iterTris < BaseModel->indices.size() / 3; ++iterTris) {
		ldiMeshVertex* v0 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 0]];
		ldiMeshVertex* v1 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 1]];
		ldiMeshVertex* v2 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 2]];
		
		vec2 bMin;
		bMin.x = min(min(v0->uv.x, v1->uv.x), v2->uv.x);
		bMin.y = min(min(v0->uv.y, v1->uv.y), v2->uv.y);

		vec2 bMax;
		bMax.x = max(max(v0->uv.x, v1->uv.x), v2->uv.x);
		bMax.y = max(max(v0->uv.y, v1->uv.y), v2->uv.y);

		int cXs = max(bMin.x * cellW, 0);
		int cXe = min(bMax.x * cellW, cellW - 1);

		int cYs = max(bMin.y * cellW, 0);
		int cYe = min(bMax.y * cellW, cellW - 1);

		for (int iY = cYs; iY <= cYe; ++iY) {
			for (int iX = cXs; iX <= cXe; ++iX) {
				int cellIdx = iX + iY * cellW;

				cells[cellIdx].tris.push_back(iterTris);
			}
		}
	}

	double texelW = 1.0 / OutTex->width;
	double texelH = 1.0 / OutTex->height;

	for (int iY = 0; iY < OutTex->height; ++iY) {
		for (int iX = 0; iX < OutTex->width; ++iX) {
			float u = (iX + 0.5) * texelW;
			float v = (iY + 0.5) * texelH;
			int idx = iX + iY * OutTex->width;

			int cellX = u * cellW;
			int cellY = v * cellW;
			ldiSpatialCell* cell = &cells[cellX + cellY * cellW];

			// Find the world pos/normal for this texel. (First tri comes first).
			//for (size_t iterTris = 0; iterTris < BaseModel->indices.size() / 3; ++iterTris) {
			for (size_t iterTris = 0; iterTris < cell->tris.size(); ++iterTris) {
				/*ldiMeshVertex* v0 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 0]];
				ldiMeshVertex* v1 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 1]];
				ldiMeshVertex* v2 = &BaseModel->verts[BaseModel->indices[iterTris * 3 + 2]];*/
				ldiMeshVertex* v0 = &BaseModel->verts[BaseModel->indices[cell->tris[iterTris] * 3 + 0]];
				ldiMeshVertex* v1 = &BaseModel->verts[BaseModel->indices[cell->tris[iterTris] * 3 + 1]];
				ldiMeshVertex* v2 = &BaseModel->verts[BaseModel->indices[cell->tris[iterTris] * 3 + 2]];

				float tU, tV, tW;

				if (getPointInTri(vec2(u, v), v0->uv, v1->uv, v2->uv, tU, tV, tW)) {
					ldiTracePoint tracePoint = {};
					tracePoint.index = idx;
					tracePoint.uv = vec2(u, v);
					tracePoint.world = v0->pos * tU + v1->pos * tV + v2->pos * tW;
					tracePoint.normal = v0->normal * tU + v1->normal * tV + v2->normal * tW;

					ldiRaycastResult result = physicsRaycast(CookedMesh, tracePoint.world + tracePoint.normal * normalAdjust, tracePoint.normal, 1.0f);

					if (result.hit) {
						ldiMeshVertex v0 = TargetModel->verts[TargetModel->indices[result.faceIdx * 3 + 0]];
						ldiMeshVertex v1 = TargetModel->verts[TargetModel->indices[result.faceIdx * 3 + 1]];
						ldiMeshVertex v2 = TargetModel->verts[TargetModel->indices[result.faceIdx * 3 + 2]];

						float u = result.barry.x;
						float v = result.barry.y;
						float w = 1.0 - (u + v);
						vec3 normal = w * v0.normal + u * v1.normal + v * v2.normal;

						//vert->pos = result.pos;
						//vert->normal = normal;//-result.normal;

						//float dist = glm::length(tracePoint.world - result.pos);
						float dist = result.dist - glm::length(tracePoint.normal * normalAdjust);

						OutTex->normalData[idx * 4 + 0] = (normal.x * 0.5 + 0.5) * 255;
						OutTex->normalData[idx * 4 + 1] = (normal.y * 0.5 + 0.5) * 255;
						OutTex->normalData[idx * 4 + 2] = (normal.z * 0.5 + 0.5) * 255;
						OutTex->normalData[idx * 4 + 3] = 255;

						OutTex->data[idx] = dist;

						/*OutTex->data[idx * 4 + 0] = 127 + dist * 127;
						OutTex->data[idx * 4 + 1] = 127 + dist * 127;
						OutTex->data[idx * 4 + 2] = 127 + dist * 127;
						OutTex->data[idx * 4 + 3] = 255;*/
					}

					/*OutTex->data[idx * 4 + 0] = tracePoint.world.x * 100;
					OutTex->data[idx * 4 + 1] = tracePoint.world.y * 100;
					OutTex->data[idx * 4 + 2] = tracePoint.world.z * 100;*/
					/*OutTex->data[idx * 4 + 0] = (tracePoint.normal.x * 0.5 + 0.5) * 255;
					OutTex->data[idx * 4 + 1] = (tracePoint.normal.y * 0.5 + 0.5) * 255;
					OutTex->data[idx * 4 + 2] = (tracePoint.normal.z * 0.5 + 0.5) * 255;
					OutTex->data[idx * 4 + 3] = 255;*/

					//tracePoints.push_back(tracePoint);
					break;
				}
			}
		}
	}

	delete[] cells;
}

void geoBakeDisplacement(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiModel* DstModel) {
	float normalAdjust = -0.4;
	
	for (size_t i = 0; i < SrcModel->verts.size(); ++i) {
		ldiMeshVertex* vert = &(SrcModel->verts)[i];
		ldiRaycastResult result = physicsRaycast(CookedMesh, vert->pos + vert->normal * normalAdjust, vert->normal, 1.0f);

		if (result.hit) {
			ldiMeshVertex v0 = DstModel->verts[DstModel->indices[result.faceIdx * 3 + 0]];
			ldiMeshVertex v1 = DstModel->verts[DstModel->indices[result.faceIdx * 3 + 1]];
			ldiMeshVertex v2 = DstModel->verts[DstModel->indices[result.faceIdx * 3 + 2]];

			float u = result.barry.x;
			float v = result.barry.y;
			float w = 1.0 - (u + v);
			vec3 normal = w * v0.normal + u * v1.normal + v * v2.normal;

			vert->pos = result.pos;
			vert->normal = normal;//-result.normal;
		}
	}
}

int modelEditorInit(ldiApp* AppContext, ldiModelEditor* Tool) {
	Tool->appContext = AppContext;
	Tool->mainViewWidth = 0;
	Tool->mainViewHeight = 0;
	
	Tool->camera = {};
	Tool->camera.position = vec3(6, 6, 6);
	Tool->camera.rotation = vec3(-45, 30, 0);

	return 0;
}

int modelEditorLoad(ldiApp* AppContext, ldiModelEditor* Tool) {

	std::cout << "Start shader compile\n";

	D3D11_INPUT_ELEMENT_DESC basicLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC meshLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	//----------------------------------------------------------------------------------------------------
	// Raymarch shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/raymarch.hlsl", "mainVs", &Tool->raymarchVertexShader, basicLayout, 3, &Tool->raymarchInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/raymarch.hlsl", "mainPs", &Tool->raymarchPixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// SDF generate compute shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/generateSdf.hlsl", "mainCs", &Tool->sdfVolumeCompute)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Dilate generate compute shader.s
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/dilate.hlsl", "mainCs", &Tool->dilateComputeShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Displace shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/displace.hlsl", "mainVs", &Tool->displaceVertexShader, meshLayout, 3, &Tool->displaceInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/displace.hlsl", "mainPs", &Tool->displacePixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Template shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/template.hlsl", "mainVs", &Tool->templateVertexShader, meshLayout, 3, &Tool->templateInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/template.hlsl", "mainPs", &Tool->templatePixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Target shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/target.hlsl", "mainVs", &Tool->targetVertexShader, meshLayout, 3, &Tool->targetInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/target.hlsl", "mainPs", &Tool->targetPixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Projection shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/projection.hlsl", "mainVs", &Tool->projectionVertexShader, meshLayout, 3, &Tool->projectionInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/projection.hlsl", "mainPs", &Tool->projectionPixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Projection shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/voxel.hlsl", "mainVs", &Tool->voxelVertexShader, meshLayout, 3, &Tool->voxelInputLayout)) {
		return 1;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/voxel.hlsl", "mainPs", &Tool->voxelPixelShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Tesselation shaders.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/hull.hlsl", "mainVs", &Tool->tessVertexShader, meshLayout, 3, &Tool->tessInputLayout)) {
		return 1;
	}

	if (!gfxCreateHullShader(AppContext, L"../../assets/shaders/hull.hlsl", "mainHs", &Tool->tessHullShader)) {
		return 1;
	}

	if (!gfxCreateDomainShader(AppContext, L"../../assets/shaders/hull.hlsl", "mainDs", &Tool->tessDomainShader)) {
		return 1;
	}

	//----------------------------------------------------------------------------------------------------
	// Fullscreen quad for raymarcher.
	//----------------------------------------------------------------------------------------------------
	{
		std::vector<ldiBasicVertex> verts;
		std::vector<uint32_t> indices;

		verts.push_back({ {-1, -1, 1}, {0, 0, 0}, {0, 0} });
		verts.push_back({ {1, -1, 1}, {0, 0, 0}, {1, 0} });
		verts.push_back({ {1, 1, 1}, {0, 0, 0}, {1, 1} });
		verts.push_back({ {-1, 1, 1}, {0, 0, 0}, {0, 1} });

		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(0);
		indices.push_back(3);
		indices.push_back(2);
		indices.push_back(0);

		Tool->fullScreenQuad = gfxCreateRenderModelBasic(Tool->appContext, &verts, &indices);
	}

	//----------------------------------------------------------------------------------------------------
	// SDF volume.
	//----------------------------------------------------------------------------------------------------
	int sdfWidth = 256;
	int sdfHeight = 256;
	int sdfDepth = 256;

	{
		D3D11_TEXTURE3D_DESC desc = {};
		desc.Width = sdfWidth;
		desc.Height = sdfHeight;
		desc.Depth = sdfDepth;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		float* sdfVolume = new float[sdfWidth * sdfHeight * sdfDepth];

		// vec3 sphereOrigin(sdfWidth / 2.0f, sdfHeight / 2.0f, sdfDepth / 2.0f);
		// float sphereRadius = 128.0f;

		// for (int iZ = 0; iZ < sdfDepth; ++iZ) {
		// 	for (int iY = 0; iY < sdfHeight; ++iY) {
		// 		for (int iX = 0; iX < sdfWidth; ++iX) {
		// 			int idx = iX + iY * sdfWidth + iZ * sdfWidth * sdfHeight;

		// 			vec3 pos(iX + 0.5f, iY + 0.5f, iZ + 0.5f);
		// 			float dist = glm::length(pos - sphereOrigin) - sphereRadius;

		// 			sdfVolume[idx] = dist;
		// 		}
		// 	}
		// }

		// FILE* sdfFile;
		// fopen_s(&sdfFile, "../../assets/models/derg256.sdf", "rb");

		// fread(sdfVolume, 4, sdfWidth * sdfHeight * sdfDepth, sdfFile);

		// fclose(sdfFile);

		D3D11_SUBRESOURCE_DATA sub = {};
		sub.pSysMem = sdfVolume;
		sub.SysMemPitch = sdfWidth * 4;
		sub.SysMemSlicePitch = sdfWidth * sdfHeight * 4;

		if (AppContext->d3dDevice->CreateTexture3D(&desc, &sub, &Tool->sdfVolumeTexture) != S_OK) {
			std::cout << "Texture failed to create\n";
			return 1;
		}

		delete[] sdfVolume;

		if (AppContext->d3dDevice->CreateShaderResourceView(Tool->sdfVolumeTexture, NULL, &Tool->sdfVolumeResourceView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return 1;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &Tool->sdfVolumeSamplerState);
	}

	//----------------------------------------------------------------------------------------------------
	// Create SDF of triangle mesh.
	//----------------------------------------------------------------------------------------------------
	{
		if (!gfxCreateTexture3DUav(AppContext, Tool->sdfVolumeTexture, &Tool->sdfVolumeUav)) {
			std::cout << "Could not create UAV\n";
			return 1;
		}

		ldiModel dergnModel = objLoadModel("../../assets/models/dergn.obj");
		// ldiModel dergnModel = objLoadModel("../../assets/models/test.obj");

		float scale = 24;
		for (int i = 0; i < dergnModel.verts.size(); ++i) {
			ldiMeshVertex* vert = &dergnModel.verts[i];
			vert->pos = vert->pos * scale;
			vert->pos += vec3(128.0f, 0.0f, 128.0f);
		}

		// Create tri list.
		std::vector<vec3> tris;
		tris.reserve(dergnModel.indices.size());

		for (size_t i = 0; i < dergnModel.indices.size(); ++i) {
			tris.push_back(dergnModel.verts[dergnModel.indices[i]].pos);
		}

		ID3D11Buffer* triListBuffer;

		if (!gfxCreateStructuredBuffer(AppContext, 4 * 3, tris.size(), tris.data(), &triListBuffer)) {
			std::cout << "Could not create structured buffer\n";
			return 1;
		}

		ID3D11ShaderResourceView* triListResourceView;

		if (!gfxCreateBufferShaderResourceView(AppContext, triListBuffer, &triListResourceView)) {
			std::cout << "Could not create shader resource view\n";
			return 1;
		}

		ID3D11Buffer* constantBuffer;

		struct ldiSdfComputeConstantBuffer {
			int triCount;
			float testVal;
			float __p0;
			float __p1;
		};

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(ldiSdfComputeConstantBuffer);
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			ldiSdfComputeConstantBuffer cbTemp;
			std::cout << "Tris size: " << tris.size() << " " << tris.size() / 3 << "\n";
			cbTemp.triCount = tris.size() / 3;
			cbTemp.testVal = 0.0f;

			D3D11_SUBRESOURCE_DATA constantBufferData = {};
			constantBufferData.pSysMem = &cbTemp;
			
			if (AppContext->d3dDevice->CreateBuffer(&desc, &constantBufferData, &constantBuffer) != S_OK) {
				std::cout << "Failed to create constant buffer\n";
				return 1;
			}
		}

		double t0 = getTime();

		D3D11_QUERY_DESC queryDesc = {};
		queryDesc.Query = D3D11_QUERY_EVENT;

		ID3D11Query* query;
		if (AppContext->d3dDevice->CreateQuery(&queryDesc, &query) != S_OK) {
			return false;
		}

		AppContext->d3dDeviceContext->CSSetShader(Tool->sdfVolumeCompute, 0, 0);
		AppContext->d3dDeviceContext->CSSetShaderResources(0, 1, &triListResourceView);
		AppContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &Tool->sdfVolumeUav, 0);
		AppContext->d3dDeviceContext->CSSetConstantBuffers(0, 1, &constantBuffer);
		AppContext->d3dDeviceContext->Dispatch(sdfWidth / 8, sdfHeight / 8, sdfDepth / 8);
		AppContext->d3dDeviceContext->CSSetShader(0, 0, 0);
		ID3D11UnorderedAccessView* ppUAViewnullptr[1] = { 0 };
		AppContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUAViewnullptr, 0);

		AppContext->d3dDeviceContext->End(query);
		BOOL queryData;

		while (S_OK != AppContext->d3dDeviceContext->GetData(query, &queryData, sizeof(BOOL), 0)) {
		}

		if (queryData != TRUE) {
			return false;
		}

		t0 = getTime() - t0;
		std::cout << "SDF compute time: " << (t0 * 1000.0) << " ms\n";
	}

	//----------------------------------------------------------------------------------------------------
	// Create displacement grid.
	//----------------------------------------------------------------------------------------------------
	{
		Tool->displacePlane.sizeX = 1000;
		Tool->displacePlane.sizeY = 1000;
		Tool->displacePlane.scale = 0.01f;
		Tool->displacePlane.map = new float[Tool->displacePlane.sizeX * Tool->displacePlane.sizeY];
		
		ldiModel* plane = &Tool->displacePlane.model;
		int sizeX = Tool->displacePlane.sizeX;
		int sizeY = Tool->displacePlane.sizeY;
		float scale = Tool->displacePlane.scale;
		
		for (int iY = 0; iY < sizeY; ++iY) {
			for (int iX = 0; iX < sizeX; ++iX) {
				plane->verts.push_back({ {iX * scale, 0, iY * scale}, {0, 1, 0}, {iX / scale, iY / scale} });
			}
		}

		int rootVert = 0;

		for (int iY = 0; iY < sizeY - 1; ++iY) {
			for (int iX = 0; iX < sizeX - 1; ++iX) {
				int v0 = rootVert;
				int v1 = v0 + 1;
				int v2 = rootVert + sizeX;
				int v3 = v2 + 1;

				plane->indices.push_back(v0);
				plane->indices.push_back(v1);
				plane->indices.push_back(v2);
				plane->indices.push_back(v2);
				plane->indices.push_back(v1);
				plane->indices.push_back(v3);

				++rootVert;
			}

			++rootVert;
		}
		
		Tool->displacePlane.renderModel = gfxCreateRenderModelMutableVerts(Tool->appContext, &Tool->displacePlane.model);

		int c = 4;
		Tool->decal0.data = imageLoadRgba("../../assets/images/disp/dergscale.png", &Tool->decal0.width, &Tool->decal0.height, &c);

		{
			D3D11_SUBRESOURCE_DATA texData = {};
			texData.pSysMem = Tool->decal0.data;
			texData.SysMemPitch = Tool->decal0.width * 4;

			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = Tool->decal0.width;
			tex2dDesc.Height = Tool->decal0.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &Tool->decal0Tex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->decal0Tex, NULL, &Tool->decal0TexView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Load displacment meshes.
	//----------------------------------------------------------------------------------------------------
	{
		//ldiModel sphereTemplateModel = objLoadQuadModel("../../assets/models/sphere_templateMed.obj");
		//Tool->sphereTemplate = gfxCreateRenderQuadModel(AppContext, &sphereTemplateModel);

		ldiModel sphereTemplateModel = objLoadModel("../../assets/models/sphere_templateUVsMed.obj");
		Tool->sphereTemplate = gfxCreateRenderModel(AppContext, &sphereTemplateModel);

		ldiModel sphereTarget1 = objLoadModel("../../assets/models/sphere_target1.obj");
		Tool->sphereTarget1 = gfxCreateRenderModel(AppContext, &sphereTarget1);

		ldiPhysicsMesh sphereTarget1Phys;
		if (physicsCookMesh(AppContext->physics, &sphereTarget1, &sphereTarget1Phys) != 0) {
			return 1;
		}

		double t0 = getTime();

		ldiDispImage dispTex = {};
		dispTex.width = 2048;
		dispTex.height = 2048;
		dispTex.data = new float[dispTex.width * dispTex.height];
		dispTex.normalData = new uint8_t[dispTex.width * dispTex.height * 4];
		
		geoBakeDisplacementTexture(Tool->appContext, &sphereTarget1Phys, &sphereTemplateModel, &sphereTarget1, &dispTex);
		t0 = getTime() - t0;
		std::cout << "Bake time: " << (t0 * 1000.0) << " ms\n";

		{
			D3D11_SUBRESOURCE_DATA texData = {};
			texData.pSysMem = dispTex.data;
			texData.SysMemPitch = dispTex.width * 4;

			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = dispTex.width;
			tex2dDesc.Height = dispTex.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R32_FLOAT;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &Tool->dispTex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}
		
			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->dispTex, NULL, &Tool->dispView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}

		{
			D3D11_SUBRESOURCE_DATA texData = {};
			texData.pSysMem = dispTex.normalData;
			texData.SysMemPitch = dispTex.width * 4;

			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = dispTex.width;
			tex2dDesc.Height = dispTex.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &Tool->dispNormTex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->dispNormTex, NULL, &Tool->dispNormView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}

		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = dispTex.width;
			tex2dDesc.Height = dispTex.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			// tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tex2dDesc.Format = DXGI_FORMAT_R32_FLOAT;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->dispRenderTex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}

			if (AppContext->d3dDevice->CreateRenderTargetView(Tool->dispRenderTex, NULL, &Tool->dispRenderTexRtView) != S_OK) {
				std::cout << "CreateRenderTargetView failed\n";
				return 1;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->dispRenderTex, NULL, &Tool->dispRenderTexView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}

			//AppContext->d3dDeviceContext->CopyResource(Tool->dispRenderTex, Tool->dispTex);
		}

		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = dispTex.width;
			tex2dDesc.Height = dispTex.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->worldPosRenderTex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}

			if (AppContext->d3dDevice->CreateRenderTargetView(Tool->worldPosRenderTex, NULL, &Tool->worldPosRenderTexRtView) != S_OK) {
				std::cout << "CreateRenderTargetView failed\n";
				return 1;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->worldPosRenderTex, NULL, &Tool->worldPosRenderTexView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}

		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.Width = dispTex.width;
			tex2dDesc.Height = dispTex.height;
			tex2dDesc.MipLevels = 1;
			tex2dDesc.ArraySize = 1;
			tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Tool->tempRenderTex))) {
				std::cout << "Texture failed to create\n";
				return false;
			}

			if (AppContext->d3dDevice->CreateRenderTargetView(Tool->tempRenderTex, NULL, &Tool->tempRenderTexRtView) != S_OK) {
				std::cout << "CreateRenderTargetView failed\n";
				return 1;
			}

			if (AppContext->d3dDevice->CreateShaderResourceView(Tool->tempRenderTex, NULL, &Tool->tempRenderTexView) != S_OK) {
				std::cout << "CreateShaderResourceView failed\n";
				return 1;
			}
		}

		{
			if (!gfxCreateTexture2DUav(AppContext, Tool->tempRenderTex, &Tool->tempRenderTexUav)) {
				return 1;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Voxel stuff.
	//----------------------------------------------------------------------------------------------------
	{
		std::cout << "Creating voxel grid\n";

		Tool->voxelGrid = voxelCreateGrid(32, 32, 32);

		double t0 = getTime();

		float vgHsX = Tool->voxelGrid.cellSizeX * 0.5f;
		float vgHsY = Tool->voxelGrid.cellSizeY * 0.5f;
		float vgHsZ = Tool->voxelGrid.cellSizeZ * 0.5f;

		for (int iZ = 0; iZ < Tool->voxelGrid.cellSizeZ; ++iZ) {
			for (int iY = 0; iY < Tool->voxelGrid.cellSizeY; ++iY) {
				for (int iX = 0; iX < Tool->voxelGrid.cellSizeX; ++iX) {
					vec3 pos;
					pos.x = (iX + 0.5f);
					pos.y = (iY + 0.5f);
					pos.z = (iZ + 0.5f);

					float radius = 40.0f;
					float dist = glm::length(pos - vec3(vgHsX, vgHsY + 30, vgHsZ)) - radius;

					float dist2 = glm::length(pos - vec3(vgHsX - 20, vgHsY - 20, vgHsZ - 15)) - 30;

					vec3 q = abs(pos - vec3(vgHsX, vgHsY - 50, vgHsZ)) - vec3(30, 30, 30);
					float dist3 = glm::length(vmax(q, vec3(0.0f))) + min(max(q.x, max(q.y, q.z)), 0.0) - 4;

					vec3 d = abs(pos - vec3(vgHsX, vgHsY - 50, vgHsZ)) - vec3(30, 30, 30);
					float dist4 = min(max(d.x, max(d.y, d.z)), 0.0) + glm::length(vmax(d, vec3(0.0f)));

					//dist = min(dist, dist4);

					float k = 40.0;
					float h = clampf(0.5 + 0.5 * (dist3 - dist) / k, 0.0, 1.0);
					dist = lerp(dist3, dist, h) - k * h * (1.0 - h);

					dist = min(dist, dist2);

					if (dist < 1.0f) {
						//dist += 1.0f;
						//dist = max(0, dist);
						//dist = 1.0f - dist;

						ldiVoxelCell voxel;
						voxel.value = dist;
						voxelSetCell(&Tool->voxelGrid, iX, iY, iZ, voxel);
					}

					/*if (dist < 0.0f) {
						dist += 1.0f;
						dist = max(0, dist);
						dist = 1.0f - dist;

						ldiVoxelCell voxel;
						voxel.value = dist;

						voxelSetCell(&Tool->voxelGrid, iX, iY, iZ, voxel);
					}*/
				}
			}
		}
		t0 = getTime() - t0;
		std::cout << "Voxel SDF time: " << (t0 * 1000.0) << " ms\n";

		ldiModel voxelModel;
		t0 = getTime();
		//voxelCreateModel(&Tool->voxelGrid, &voxelModel);
		voxelMarch(&Tool->voxelGrid, &voxelModel);
		t0 = getTime() - t0;
		std::cout << "Voxel build mesh time: " << (t0 * 1000.0) << " ms\n";

		t0 = getTime();
		modelCreateFaceNormals(&voxelModel);
		t0 = getTime() - t0;
		std::cout << "Voxel mesh calculate normals time: " << (t0 * 1000.0) << " ms\n";

		Tool->voxelRenderModel = gfxCreateRenderModel(AppContext, &voxelModel);

		std::cout << "Voxel grid size: " << Tool->voxelGrid.cellSizeX << ", " << Tool->voxelGrid.cellSizeY << ", " << Tool->voxelGrid.cellSizeZ << " (" << 
			Tool->voxelGrid.cellSizeX * Tool->voxelGrid.cellSizeY * Tool->voxelGrid.cellSizeZ << ")\n";

		std::cout << "Voxel grid index size: " << (Tool->voxelGrid.sizeX * Tool->voxelGrid.sizeY * Tool->voxelGrid.sizeZ * sizeof(int)) / 1024.0 << " KB\n";

		std::cout << "Voxel allocated chunks: " << Tool->voxelGrid.rawChunks.size() << " (" << (Tool->voxelGrid.rawChunks.size() * sizeof(ldiVoxelChunk)) / (1024.0 * 1024.0) << " MB)\n";

		std::cout << "Voxel mesh - verts: " << voxelModel.verts.size() << " tris: " << voxelModel.indices.size() / 3 << "\n";
	}

	return 0;
}

void renderFrustumDebug(ldiApp* AppContext, mat4 ProjViewMat) {
	mat4 invProjViewMat = glm::inverse(ProjViewMat);

	vec4 f0 = invProjViewMat * vec4(-1, -1, 0, 1); f0 /= f0.w;
	vec4 f1 = invProjViewMat * vec4(1, -1, 0, 1); f1 /= f1.w;
	vec4 f2 = invProjViewMat * vec4(1, 1, 0, 1); f2 /= f2.w;
	vec4 f3 = invProjViewMat * vec4(-1, 1, 0, 1); f3 /= f3.w;

	vec4 f4 = invProjViewMat * vec4(-1, -1, 1, 1); f4 /= f4.w;
	vec4 f5 = invProjViewMat * vec4(1, -1, 1, 1); f5 /= f5.w;
	vec4 f6 = invProjViewMat * vec4(1, 1, 1, 1); f6 /= f6.w;
	vec4 f7 = invProjViewMat * vec4(-1, 1, 1, 1); f7 /= f7.w;

	pushDebugLine(&AppContext->defaultDebug, f0, f4, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f1, f5, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f2, f6, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f3, f7, vec3(1, 0, 1));

	pushDebugLine(&AppContext->defaultDebug, f0, f1, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f1, f2, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f2, f3, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f3, f0, vec3(1, 0, 1));

	pushDebugLine(&AppContext->defaultDebug, f4, f5, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f5, f6, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f6, f7, vec3(1, 0, 1));
	pushDebugLine(&AppContext->defaultDebug, f7, f4, vec3(1, 0, 1));
}

void modelEditorRender(ldiModelEditor* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->renderViewBuffers, Width, Height);
	}

	// Projector setup.
	mat4 decal0PVMat;

	{
		double time = getTime();
		//mat4 decalViewMat = glm::lookAtRH(vec3(sin(time), 1, cos(time)), vec3(0, 0, 0), vec3(0, 1, 0));
		vec3 d0p = glm::normalize(Tool->decal0Pos);
		mat4 decalViewMat = glm::lookAtRH(d0p * 1.5f, vec3(0, 0, 0), vec3(0, 1, 0));
		mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(25.0f * Tool->decal0Scale), 1.0f, 1.0f, 0.01f, 1.5f);
		decal0PVMat = projMat * decalViewMat;
	}

	//----------------------------------------------------------------------------------------------------
	// Project to displacement map.
	//----------------------------------------------------------------------------------------------------
	{
		ID3D11RenderTargetView* renderTargets[] = {
			Tool->dispRenderTexRtView,
			Tool->worldPosRenderTexRtView
		};

		appContext->d3dDeviceContext->OMSetRenderTargets(2, renderTargets, NULL);

		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = 2048;
		viewport.Height = 2048;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
		vec4 bkgColor(0, 0, 0, 0);
		appContext->d3dDeviceContext->ClearRenderTargetView(Tool->dispRenderTexRtView, (float*)&bkgColor);
		appContext->d3dDeviceContext->ClearRenderTargetView(Tool->worldPosRenderTexRtView, (float*)&bkgColor);
		//appContext->d3dDeviceContext->ClearDepthStencilView(Tool->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		ID3D11ShaderResourceView* srvs[] = {
			Tool->dispView,
			Tool->decal0TexView
		};

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->screenSize.x = getTime();
		constantBuffer->screenSize.y = Tool->target1Blend;
		constantBuffer->screenSize.z = Tool->target2Blend;
		constantBuffer->proj = decal0PVMat;
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		appContext->d3dDeviceContext->PSSetShaderResources(0, 2, srvs);

		ID3D11SamplerState* samplers[] = {
			appContext->defaultLinearSamplerState,
			appContext->wrapLinearSamplerState
		};
		appContext->d3dDeviceContext->PSSetSamplers(0, 2, samplers);

		gfxRenderModel(appContext, &Tool->sphereTemplate, appContext->doubleSidedRasterizerState, Tool->projectionVertexShader, Tool->projectionPixelShader, Tool->projectionInputLayout);

		appContext->d3dDeviceContext->OMSetRenderTargets(0, NULL, NULL);

		appContext->d3dDeviceContext->CSSetShader(Tool->dilateComputeShader, 0, 0);
		appContext->d3dDeviceContext->CSSetSamplers(0, 1, &appContext->defaultLinearSamplerState);
		
		ID3D11ShaderResourceView* csSrvs[] = {
			Tool->dispRenderTexView,
			Tool->worldPosRenderTexView
		};

		appContext->d3dDeviceContext->CSSetShaderResources(0, 2, csSrvs);

		appContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &Tool->tempRenderTexUav, 0);
		//AppContext->d3dDeviceContext->CSSetConstantBuffers(0, 1, &constantBuffer);
		appContext->d3dDeviceContext->Dispatch(2048 / 8, 2048 / 8, 1);

		appContext->d3dDeviceContext->CSSetShader(0, 0, 0);
		ID3D11UnorderedAccessView* nullUav[] = { 0 };
		appContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUav, 0);
		ID3D11ShaderResourceView* nullSrv[] = { 0, 0 };
		appContext->d3dDeviceContext->CSSetShaderResources(0, 2, nullSrv);
	}

	//----------------------------------------------------------------------------------------------------
	// Initial debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(&appContext->defaultDebug);
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	pushDebugLine(&appContext->defaultDebug, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

	/*pushDebugLine(appContext, vec3(3, 0, 0), vec3(3, 5, 0), vec3(0.25, 0, 0.25));
	pushDebugLine(appContext, vec3(3, 5, 0), vec3(3, 10, 0), vec3(0.5, 0, 0.5));
	pushDebugLine(appContext, vec3(3, 10, 0), vec3(3, 12, 0), vec3(0.75, 0, 0.75));
	pushDebugLine(appContext, vec3(3, 12, 0), vec3(3, 15, 0), vec3(1, 0, 1));*/
	
	//pushDebugBoxSolid(appContext, vec3(0, 0, 1), vec3(2, 2, 2), vec3(0.5, 0, 0.5));

	renderFrustumDebug(appContext, decal0PVMat);

	// Grid.
	int gridCount = 10;
	float gridCellWidth = 1.0f;
	vec3 gridColor = Tool->gridColor;
	vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
	for (int i = 0; i < gridCount + 1; ++i) {
		pushDebugLine(&appContext->defaultDebug, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
		pushDebugLine(&appContext->defaultDebug, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Tool->camera.rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Tool->camera.rotation.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -Tool->camera.position);

	mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)Tool->mainViewWidth, (float)Tool->mainViewHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->renderViewBuffers.mainViewRtView, Tool->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->mainViewWidth;
	viewport.Height = Tool->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->renderViewBuffers.mainViewRtView, (float*)&Tool->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//----------------------------------------------------------------------------------------------------
	// Render models.
	//----------------------------------------------------------------------------------------------------
	if (Tool->showRaymarchView) {
		pushDebugBox(&appContext->defaultDebug, vec3(0, 64 / 20.0f, 0), vec3(128 / 20.0f, 128 / 20.0f, 128 / 20.0f), vec3(1, 0, 1));

		mat4 projViewMat = projMat * camViewMat;
		mat4 invProjViewMat = glm::inverse(projViewMat);

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = invProjViewMat;
		constantBuffer->proj = projViewMat;
		constantBuffer->color = vec4(0, 0, 0, getTime());
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		appContext->d3dDeviceContext->PSSetShaderResources(0, 1, &Tool->sdfVolumeResourceView);
		appContext->d3dDeviceContext->PSSetSamplers(0, 1, &Tool->sdfVolumeSamplerState);

		gfxRenderBasicModel(appContext, &Tool->fullScreenQuad, Tool->raymarchInputLayout, Tool->raymarchVertexShader, Tool->raymarchPixelShader, appContext->rayMarchDepthStencilState);
	}

	//----------------------------------------------------------------------------------------------------
	// Sphere template.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->screenSize.x = Tool->dispBlend;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		
		UINT lgStride = sizeof(ldiMeshVertex);
		UINT lgOffset = 0;
		ldiRenderModel* model = &Tool->sphereTemplate;

		appContext->d3dDeviceContext->IASetInputLayout(Tool->tessInputLayout);
		appContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &model->vertexBuffer, &lgStride, &lgOffset);
		appContext->d3dDeviceContext->IASetIndexBuffer(model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		//appContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		appContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		
		appContext->d3dDeviceContext->VSSetShader(Tool->tessVertexShader, 0, 0);
		//appContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		//appContext->d3dDeviceContext->VSSetShaderResources(0, 1, &Tool->tempRenderTexView);
		//appContext->d3dDeviceContext->VSSetSamplers(0, 1, &appContext->defaultLinearSamplerState);
		
		appContext->d3dDeviceContext->PSSetShader(Tool->templatePixelShader, 0, 0);
		appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		ID3D11ShaderResourceView* views[] = {
			Tool->tempRenderTexView,
			Tool->dispNormView,
		};
		appContext->d3dDeviceContext->PSSetShaderResources(0, 2, views);
		appContext->d3dDeviceContext->PSSetSamplers(0, 1, &appContext->defaultLinearSamplerState);

		appContext->d3dDeviceContext->HSSetShader(Tool->tessHullShader, NULL, 0);
		appContext->d3dDeviceContext->DSSetShader(Tool->tessDomainShader, NULL, 0);
		appContext->d3dDeviceContext->DSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->DSSetShaderResources(0, 1, &Tool->tempRenderTexView);
		appContext->d3dDeviceContext->DSSetSamplers(0, 1, &appContext->defaultLinearSamplerState);

		appContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		
		appContext->d3dDeviceContext->OMSetBlendState(appContext->defaultBlendState, NULL, 0xffffffff);
		appContext->d3dDeviceContext->RSSetState(appContext->defaultRasterizerState);
		appContext->d3dDeviceContext->OMSetDepthStencilState(appContext->defaultDepthStencilState, 0);
		
		appContext->d3dDeviceContext->DrawIndexed(model->indexCount, 0, 0);

		if (Tool->showWireframe) {
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->screenSize.x = Tool->dispBlend;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
			appContext->d3dDeviceContext->RSSetState(appContext->wireframeRasterizerState);
			appContext->d3dDeviceContext->DrawIndexed(model->indexCount, 0, 0);
		}
		
		appContext->d3dDeviceContext->HSSetShader(NULL, NULL, 0);
		appContext->d3dDeviceContext->DSSetShader(NULL, NULL, 0);

		//gfxRenderModel(appContext, &Tool->sphereTemplate, appContext->defaultRasterizerState, Tool->templateVertexShader, Tool->templatePixelShader, Tool->templateInputLayout);
		
		/*if (Tool->showWireframe) {
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->screenSize.x = Tool->dispBlend;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

			gfxRenderModel(appContext, &Tool->sphereTemplate, appContext->wireframeRasterizerState, Tool->templateVertexShader, Tool->templatePixelShader, Tool->templateInputLayout);
		}*/
	}

	//----------------------------------------------------------------------------------------------------
	// Sphere target1.
	//----------------------------------------------------------------------------------------------------
	{
		mat4 pvm = projMat * camViewMat * glm::translate(mat4(1.0f), vec3(0, 0, 3));

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = pvm;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderModel(appContext, &Tool->sphereTarget1, appContext->defaultRasterizerState, Tool->targetVertexShader, Tool->targetPixelShader, Tool->targetInputLayout);

		if (Tool->showWireframe) {
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = pvm;
			constantBuffer->color = vec4(0, 0, 0, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

			gfxRenderModel(appContext, &Tool->sphereTarget1, appContext->wireframeRasterizerState, Tool->targetVertexShader, Tool->targetPixelShader, Tool->targetInputLayout);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Voxel grid.
	//----------------------------------------------------------------------------------------------------
	{
		if (Tool->showVoxelDebug) {
			voxelRenderDebug(Tool->appContext, &Tool->voxelGrid);
		}

		D3D11_MAPPED_SUBRESOURCE ms;
		appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
		constantBuffer->mvp = projViewModelMat;
		constantBuffer->color = vec4(1, 1, 1, 1);
		appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

		gfxRenderModel(appContext, &Tool->voxelRenderModel, appContext->defaultRasterizerState, Tool->voxelVertexShader, Tool->voxelPixelShader, Tool->voxelInputLayout);

		if (Tool->showWireframe) {
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(1, 1, 1, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

			gfxRenderModel(appContext, &Tool->voxelRenderModel, true);
		}
	}


	// Update displace plane.
	{
		//ldiDisplacementPlane* plane = &Tool->displacePlane;
		//double time = getTime();

		//for (int iY = 0; iY < plane->sizeY; ++iY) {
		//	for (int iX = 0; iX < plane->sizeX; ++iX) {
		//		//plane->map[iX + (iY * plane->sizeX)] = sin(time + iX * 0.1) * 0.1 + cos(time + iY * 0.1) * 0.1;
		//		plane->map[iX + (iY * plane->sizeX)] = 0;
		//	}
		//}

		//// Copy image.
		//{
		//	copyImage(&Tool->decal0, plane, 0, 0, 512, 512);
		//	copyImage(&Tool->decal0, plane, 32, 32, 512, 512);
		//}

		//for (int iY = 1; iY < plane->sizeY - 1; ++iY) {
		//	for (int iX = 1; iX < plane->sizeX - 1; ++iX) {
		//		float dL = plane->map[(iX - 1) + ((iY + 0) * plane->sizeX)];
		//		float dR = plane->map[(iX + 1) + ((iY + 0) * plane->sizeX)];
		//		float dT = plane->map[(iX + 0) + ((iY - 1) * plane->sizeX)];
		//		float dB = plane->map[(iX + 0) + ((iY + 1) * plane->sizeX)];


		//		vec3 normal;
		//		normal.x = dL - dR;
		//		normal.y = 2 * plane->scale;
		//		normal.z = dT - dB;
		//		//normal = normal * 0.25f;
		//		normal = glm::normalize(normal);

		//		//normal = glm::normalize(vec3(dL - dR, 2, dT - dB));

		//		/*vec3 va = glm::normalize(vec3(2, dL - dR, 0));
		//		vec3 vb = glm::normalize(vec3(0, dT - dB, 2));
		//		normal = glm::normalize(glm::cross(va, vb));*/
		//			
		//		int idx = iX + iY * plane->sizeX;				
		//		plane->model.verts[idx].pos.y = plane->map[idx];
		//		plane->model.verts[idx].normal = normal;
		//	}
		//}

		//D3D11_MAPPED_SUBRESOURCE ms;
		//appContext->d3dDeviceContext->Map(plane->renderModel.vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		//memcpy(ms.pData, plane->model.verts.data(), plane->model.verts.size() * sizeof(ldiMeshVertex));
		//appContext->d3dDeviceContext->Unmap(plane->renderModel.vertexBuffer, 0);

		/*{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(1, 1, 1, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

			gfxRenderModel(appContext, &Tool->displacePlane.renderModel, false, Tool->displaceVertexShader, Tool->displacePixelShader, Tool->raymarchInputLayout);
		}

		if (Tool->showWireframe) {
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);

			gfxRenderModel(appContext, &Tool->displacePlane.renderModel, true, Tool->displaceVertexShader, Tool->displacePixelShader, Tool->raymarchInputLayout);
		}*/
	}

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

	renderDebugPrimitives(appContext, &appContext->defaultDebug);
}

void modelEditorShowUi(ldiModelEditor* Tool) {
	ImGui::Begin("Model editor controls");
	if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Background", (float*)&Tool->viewBackgroundColor);
		ImGui::ColorEdit3("Grid", (float*)&Tool->gridColor);
		ImGui::SliderFloat("Camera speed", &Tool->cameraSpeed, 0.0f, 1.0f);
		ImGui::Checkbox("Show raymarching", &Tool->showRaymarchView);
		ImGui::Checkbox("Show wireframe", &Tool->showWireframe);
		ImGui::Checkbox("Show voxel debug", &Tool->showVoxelDebug);
		
		ImGui::SliderFloat("Displacement", &Tool->dispBlend, 0.0f, 1.0f);
		ImGui::SliderFloat("Target1", &Tool->target1Blend, 0.0f, 1.0f);
		ImGui::SliderFloat("Target2", &Tool->target2Blend, 0.0f, 1.0f);

		ImGui::DragFloat3("Decal0", (float*)&Tool->decal0Pos, 0.001f, -1.0f, 1.0f);
		ImGui::SliderFloat("Decal0 scale", &Tool->decal0Scale, 0.01f, 2.0f);
	}
		
	ImGui::End();

	if (ImGui::Begin("UV view", 0, ImGuiWindowFlags_NoCollapse)) {
		ImGui::Text("Temp render texture");
		ImGui::Image(Tool->tempRenderTexView, ImVec2(512, 512));
		
		ImGui::Text("Displacement render texture");
		ImGui::Image(Tool->dispRenderTexView, ImVec2(512, 512));
		ImGui::Text("World pos render texture");
		ImGui::Image(Tool->worldPosRenderTexView, ImVec2(512, 512));

		ImGui::Text("Baked displacement texture");
		ImGui::Image(Tool->dispView, ImVec2(512, 512));
		ImGui::Text("Baked normals texture");
		ImGui::Image(Tool->dispNormView, ImVec2(512, 512));
	}
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Model editor", 0, ImGuiWindowFlags_NoCollapse)) {
		ImVec2 viewSize = ImGui::GetContentRegionAvail();
		ImVec2 startPos = ImGui::GetCursorPos();
		ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

		// This will catch our interactions.
		ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool isHovered = ImGui::IsItemHovered(); // Hovered
		const bool isActive = ImGui::IsItemActive();   // Held

		{
			vec3 camMove(0, 0, 0);
			ldiCamera* camera = &Tool->camera;
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
				float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime * Tool->cameraSpeed;
				camera->position += camMove * cameraSpeed;
			}
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.15f;
			Tool->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
		}

		if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
			mouseDelta *= 0.025f;

			ldiCamera* camera = &Tool->camera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

			camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
			camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
		}

		ImGui::SetCursorPos(startPos);
		std::vector<ldiTextInfo> textBuffer;
		modelEditorRender(Tool, viewSize.x, viewSize.y, &textBuffer);
		ImGui::Image(Tool->renderViewBuffers.mainViewResourceView, viewSize);

		// NOTE: ImDrawList API uses screen coordinates!
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int i = 0; i < textBuffer.size(); ++i) {
			ldiTextInfo* info = &textBuffer[i];
			drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
		}

		{
			// Viewport overlay widgets.
			ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			ImGui::BeginChild("_simpleOverlayMainView", ImVec2(200, 25), false, ImGuiWindowFlags_NoScrollbar);

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::EndChild();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}