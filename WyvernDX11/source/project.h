#pragma once

#include "lcms2.h"

struct ldiProjectContext {
	std::string					path;

	bool						scanLoaded = false;
	ldiScan						scan = {};

	bool						sourceModelLoaded = false;
	ldiModel					sourceModel;
	float						sourceModelScale = 1.0f;
	vec3						sourceModelTranslate = vec3(0.0f, 0.0f, 0.0f);
	vec3						sourceModelRotate = vec3(0.0f, 0.0f, 0.0f);
	ldiRenderModel				sourceRenderModel;
	ldiPhysicsMesh				sourceCookedModel;

	bool						sourceTextureLoaded = false;
	ldiImage					sourceTextureRaw;
	ID3D11Texture2D*			sourceTexture;
	ID3D11ShaderResourceView*	sourceTextureSrv;
	ldiImage					sourceTextureCmyk;
	ldiImage					sourceTextureCmykChannels[4];
	ID3D11Texture2D*			sourceTextureCmykTexture[4];
	ID3D11ShaderResourceView*	sourceTextureCmykSrv[4];

	bool						quadModelLoaded = false;
	ldiQuadModel				quadModel;
	ldiRenderModel				quadDebugModel;
	ldiRenderModel				quadModelWhite;
	ldiRenderLines				quadMeshWire;

	bool						registeredModelLoaded = false;
	ldiQuadModel				registeredModel;
	ldiRenderModel				registeredRenderModel;

	bool						surfelsLoaded = false;
	std::vector<ldiNewSurfel>	surfels;
	ldiRenderModel				surfelsRenderModel;
	ldiRenderModel				surfelsGroupRenderModel;
	ldiRenderModel				surfelsCoverageViewRenderModel;
	ldiImage					surfelsSamplesRaw;
	ID3D11Texture2D*			surfelsSamplesTexture;
	ID3D11ShaderResourceView*	surfelsSamplesTextureSrv;
	vec3						surfelsBoundsMin;
	vec3						surfelsBoundsMax;
	ldiSpatialGrid				surfelsSpatialGrid = {};

	//bool						poissonSamplesLoaded = false;
	//ldiPoissonSpatialGrid		poissonSpatialGrid = {};
	//ldiRenderModel				poissonSamplesRenderModel;
	//ldiRenderModel				poissonSamplesCmykRenderModel[4];
	//ldiRenderPointCloud			pointCloudRenderModel;

	bool						processed = false;
	ldiPointCloud				pointDistrib;
	ldiRenderPointCloud			pointDistribCloud;

	int							toolViewSize = 512;
	ID3D11Texture2D*			toolViewTex;
	ID3D11ShaderResourceView*	toolViewTexView;
	ID3D11RenderTargetView*		toolViewTexRtView;
	ID3D11Texture2D*			toolViewDepthStencil;
	ID3D11DepthStencilView*		toolViewDepthStencilView;
	ID3D11ShaderResourceView*	toolViewDepthStencilSrv;

	ldiCamera					galvoCam;
	mat4						workWorldMat;

	float						fovX = 11.478f;
	float						fovY = 11.478f;

	std::vector<vec3>			debugPoints;
	std::vector<ldiLineSegment> debugLineSegs;

	ID3D11Buffer*				coverageBuffer;
	ID3D11UnorderedAccessView*	coverageBufferUav;
	ID3D11ShaderResourceView*	coverageBufferSrv;
	ID3D11Buffer*				coverageStagingBuffer;

	int							coverageSurfelCount;
};

mat4 projectGetSourceTransformMat(ldiProjectContext* Project) {
	mat4 result = glm::identity<mat4>();

	result = glm::translate(result, vec3(Project->sourceModelTranslate));
	result = result * glm::toMat4(quat(Project->sourceModelRotate));
	result = glm::scale(result, vec3(Project->sourceModelScale));

	return result;
}

void projectInvalidateSourceModelData(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->sourceModelLoaded) {
		Project->sourceModelLoaded = false;
		Project->sourceRenderModel.indexBuffer->Release();
		Project->sourceRenderModel.vertexBuffer->Release();
	}
}

void projectInvalidateSourceTextureData(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->sourceTextureLoaded) {
		Project->sourceTextureLoaded = false;
		Project->sourceTexture->Release();
		Project->sourceTextureSrv->Release();
		delete[] Project->sourceTextureRaw.data;
		delete[] Project->sourceTextureCmyk.data;

		for (int i = 0; i < 4; ++i) {
			delete[] Project->sourceTextureCmykChannels[i].data;
			Project->sourceTextureCmykTexture[i]->Release();
			Project->sourceTextureCmykSrv[i]->Release();
		}
	}
}

void projectInvalidateQuadModelData(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->quadModelLoaded) {
		Project->quadModelLoaded = false;		
		Project->quadDebugModel.indexBuffer->Release();
		Project->quadDebugModel.vertexBuffer->Release();
		Project->quadModelWhite.indexBuffer->Release();
		Project->quadModelWhite.vertexBuffer->Release();
		Project->quadMeshWire.indexBuffer->Release();
		Project->quadMeshWire.vertexBuffer->Release();
	}
}

void projectInvalidateRegisteredModelData(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->registeredModelLoaded) {
		Project->registeredModelLoaded = false;
	}
}

void projectInvalidateSurfelData(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->surfelsLoaded) {
		Project->surfelsLoaded = false;
		physicsDestroyCookedMesh(AppContext->physics, &Project->sourceCookedModel);
		Project->surfelsRenderModel.indexBuffer->Release();
		Project->surfelsRenderModel.vertexBuffer->Release();
		delete[] Project->surfelsSamplesRaw.data;
		Project->surfelsSamplesTexture->Release();
		Project->surfelsSamplesTextureSrv->Release();
		spatialGridDestroy(&Project->surfelsSpatialGrid);
	}
}

void projectInit(ldiApp* AppContext, ldiProjectContext* Project) {
	projectInvalidateSourceModelData(AppContext, Project);
	projectInvalidateSourceTextureData(AppContext, Project);
	projectInvalidateQuadModelData(AppContext, Project);
	projectInvalidateRegisteredModelData(AppContext, Project);
	projectInvalidateSurfelData(AppContext, Project);
	*Project = {};
}

bool projectFinalizeImportedModel(ldiApp* AppContext, ldiProjectContext* Project) {
	Project->sourceRenderModel = gfxCreateRenderModel(AppContext, &Project->sourceModel);
	Project->sourceModelLoaded = true;

	return true;
}

bool projectImportModel(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	projectInvalidateSourceModelData(AppContext, Project);
	std::cout << "Project source mesh path: " << Path << "\n";

	uint8_t* sourceModelFile = nullptr;
	int sourceModelFileSize = readFile(Path, &sourceModelFile);

	if (sourceModelFileSize < 0) {
		std::cout << "Failed to load source model file " << Path << "\n";
		return false;
	}

	Project->sourceModel = objLoadModel(sourceModelFile, sourceModelFileSize);

	return projectFinalizeImportedModel(AppContext, Project);
}

bool projectFinalizeImportedTexture(ldiApp* AppContext, ldiProjectContext* Project) {
	vec3 cmykColor[4] = {
		vec3(0, 166.0 / 255.0, 214.0 / 255.0),
		vec3(1.0f, 0, 144.0 / 255.0),
		vec3(245.0 / 255.0, 230.0 / 255.0, 23.0 / 255.0),
		vec3(0.0f, 0.0f, 0.0f)
	};

	// NOTE: Converted to sRGB for viewing purposes.
	for (int channelIter = 0; channelIter < 4; ++channelIter) {
		ldiImage* image = &Project->sourceTextureCmykChannels[channelIter];

		image->width = Project->sourceTextureCmyk.width;
		image->height = Project->sourceTextureCmyk.height;
		image->data = new uint8_t[image->width * image->height * 4];

		for (int i = 0; i < image->width * image->height; ++i) {
			float v = ((float)Project->sourceTextureCmyk.data[i * 4 + channelIter]) / 255.0f;
			//float vi = LinearToGamma(v) * 255.0f;
			float vi = LinearToGamma(v);

			vec3 col = glm::mix(vec3(1, 1, 1), cmykColor[channelIter], vi);

			image->data[i * 4 + 0] = (uint8_t)(col.r * 255);
			image->data[i * 4 + 1] = (uint8_t)(col.g * 255);
			image->data[i * 4 + 2] = (uint8_t)(col.b * 255);

			//image->data[i * 4 + 0] = cmykImagePixels[i * 4 + channelIter];
			//image->data[i * 4 + 1] = cmykImagePixels[i * 4 + channelIter];
			//image->data[i * 4 + 2] = cmykImagePixels[i * 4 + channelIter];
			image->data[i * 4 + 3] = 255;
		}
	}
	
	if (!gfxCreateTextureR8G8B8A8Basic(AppContext, &Project->sourceTextureRaw, &Project->sourceTexture, &Project->sourceTextureSrv)) {
		return false;
	}

	for (int channelIter = 0; channelIter < 4; ++channelIter) {
		ldiImage* image = &Project->sourceTextureCmykChannels[channelIter];

		if (!gfxCreateTextureR8G8B8A8Basic(AppContext, image, &Project->sourceTextureCmykTexture[channelIter], &Project->sourceTextureCmykSrv[channelIter])) {
			return false;
		}
	}

	Project->sourceTextureLoaded = true;

	return true;
}

bool projectImportTexture(ldiApp* AppContext, ldiProjectContext* Project, const char* Path) {
	projectInvalidateSourceTextureData(AppContext, Project);

	std::cout << "Project source texture path: " << Path << "\n";

	double t0 = getTime();

	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba(Path, &x, &y, &n);

	Project->sourceTextureRaw.width = x;
	Project->sourceTextureRaw.height = y;
	Project->sourceTextureRaw.data = imageRawPixels;

	t0 = getTime() - t0;
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ") " << t0 * 1000.0f << " ms\n";

	//----------------------------------------------------------------------------------------------------
	// CMYK transformation.
	//----------------------------------------------------------------------------------------------------
	//hsRGB = cmsCreate_sRGBProfile();
	cmsHPROFILE srgbProfile = cmsOpenProfileFromFile("../../assets/profiles/sRGB_v4_ICC_preference.icc", "r");
	cmsHPROFILE cmykProfile = cmsOpenProfileFromFile("../../assets/profiles/USWebCoatedSWOP.icc", "r");

	double versionInfo = cmsGetProfileVersion(srgbProfile);
	std::cout << "sRGB olor profile info: " << versionInfo << "\n";

	versionInfo = cmsGetProfileVersion(cmykProfile);
	std::cout << "CMYK color profile info: " << versionInfo << "\n";

	cmsHTRANSFORM colorTransform = cmsCreateTransform(srgbProfile, TYPE_RGBA_8, cmykProfile, TYPE_CMYK_8, INTENT_PERCEPTUAL, 0);

	std::cout << "Converting sRGB image to CMYK\n";
	uint8_t* cmykImagePixels = new uint8_t[x * y * 4];
	cmsDoTransform(colorTransform, imageRawPixels, cmykImagePixels, x * y);

	Project->sourceTextureCmyk.width = x;
	Project->sourceTextureCmyk.height = y;
	Project->sourceTextureCmyk.data = cmykImagePixels;

	cmsDeleteTransform(colorTransform);
	cmsCloseProfile(srgbProfile);
	cmsCloseProfile(cmykProfile);

	return projectFinalizeImportedTexture(AppContext, Project);
}

bool projectCreateVoxelMesh(ldiModel* Model) {
	double t0 = getTime();
	if (!stlSaveModel("../cache/source.stl", Model)) {
		return false;
	}
	t0 = getTime() - t0;
	std::cout << "Save STL: " << t0 * 1000.0f << " ms\n";

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// char args[] = "vdb_tool.exe -read ../../../bin/cache/source.stl -m2ls voxel=0.01 -l2m -write ../../../bin/cache/voxel.ply";
	char args[] = "vdb_tool.exe -read ../../../bin/cache/source.stl -m2ls voxel=0.0075 -l2m -write ../../../bin/cache/voxel.ply";

	CreateProcessA(
		"../../assets/bin/openvdb/vdb_tool.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		"../../assets/bin/openvdb",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);

	ldiModel model;
	plyLoadModel("../cache/voxel.ply", &model);
	//stlSaveModel("../cache/voxel_read.stl", &model);
	plySaveModel("../cache/voxel_tri_invert.ply", &model);

	return true;
}

bool projectCreateQuadMesh(const std::string& OutputPath) {
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char args[2048];
	// NOTE: scale = (side length in um) * 0.0002
	sprintf_s(args, "\"Instant Meshes.exe\" ../../bin/cache/voxel_tri_invert.ply -o %s --scale 0.04", OutputPath.c_str());

	CreateProcessA(
		"../../assets/bin/Instant Meshes.exe",
		args,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		"../../assets/bin",
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);

	return true;
}

static void _printQuadModelInfo(ldiQuadModel* Model) {
	int quadCount = Model->indices.size() / 4;

	double totalSideLengths = 0.0;
	int totalSideCount = 0;
	float maxSide = FLT_MIN;
	float minSide = FLT_MAX;

	// Side lengths.
	for (int i = 0; i < quadCount; ++i) {
		vec3 v0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 v1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 v2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 v3 = Model->verts[Model->indices[i * 4 + 3]];

		float s0 = glm::length(v0 - v1);
		float s1 = glm::length(v1 - v2);
		float s2 = glm::length(v2 - v3);
		float s3 = glm::length(v3 - v0);

		maxSide = max(maxSide, s0);
		maxSide = max(maxSide, s1);
		maxSide = max(maxSide, s2);
		maxSide = max(maxSide, s3);

		minSide = min(minSide, s0);
		minSide = min(minSide, s1);
		minSide = min(minSide, s2);
		minSide = min(minSide, s3);

		totalSideLengths += s0 + s1 + s2 + s3;
		totalSideCount += 4;
	}

	totalSideLengths /= double(totalSideCount);

	std::cout << "Quad model - quads: " << quadCount << " sideAvg: " << (totalSideLengths * 10000.0) << " um \n";
	std::cout << "Min: " << (minSide * 10000.0) << " um Max: " << (maxSide * 10000.0) << " um \n";
}

bool projectFinalizeQuadModel(ldiApp* AppContext, ldiProjectContext* Project) {
	Project->quadDebugModel = gfxCreateRenderQuadModelDebug(AppContext, &Project->quadModel);
	Project->quadModelWhite = gfxCreateRenderQuadModelWhite(AppContext, &Project->quadModel, vec3(0.9f, 0.9f, 0.9f));
	Project->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &Project->quadModel);
	Project->quadModelLoaded = true;

	_printQuadModelInfo(&Project->quadModel);

	return true;
}

static bool projectLoadQuadModel(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	Project->quadModelLoaded = false;

	if (!plyLoadQuadMesh(Path.c_str(), &Project->quadModel)) {
		return false;
	}

	return true;
}

static bool projectCreateQuadModel(ldiApp* AppContext, ldiProjectContext* Project) {
	projectInvalidateQuadModelData(AppContext, Project);

	if (!Project->sourceModelLoaded) {
		return false;
	}

	Project->quadModelLoaded = false;

	mat4 worldMat = projectGetSourceTransformMat(Project);
	ldiModel transModel = modelGetTransformed(&Project->sourceModel, worldMat);

	if (!projectCreateVoxelMesh(&transModel)) {
		return false;
	}

	if (!projectCreateQuadMesh("../../bin/cache/quad_mesh.ply")) {
		return false;
	}

	if (!projectLoadQuadModel(AppContext, Project, "../cache/quad_mesh.ply")) {
		return false;
	}

	return projectFinalizeQuadModel(AppContext, Project);
}

void geoCreateSurfels(ldiQuadModel* Model, std::vector<ldiSurfel>* Result) {
	int quadCount = Model->indices.size() / 4;

	Result->clear();
	Result->reserve(quadCount);

	float normalAdjust = 0.0;

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 p1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 p2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 p3 = Model->verts[Model->indices[i * 4 + 3]];

		vec3 center = p0 + p1 + p2 + p3;
		center /= 4;

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		vec3 normal0 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		vec3 normal1 = glm::normalize(glm::cross(p3 - p2, p1 - p2));

		vec3 normal = normal0 + normal1;
		normal = glm::normalize(normal);

		ldiSurfel surfel{};
		surfel.id = i;
		surfel.position = center;// + normal * normalAdjust;
		surfel.normal = normal;
		surfel.color = vec4(0, 0, 0, 0);
		surfel.scale = 0.0075f;

		Result->push_back(surfel);
	}
}

void geoCreateSurfelsHigh(ldiQuadModel* Model, std::vector<ldiSurfel>* Result) {
	int quadCount = Model->indices.size() / 4;

	Result->clear();
	Result->reserve(quadCount * 4);

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 p1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 p2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 p3 = Model->verts[Model->indices[i * 4 + 3]];

		vec3 center = p0 + p1 + p2 + p3;
		center /= 4;

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		vec3 normal0 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		vec3 normal1 = glm::normalize(glm::cross(p3 - p2, p1 - p2));

		vec3 normal = normal0 + normal1;
		normal = glm::normalize(normal);

		vec3 s[4];

		// Bilinear interpolation of corners.
		vec3 a = (p0 + (p1 - p0) * 0.25f);
		vec3 b = (p3 + (p2 - p3) * 0.25f);
		s[0] = (a + (b - a) * 0.25f);
		s[1] = (a + (b - a) * 0.75f);

		a = (p0 + (p1 - p0) * 0.75f);
		b = (p3 + (p2 - p3) * 0.75f);
		s[2] = (a + (b - a) * 0.25f);
		s[3] = (a + (b - a) * 0.75f);

		for (int sIter = 0; sIter < 4; ++sIter) {
			ldiSurfel surfel{};
			surfel.id = i;
			surfel.normal = normal;
			surfel.color = vec4(0.5f, 0.5f, 0.5f, 0.0f);
			surfel.scale = 0.0075f;
			surfel.position = s[sIter];

			Result->push_back(surfel);
		}
	}
}

void geoCreateSurfelsNew(ldiQuadModel* Model, std::vector<ldiNewSurfel>* Result, const int SamplesTexWidth, const int SamplesPerSide) {
	const int samplesPerSurfel = SamplesPerSide * SamplesPerSide;
	const double sampleTexPixel = 1.0 / SamplesTexWidth;

	const int quadCount = Model->indices.size() / 4;

	Result->clear();
	Result->reserve(quadCount);

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = Model->verts[Model->indices[i * 4 + 0]];
		vec3 p1 = Model->verts[Model->indices[i * 4 + 1]];
		vec3 p2 = Model->verts[Model->indices[i * 4 + 2]];
		vec3 p3 = Model->verts[Model->indices[i * 4 + 3]];

		vec3 center = p0 + p1 + p2 + p3;
		center /= 4;

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		vec3 normal0 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		vec3 normal1 = glm::normalize(glm::cross(p3 - p2, p1 - p2));

		vec3 normal = normal0 + normal1;
		normal = glm::normalize(normal);

		// Sample texture location.
		const int tileX = i % (SamplesTexWidth / SamplesPerSide);
		const int tileY = i / (SamplesTexWidth / SamplesPerSide);
		const vec2 uvMin(tileX * SamplesPerSide * sampleTexPixel, tileY * SamplesPerSide * sampleTexPixel);
		const vec2 uvMax((tileX + 1) * SamplesPerSide * sampleTexPixel, (tileY + 1) * SamplesPerSide * sampleTexPixel);

		ldiNewSurfel surfel = {};
		surfel.id = i;
		surfel.position = center;
		surfel.normal = normal;

		vec3 color(1, 1, 1);// = normal * 0.5f + 0.5f;
		
		surfel.verts[0].color = color;
		surfel.verts[0].position = p0;
		surfel.verts[0].uv = vec2(uvMin.x, uvMin.y);

		surfel.verts[1].color = color;
		surfel.verts[1].position = p1;
		surfel.verts[1].uv = vec2(uvMax.x, uvMin.y);

		surfel.verts[2].color = color;
		surfel.verts[2].position = p2;
		surfel.verts[2].uv = vec2(uvMax.x, uvMax.y);

		surfel.verts[3].color = color;
		surfel.verts[3].position = p3;
		surfel.verts[3].uv = vec2(uvMin.x, uvMax.y);
		
		Result->push_back(surfel);
	}
}

vec4 getColorSample(ldiImage* Image, vec2 Uv) {
	Uv.y = 1.0 - Uv.y;

	float cX = Uv.x * Image->width;
	float cY = Uv.y * Image->height;

	int p0X = (int)cX;
	int p0Y = (int)cY;

	p0X = clamp(p0X, 0, Image->width - 1);
	p0Y = clamp(p0Y, 0, Image->height - 1);

	uint32_t sample = *(uint32_t*)(&Image->data[(p0X + p0Y * Image->width) * 4]);

	vec4 result;
	result.r = (float)((sample >> 0) & 0xFF) / 255.0;
	result.g = (float)((sample >> 8) & 0xFF) / 255.0;
	result.b = (float)((sample >> 16) & 0xFF) / 255.0;
	result.a = (float)((sample >> 24) & 0xFF) / 255.0;

	return result;
}

vec4 getColorSampleBilinear(ldiImage* Image, vec2 Uv) {
	Uv.y = 1.0 - Uv.y;

	float cX = Uv.x * Image->width;
	float cY = Uv.y * Image->height;

	int p0X = (int)(cX - 0.5);
	int p0Y = (int)(cY - 0.5);

	int p1X = (int)(cX + 0.5);
	int p1Y = (int)(cY - 0.5);

	int p2X = (int)(cX - 0.5);
	int p2Y = (int)(cY + 0.5);

	int p3X = (int)(cX + 0.5);
	int p3Y = (int)(cY + 0.5);

	float lerpX = (cX + 0.5f) - floorf(cX + 0.5f);
	float lerpY = (cY + 0.5f) - floorf(cY + 0.5f);

	p0X = clamp(p0X, 0, Image->width - 1);
	p0Y = clamp(p0Y, 0, Image->height - 1);

	p1X = clamp(p1X, 0, Image->width - 1);
	p1Y = clamp(p1Y, 0, Image->height - 1);

	p2X = clamp(p2X, 0, Image->width - 1);
	p2Y = clamp(p2Y, 0, Image->height - 1);

	p3X = clamp(p3X, 0, Image->width - 1);
	p3Y = clamp(p3Y, 0, Image->height - 1);

	vec4 s0;
	vec4 s1;
	vec4 s2;
	vec4 s3;

	{
		uint32_t sample = *(uint32_t*)(&Image->data[(p0X + p0Y * Image->width) * 4]);
		
		s0.r = (float)((sample >> 0) & 0xFF) / 255.0;
		s0.g = (float)((sample >> 8) & 0xFF) / 255.0;
		s0.b = (float)((sample >> 16) & 0xFF) / 255.0;
		s0.a = (float)((sample >> 24) & 0xFF) / 255.0;
	}

	{
		uint32_t sample = *(uint32_t*)(&Image->data[(p1X + p1Y * Image->width) * 4]);

		s1.r = (float)((sample >> 0) & 0xFF) / 255.0;
		s1.g = (float)((sample >> 8) & 0xFF) / 255.0;
		s1.b = (float)((sample >> 16) & 0xFF) / 255.0;
		s1.a = (float)((sample >> 24) & 0xFF) / 255.0;
	}

	{
		uint32_t sample = *(uint32_t*)(&Image->data[(p2X + p2Y * Image->width) * 4]);

		s2.r = (float)((sample >> 0) & 0xFF) / 255.0;
		s2.g = (float)((sample >> 8) & 0xFF) / 255.0;
		s2.b = (float)((sample >> 16) & 0xFF) / 255.0;
		s2.a = (float)((sample >> 24) & 0xFF) / 255.0;
	}

	{
		uint32_t sample = *(uint32_t*)(&Image->data[(p3X + p3Y * Image->width) * 4]);

		s3.r = (float)((sample >> 0) & 0xFF) / 255.0;
		s3.g = (float)((sample >> 8) & 0xFF) / 255.0;
		s3.b = (float)((sample >> 16) & 0xFF) / 255.0;
		s3.a = (float)((sample >> 24) & 0xFF) / 255.0;
	}

	vec4 x0 = glm::mix(s0, s1, lerpX);
	vec4 x1 = glm::mix(s2, s3, lerpX);
	vec4 y1 = glm::mix(x0, x1, lerpY);
	
	return y1;
}

struct ldiColorTransferThreadContext {
	ldiPhysics* physics;
	ldiPhysicsMesh* cookedMesh;
	ldiModel* srcModel;
	ldiImage* image;
	ldiImage* samplesImage;
	std::vector<ldiNewSurfel>* surfels;
	int samplesPerSide;
	int	startIdx;
	int	endIdx;
};

void geoTransferThreadBatch(ldiColorTransferThreadContext Context) {
	const float normalAdjust = 0.01;
	const double sampleTexPixel = 1.0 / Context.samplesImage->width;
	const double samplePosOffsetHalf = 0.5 / Context.samplesPerSide;

	for (size_t i = Context.startIdx; i < Context.endIdx; ++i) {
		ldiNewSurfel* s = &(*Context.surfels)[i];

		for (int iY = 0; iY < Context.samplesPerSide; ++iY) {
			for (int iX = 0; iX < Context.samplesPerSide; ++iX) {
				const double lerpX = ((double)iX / (double)Context.samplesPerSide) + samplePosOffsetHalf;
				const double lerpY = ((double)iY / (double)Context.samplesPerSide) + samplePosOffsetHalf;

				const vec3 posX0 = glm::mix(s->verts[0].position, s->verts[1].position, lerpX);
				const vec3 posX1 = glm::mix(s->verts[3].position, s->verts[2].position, lerpX);
				const vec3 pos = glm::mix(posX0, posX1, lerpY);

				const float uvX = lerp(s->verts[0].uv.x, s->verts[1].uv.x, lerpX);
				const float uvY = lerp(s->verts[0].uv.y, s->verts[2].uv.y, lerpY);
				
				const int pX = (int)(uvX / sampleTexPixel);
				const int pY = (int)(uvY / sampleTexPixel);
				const int samplesImageIdx = pX + pY * Context.samplesImage->width;

				ldiRaycastResult result = physicsRaycast(Context.cookedMesh, pos + s->normal * normalAdjust, -s->normal, 0.1f);

				if (result.hit) {
					ldiMeshVertex v0 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 0]];
					ldiMeshVertex v1 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 1]];
					ldiMeshVertex v2 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 2]];

					float u = result.barry.x;
					float v = result.barry.y;
					float w = 1.0 - (u + v);
					vec2 uv = w * v0.uv + u * v1.uv + v * v2.uv;

					vec4 s0 = getColorSampleBilinear(Context.image, uv);
					//vec4 s0 = getColorSample(Context.image, uv);

					Context.samplesImage->data[samplesImageIdx * 4 + 0] = s0.r * 255;
					Context.samplesImage->data[samplesImageIdx * 4 + 1] = s0.g * 255;
					Context.samplesImage->data[samplesImageIdx * 4 + 2] = s0.b * 255;
					Context.samplesImage->data[samplesImageIdx * 4 + 3] = s0.a * 255;
				} else {
					Context.samplesImage->data[samplesImageIdx * 4 + 0] = 255;
					Context.samplesImage->data[samplesImageIdx * 4 + 1] = 0;
					Context.samplesImage->data[samplesImageIdx * 4 + 2] = 0;
					Context.samplesImage->data[samplesImageIdx * 4 + 3] = 255;
				}
			}
		}
	}
}

void geoTransferColorToSurfels(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiImage* Image, std::vector<ldiNewSurfel>* Surfels, ldiImage* SamplesImage) {
	double t0 = getTime();

	const int threadCount = 20;
	int batchSize = Surfels->size() / threadCount;
	int batchRemainder = Surfels->size() - (batchSize * threadCount);
	std::thread workerThread[threadCount];

	const int samplesPerSide = 4;

	for (int t = 0; t < threadCount; ++t) {
		ldiColorTransferThreadContext tc{};
		tc.physics = AppContext->physics;
		tc.cookedMesh = CookedMesh;
		tc.srcModel = SrcModel;
		tc.image = Image;
		tc.samplesImage = SamplesImage;
		tc.surfels = Surfels;
		tc.samplesPerSide = samplesPerSide;
		tc.startIdx = t * batchSize;
		tc.endIdx = (t + 1) * batchSize;

		if (t == threadCount - 1) {
			tc.endIdx += batchRemainder;
		}

		workerThread[t] = std::thread(geoTransferThreadBatch, tc);
	}

	for (int t = 0; t < threadCount; ++t) {
		workerThread[t].join();
	}

	t0 = getTime() - t0;
	std::cout << "Transfer color count: " << (Surfels->size() * samplesPerSide * samplesPerSide) << "\n";
	std::cout << "Transfer time: " << t0 * 1000.0f << " ms\n";
}

struct ldiSmoothNormalsThreadContext {
	ldiSpatialGrid* grid;
	std::vector<ldiSurfel>* surfels;
	int						startIdx;
	int						endIdx;
};

void surfelsSmoothNormalsThreadBatch(ldiSmoothNormalsThreadContext Context) {
	for (int i = Context.startIdx; i < Context.endIdx; ++i) {
		ldiSurfel* srcSurfel = &(*Context.surfels)[i];
		vec3 avgNorm = srcSurfel->normal;
		int avgCount = 1;

		vec3 cell = spatialGridGetCellFromWorldPosition(Context.grid, srcSurfel->position);

		int sX = (int)(cell.x - 0.5f);
		int eX = sX + 1;

		int sY = (int)(cell.y - 0.5f);
		int eY = sY + 1;

		int sZ = (int)(cell.z - 0.5f);
		int eZ = sZ + 1;

		for (int iZ = sZ; iZ <= eZ; ++iZ) {
			for (int iY = sY; iY <= eY; ++iY) {
				for (int iX = sX; iX <= eX; ++iX) {
					ldiSpatialCellResult cellResult = spatialGridGetCell(Context.grid, iX, iY, iZ);
					for (int s = 0; s < cellResult.count; ++s) {
						int surfelId = cellResult.data[s];

						if (surfelId != i) {
							ldiSurfel* dstSurfel = &(*Context.surfels)[surfelId];

							float dist = glm::length(dstSurfel->position - srcSurfel->position);

							if (dist < 0.02f) {
								avgNorm += dstSurfel->normal * (1.0f - (dist / 0.02f));
								avgCount += 1;
							}
						}
					}
				}
			}
		}

		avgNorm = glm::normalize(avgNorm);
		srcSurfel->smoothedNormal = avgNorm;
	}
}

bool projectFinalizeSurfels(ldiApp* AppContext, ldiProjectContext* Project) {
	gfxCreateTextureR8G8B8A8Basic(AppContext, &Project->surfelsSamplesRaw, &Project->surfelsSamplesTexture, &Project->surfelsSamplesTextureSrv);
	Project->surfelsRenderModel = gfxCreateNewSurfelRenderModel(AppContext, &Project->surfels);

	//----------------------------------------------------------------------------------------------------
	// Create spatial structure for surfels.
	//----------------------------------------------------------------------------------------------------
	double t0 = getTime();
	vec3 surfelsMin(10000, 10000, 10000);
	vec3 surfelsMax(-10000, -10000, -10000);

	for (size_t i = 0; i < Project->surfels.size(); ++i) {
		ldiNewSurfel* s = &Project->surfels[i];

		surfelsMin.x = min(surfelsMin.x, s->position.x);
		surfelsMin.y = min(surfelsMin.y, s->position.y);
		surfelsMin.z = min(surfelsMin.z, s->position.z);

		surfelsMax.x = max(surfelsMax.x, s->position.x);
		surfelsMax.y = max(surfelsMax.y, s->position.y);
		surfelsMax.z = max(surfelsMax.z, s->position.z);
	}

	ldiSpatialGrid spatialGrid{};
	spatialGridInit(&spatialGrid, surfelsMin, surfelsMax, 0.03f);

	for (size_t i = 0; i < Project->surfels.size(); ++i) {
		spatialGridPrepEntry(&spatialGrid, Project->surfels[i].position);
	}

	spatialGridCompile(&spatialGrid);

	for (size_t i = 0; i < Project->surfels.size(); ++i) {
		spatialGridAddEntry(&spatialGrid, Project->surfels[i].position, (int)i);
	}

	Project->surfelsSpatialGrid = spatialGrid;
	Project->surfelsBoundsMin = surfelsMin;
	Project->surfelsBoundsMax = surfelsMax;

	t0 = getTime() - t0;
	std::cout << "Build surfel spatial grid: " << t0 * 1000.0f << " ms\n";
	
	Project->surfelsLoaded = true;

	return true;
}

bool projectCreateSurfels(ldiApp* AppContext, ldiProjectContext* Project) {
	projectInvalidateSurfelData(AppContext, Project);

	if (!Project->quadModelLoaded || !Project->sourceTextureLoaded) {
		return false;
	}

	Project->surfelsSamplesRaw.width = 4096;
	Project->surfelsSamplesRaw.height = Project->surfelsSamplesRaw.width;
	Project->surfelsSamplesRaw.data = new uint8_t[Project->surfelsSamplesRaw.width * Project->surfelsSamplesRaw.width * 4];
	memset(Project->surfelsSamplesRaw.data, 0, Project->surfelsSamplesRaw.width * Project->surfelsSamplesRaw.width * 4);

	geoCreateSurfelsNew(&Project->quadModel, &Project->surfels, Project->surfelsSamplesRaw.width, 4);
	//geoCreateSurfels(&Project->quadModel, &Project->surfelsLow);
	//geoCreateSurfelsHigh(&Project->quadModel, &Project->surfelsHigh);
	//std::cout << "High res surfel count: " << Project->surfelsHigh.size() << "\n";

	physicsCookMesh(AppContext->physics, &Project->sourceModel, &Project->sourceCookedModel);

	geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureCmyk, &Project->surfels, &Project->surfelsSamplesRaw);
	
	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureCmyk, &Project->surfelsHigh);
	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureRaw, &Project->surfelsHigh);

	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureRaw, &Project->surfelsNew, &Project->surfelsSamplesRaw);

	//imageWrite("samplestest.png", 4096, 4096, 4, 4096 * 4, Project->surfelsSamplesRaw.data);

	//----------------------------------------------------------------------------------------------------
	// Create spatial structure for surfels.
	//----------------------------------------------------------------------------------------------------
	/*double t0 = getTime();
	vec3 surfelsMin(10000, 10000, 10000);
	vec3 surfelsMax(-10000, -10000, -10000);

	for (size_t i = 0; i < Project->surfelsLow.size(); ++i) {
		ldiSurfel* s = &Project->surfelsLow[i];

		surfelsMin.x = min(surfelsMin.x, s->position.x);
		surfelsMin.y = min(surfelsMin.y, s->position.y);
		surfelsMin.z = min(surfelsMin.z, s->position.z);

		surfelsMax.x = max(surfelsMax.x, s->position.x);
		surfelsMax.y = max(surfelsMax.y, s->position.y);
		surfelsMax.z = max(surfelsMax.z, s->position.z);
	}

	ldiSpatialGrid spatialGrid{};
	spatialGridInit(&spatialGrid, surfelsMin, surfelsMax, 0.05f);

	for (size_t i = 0; i < Project->surfelsLow.size(); ++i) {
		spatialGridPrepEntry(&spatialGrid, Project->surfelsLow[i].position);
	}

	spatialGridCompile(&spatialGrid);

	for (size_t i = 0; i < Project->surfelsLow.size(); ++i) {
		spatialGridAddEntry(&spatialGrid, Project->surfelsLow[i].position, (int)i);
	}

	Project->surfelLowSpatialGrid = spatialGrid;
	Project->surfelsBoundsMin = surfelsMin;
	Project->surfelsBoundsMax = surfelsMax;

	t0 = getTime() - t0;
	std::cout << "Build spatial grid: " << t0 * 1000.0f << " ms\n";*/

	//----------------------------------------------------------------------------------------------------
	// Smooth normals.
	//----------------------------------------------------------------------------------------------------
	//t0 = getTime();
	//{
	//	const int threadCount = 20;
	//	int batchSize = Project->surfelsLow.size() / threadCount;
	//	int batchRemainder = Project->surfelsLow.size() - (batchSize * threadCount);
	//	std::thread workerThread[threadCount];

	//	for (int n = 0; n < 4; ++n) {
	//		for (int t = 0; t < threadCount; ++t) {
	//			ldiSmoothNormalsThreadContext tc{};
	//			tc.grid = &spatialGrid;
	//			tc.surfels = &Project->surfelsLow;
	//			tc.startIdx = t * batchSize;
	//			tc.endIdx = (t + 1) * batchSize;

	//			if (t == threadCount - 1) {
	//				tc.endIdx += batchRemainder;
	//			}

	//			workerThread[t] = std::move(std::thread(surfelsSmoothNormalsThreadBatch, tc));
	//		}

	//		for (int t = 0; t < threadCount; ++t) {
	//			workerThread[t].join();
	//		}

	//		// Apply smoothed normal back to surfel normal.
	//		for (int i = 0; i < Project->surfelsLow.size(); ++i) {
	//			ldiSurfel* srcSurfel = &Project->surfelsLow[i];
	//			srcSurfel->normal = srcSurfel->smoothedNormal;
	//		}
	//	}
	//}
	//t0 = getTime() - t0;
	//std::cout << "Normal smoothing: " << t0 * 1000.0f << " ms\n";

	//Project->surfelLowRenderModel = gfxCreateSurfelRenderModel(AppContext, &Project->surfelsLow);
	//Project->surfelHighRenderModel = gfxCreateSurfelRenderModel(AppContext, &Project->surfelsHigh, 0.001f, 0);
	//Project->coveragePointModel = gfxCreateCoveragePointRenderModel(AppContext, &Project->surfelsLow, &Project->quadModel);

	return projectFinalizeSurfels(AppContext, Project);
}

void _surfacePartitioning(ldiApp* AppContext, ldiProjectContext* Project) {
	ldiSpatialGrid* grid = &Project->surfelsSpatialGrid;
	std::vector<ldiNewSurfel>* surfels = &Project->surfels;

	std::vector<int> groupIds;
	groupIds.resize(surfels->size());

	struct ldiSurfelGroup {
		int id;
		vec3 color;
		vec3 normal;
		std::vector<int> surfelIds;
	};

	double t0 = getTime();

	std::vector<ldiSurfelGroup> surfelGroups;
	surfelGroups.push_back({ 0, getRandomColorHighSaturation(), vec3Zero });

	for (size_t iterSeed = 0; iterSeed < surfels->size(); ++iterSeed) {
		//for (size_t iterSeed = 0; iterSeed < 2000; ++iterSeed) {
		if (groupIds[iterSeed] != 0) {
			continue;
		}

		std::queue<int> surfelQueue;
		surfelQueue.push(iterSeed);
		surfelGroups.push_back({ (int)surfelGroups.size(), getRandomColorHighSaturation(), (*surfels)[iterSeed].normal });
		ldiSurfelGroup* currentGroup = &surfelGroups.back();

		while (true) {
			std::vector<bool> groupProcessed;
			groupProcessed.resize(surfels->size());

			int updateCount = 0;

			while (!surfelQueue.empty()) {
				int srcSurfelId = surfelQueue.front();
				surfelQueue.pop();

				if (groupProcessed[srcSurfelId]) {
					continue;
				}

				groupProcessed[srcSurfelId] = true;

				ldiNewSurfel* srcSurfel = &(*surfels)[srcSurfelId];
				groupIds[srcSurfelId] = currentGroup->id;
				currentGroup->surfelIds.push_back(srcSurfelId);

				vec3 cell = spatialGridGetCellFromWorldPosition(grid, srcSurfel->position);

				int sX = (int)cell.x - 1;
				int eX = sX + 3;

				int sY = (int)cell.y - 1;
				int eY = sY + 3;

				int sZ = (int)cell.z - 1;
				int eZ = sZ + 3;

				sX = max(0, sX);
				eX = min(grid->countX - 1, eX);

				sY = max(0, sY);
				eY = min(grid->countY - 1, eY);

				sZ = max(0, sZ);
				eZ = min(grid->countZ - 1, eZ);

				float distVal = 0.03f;

				for (int iZ = sZ; iZ <= eZ; ++iZ) {
					for (int iY = sY; iY <= eY; ++iY) {
						for (int iX = sX; iX <= eX; ++iX) {
							ldiSpatialCellResult cellResult = spatialGridGetCell(grid, iX, iY, iZ);
							for (int s = 0; s < cellResult.count; ++s) {
								int surfelId = cellResult.data[s];

								if (groupIds[surfelId] == 0) {
									ldiNewSurfel* dstSurfel = &(*surfels)[surfelId];

									float dist = glm::length(dstSurfel->position - srcSurfel->position);

									if (dist <= distVal) {
										float angle = glm::dot(glm::normalize(currentGroup->normal), dstSurfel->normal);

										if (angle > 0.866f) { // 30 degs
											++updateCount;
											currentGroup->normal += dstSurfel->normal;
											surfelQueue.push(surfelId);
											//groupIds[surfelId] = currentGroup->id;
											//currentGroup->surfelIds.push_back(surfelId);
										}
									}
								}
							}
						}
					}
				}
			}

			std::cout << "Update count: " << updateCount << "\n";

			if (updateCount > 0) {
				for (size_t i = 0; i < currentGroup->surfelIds.size(); ++i) {
					surfelQueue.push(currentGroup->surfelIds[i]);
				}
			} else {
				break;
			}
		}
	}

	/*surfelGroups.push_back({ (int)surfelGroups.size(), getRandomColorHighSaturation(), vec3Zero });

	for (size_t i = 0; i < surfels->size(); ++i) {
		ldiNewSurfel* srcSurfel = &(*surfels)[i];

		if (groupIds[i] != 0) {
			continue;
		}

		float angle = glm::dot(glm::normalize(surfelGroups[1].normal), srcSurfel->normal);

		if (angle > 0.9f) {
			groupIds[i] = (int)surfelGroups.size();
		}
	}*/

	t0 = getTime() - t0;
	std::cout << "Surfel grouping in: " << t0 * 1000.0f << " ms\n";

	for (size_t i = 0; i < Project->surfels.size(); ++i) {
		if (groupIds[i] != 0) {
			ldiNewSurfel* s = &(Project->surfels[i]);
			vec3 color = surfelGroups[groupIds[i]].color;

			s->verts[0].color = color;
			s->verts[1].color = color;
			s->verts[2].color = color;
			s->verts[3].color = color;

			/*ldiPointCloudVertex p = {};
			p.position = s->position;
			p.normal = s->normal;
			p.color = surfelGroups[groupIds[i]].color;

			Project->pointDistrib.points.push_back(p);*/
		}
	}

	//Project->surfelsGroupRenderModel = gfxCreateQuadSurfelRenderModel(AppContext, &Project->surfels);
}

bool _surfelCoveragePrep(ldiApp* AppContext, ldiProjectContext* Project) {
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = Project->toolViewSize;
		tex2dDesc.Height = Project->toolViewSize;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
		tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = 0;

		if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &Project->toolViewTex))) {
			std::cout << "Texture failed to create\n";
			return false;
		}

		if (AppContext->d3dDevice->CreateRenderTargetView(Project->toolViewTex, NULL, &Project->toolViewTexRtView) != S_OK) {
			std::cout << "CreateRenderTargetView failed\n";
			return false;
		}

		if (AppContext->d3dDevice->CreateShaderResourceView(Project->toolViewTex, NULL, &Project->toolViewTexView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return false;
		}

		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = Project->toolViewSize;
		depthStencilDesc.Height = Project->toolViewSize;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		AppContext->d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &Project->toolViewDepthStencil);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = 0;
		dsvDesc.Texture2D.MipSlice = 0;

		if (AppContext->d3dDevice->CreateDepthStencilView(Project->toolViewDepthStencil, &dsvDesc, &Project->toolViewDepthStencilView) != S_OK) {
			std::cout << "CreateDepthStencilView failed\n";
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		if (AppContext->d3dDevice->CreateShaderResourceView(Project->toolViewDepthStencil, &srvDesc, &Project->toolViewDepthStencilSrv) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return false;
		}
	}

	// Coverage resources setup.
	{
		if (!gfxCreateStructuredBuffer(AppContext, 4, Project->surfels.size(), NULL, &Project->coverageBuffer)) {
			return false;
		}

		if (!gfxCreateBufferUav(AppContext, Project->coverageBuffer, &Project->coverageBufferUav)) {
			return false;
		}

		if (!gfxCreateBufferShaderResourceView(AppContext, Project->coverageBuffer, &Project->coverageBufferSrv)) {
			return false;
		}

		if (!gfxCreateStagingBuffer(AppContext, 4, (int)Project->surfels.size(), &Project->coverageStagingBuffer)) {
			return false;
		}
	}

	Project->surfelsCoverageViewRenderModel = gfxCreateCoveragePointRenderModel(AppContext, &Project->surfels);

	return true;
}

bool _surfelTracing(ldiApp* AppContext, ldiProjectContext* Project) {
	ldiModel surfelTriModel = {};

	int quadCount = Project->surfels.size();
	int triCount = quadCount * 2;
	int vertCount = quadCount * 4;

	surfelTriModel.verts.resize(vertCount);
	surfelTriModel.indices.resize(triCount * 3);

	for (int i = 0; i < (int)Project->surfels.size(); ++i) {
		ldiNewSurfel* s = &Project->surfels[i];

		surfelTriModel.verts[i * 4 + 0].pos = s->verts[0].position;
		surfelTriModel.verts[i * 4 + 1].pos = s->verts[1].position;
		surfelTriModel.verts[i * 4 + 2].pos = s->verts[2].position;
		surfelTriModel.verts[i * 4 + 3].pos = s->verts[3].position;

		surfelTriModel.indices[i * 6 + 0] = i * 4 + 2;
		surfelTriModel.indices[i * 6 + 1] = i * 4 + 1;
		surfelTriModel.indices[i * 6 + 2] = i * 4 + 0;
		surfelTriModel.indices[i * 6 + 3] = i * 4 + 0;
		surfelTriModel.indices[i * 6 + 4] = i * 4 + 3;
		surfelTriModel.indices[i * 6 + 5] = i * 4 + 2;
	}

	ldiPhysicsMesh cookedSurfels = {};
	physicsCookMesh(AppContext->physics, &surfelTriModel, &cookedSurfels);

	double t0 = getTime();

	float normalAdjust = 0.005f;

	Project->debugPoints.clear();
	Project->debugLineSegs.clear();

	ldiPhysicsCone cone = {};
	if (!physicsCreateCone(AppContext->physics, 21.0f, 0.25f, 12, &cone)) {
		return false;
	}

	for (int i = 0; i < (int)Project->surfels.size(); ++i) {
		ldiNewSurfel* s = &Project->surfels[i];
		vec3 startPos = s->position + s->normal * normalAdjust;

		//ldiRaycastResult result = physicsRaycast(&cookedSurfels, startPos, s->normal, 20.15f);
		//if (result.hit) {
		//	//Project->debugLineSegs.push_back({ startPos, result.pos });

		//	s->verts[0].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[1].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[2].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[3].color = vec3(1.0f, 0.0f, 0.0f);
		//} else {
		//	s->verts[0].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[1].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[2].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[3].color = vec3(0.5f, 0.5f, 0.5f);
		//}

		//if (physicsBeamOverlap(&cookedSurfels, startPos, s->normal, 0.02, 21.0f)) {
		//	//vec3 start = outMat * vec4(0, 0, 0, 1.0f);
		//	//vec3 end = outMat * vec4(1.0f, 0, 0, 1.0f);
		//	//Project->debugLineSegs.push_back({ start, end });
		//	//Project->debugLineSegs.push_back({ startPos, startPos + s->normal * (10.1f + 0.01f) });

		//	s->verts[0].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[1].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[2].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[3].color = vec3(1.0f, 0.0f, 0.0f);
		//} else {
		//	s->verts[0].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[1].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[2].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[3].color = vec3(0.5f, 0.5f, 0.5f);
		//}

		//if (physicsConeOverlap(&cookedSurfels, &cone, startPos, s->normal)) {
		//	//vec3 start = outMat * vec4(0, 0, 0, 1.0f);
		//	//vec3 end = outMat * vec4(1.0f, 0, 0, 1.0f);
		//	//Project->debugLineSegs.push_back({ start, end });
		//	//Project->debugLineSegs.push_back({ startPos, startPos + s->normal * (10.1f + 0.01f) });

		//	s->verts[0].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[1].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[2].color = vec3(1.0f, 0.0f, 0.0f);
		//	s->verts[3].color = vec3(1.0f, 0.0f, 0.0f);
		//} else {
		//	s->verts[0].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[1].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[2].color = vec3(0.5f, 0.5f, 0.5f);
		//	s->verts[3].color = vec3(0.5f, 0.5f, 0.5f);
		//}
	}

	t0 = getTime() - t0;
	std::cout << "Vis query time: " << (t0 * 1000) << " ms\n";

	Project->surfelsGroupRenderModel = gfxCreateQuadSurfelRenderModel(AppContext, &Project->surfels);

	return true;
}

bool projectProcess(ldiApp* AppContext, ldiProjectContext* Project) {
	Project->processed = false;

	if (!Project->surfelsLoaded) {
		return false;
	}

	Project->pointDistrib.points.clear();
	//Project->pointDistribCloud = gfxCreateRenderPointCloud(AppContext, &Project->pointDistrib);

	if (!_surfelCoveragePrep(AppContext, Project)) {
		return false;
	}
	
	Project->processed = true;

	return true;
}

void projectSave(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->path.empty()) {
		std::cout << "Project does not have a file path\n";
		return;
	}

	std::cout << "Saving project: " << Project->path << "\n";

	FILE* file;
	fopen_s(&file, Project->path.c_str(), "wb");
	if (file == 0) {
		std::cout << "Could not open file\n";
		return;
	}

	fwrite(&Project->sourceModelLoaded, sizeof(bool), 1, file);
	if (Project->sourceModelLoaded) {
		serialize(file, &Project->sourceModel);
		fwrite(&Project->sourceModelScale, sizeof(float), 1, file);
		fwrite(&Project->sourceModelTranslate, sizeof(vec3), 1, file);
		fwrite(&Project->sourceModelRotate, sizeof(vec3), 1, file);
	}

	fwrite(&Project->sourceTextureLoaded, sizeof(bool), 1, file);
	if (Project->sourceTextureLoaded) {
		serialize(file, &Project->sourceTextureRaw, 4);
		serialize(file, &Project->sourceTextureCmyk, 4);
	}

	fwrite(&Project->quadModelLoaded, sizeof(bool), 1, file);
	if (Project->quadModelLoaded) {
		serialize(file, &Project->quadModel);
	}

	fwrite(&Project->surfelsLoaded, sizeof(bool), 1, file);
	if (Project->surfelsLoaded) {
		serialize(file, Project->surfels);
		serialize(file, &Project->surfelsSamplesRaw, 4);
	}

	fclose(file);
}

bool projectLoad(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	std::cout << "Loading project: " << Project->path << "\n";

	projectInit(AppContext, Project);
	Project->path = Path;

	FILE* file;
	fopen_s(&file, Path.c_str(), "rb");
	if (file == 0) {
		return false;
	}

	fread(&Project->sourceModelLoaded, sizeof(bool), 1, file);
	if (Project->sourceModelLoaded) {
		deserialize(file, &Project->sourceModel);
		fread(&Project->sourceModelScale, sizeof(float), 1, file);
		fread(&Project->sourceModelTranslate, sizeof(vec3), 1, file);
		fread(&Project->sourceModelRotate, sizeof(vec3), 1, file);
		projectFinalizeImportedModel(AppContext, Project);
	}

	fread(&Project->sourceTextureLoaded, sizeof(bool), 1, file);
	if (Project->sourceTextureLoaded) {
		deserialize(file, &Project->sourceTextureRaw, 4);
		deserialize(file, &Project->sourceTextureCmyk, 4);
		projectFinalizeImportedTexture(AppContext, Project);
	}

	fread(&Project->quadModelLoaded, sizeof(bool), 1, file);
	if (Project->quadModelLoaded) {
		deserialize(file, &Project->quadModel);
		projectFinalizeQuadModel(AppContext, Project);
	}

	fread(&Project->surfelsLoaded, sizeof(bool), 1, file);
	if (Project->surfelsLoaded) {
		deserialize(file, Project->surfels);
		deserialize(file, &Project->surfelsSamplesRaw, 4);
		projectFinalizeSurfels(AppContext, Project);
	}

	fclose(file);

	return true;
}

vec3 _customProj(vec3 P) {
	// Map P from view space to clip space.

	// Z = 0 at near, 1 at far.
	// X = -1 left, 1 right
	// Y = -1 bottom, 1 top
	
	float zNear = 1.0f;
	float zFar = 10.0f;

	float yTop = 2.0f;
	float yBottom = -2.0f;

	float xFov = 11.0f;

	float xRatioHalf = sin(glm::radians(xFov) / 2);

	// Calculate z from P.z
	float z = P.z;
	float Pz = (z - zNear) / (zFar - zNear);

	// Calculate y from P.y
	float y = P.y;
	float Py = 2 * (y - yBottom) / (yTop - yBottom) - 1;

	// Calculate x from P.x
	float Px = (z * xRatioHalf) != 0 ? P.x / (z * xRatioHalf) : 0.0f;

	return vec3(Px, Py, Pz);
}

vec3 _customProjInverse(vec3 P) {
	// Map P from clip space to view space.

	float zNear = 1.0f;
	float zFar = 10.0f;

	float yTop = 2.0f;
	float yBottom = -2.0f;

	float xFov = 11.0f;

	float xRatioHalf = sin(glm::radians(xFov) / 2);

	float z = zNear + (zFar - zNear) * P.z;
	float y = yBottom + (yTop - yBottom) * (P.y * 0.5f + 0.5f);
	float x = (z * xRatioHalf) * P.x;

	return vec3(x, y, z);
}

void projectRenderToolHeadView(ldiApp* AppContext, ldiProjectContext* Project, ldiCamera* Camera) {
	if (!Project->surfelsLoaded) {
		return;
	}

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Project->toolViewSize;
	viewport.Height = Project->toolViewSize;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	AppContext->d3dDeviceContext->RSSetViewports(1, &viewport);

	ID3D11RenderTargetView* renderTargets[] = {
		Project->toolViewTexRtView
	};

	//----------------------------------------------------------------------------------------------------
	// Depth "pre-pass".
	//----------------------------------------------------------------------------------------------------
	float bkgColor[] = { 0.5, 0.5, 0.5, 1.0 };
	UINT clearValue[] = { 0, 0, 0, 0 };

	AppContext->d3dDeviceContext->OMSetRenderTargets(0, renderTargets, Project->toolViewDepthStencilView);
	AppContext->d3dDeviceContext->ClearDepthStencilView(Project->toolViewDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	AppContext->d3dDeviceContext->ClearUnorderedAccessViewUint(Project->coverageBufferUav, clearValue);

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(AppContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;

		mat4 viewModel = Camera->viewMat * Project->workWorldMat;

		constantBuffer->mvp = Camera->projMat * viewModel;
		constantBuffer->color = (glm::inverse(Project->workWorldMat) * glm::inverse(Camera->viewMat))[3];
		AppContext->d3dDeviceContext->Unmap(AppContext->mvpConstantBuffer, 0);
	}

	{
		ldiRenderModel* Model = &Project->quadDebugModel;
		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;

		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(NULL, NULL, 0);
		AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);
		AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
		AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
	}

	//----------------------------------------------------------------------------------------------------
	// Coverage test.
	//----------------------------------------------------------------------------------------------------
	{
		ldiRenderModel* Model = &Project->surfelsCoverageViewRenderModel;
		UINT lgStride = sizeof(ldiCoveragePointVertex);
		UINT lgOffset = 0;

		ID3D11UnorderedAccessView* uavs[] = {
			Project->coverageBufferUav,
		};

		ID3D11ShaderResourceView* psSrvs[] = {
			Project->toolViewDepthStencilSrv,
		};

		AppContext->d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(1, renderTargets, NULL, 1, 1, uavs, 0);
		AppContext->d3dDeviceContext->ClearRenderTargetView(Project->toolViewTexRtView, (float*)&bkgColor);

		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->surfelCoverageInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->VSSetShader(AppContext->surfelCoverageVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->surfelCoveragePixelShader, 0, 0);
		//AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, cbfrs);
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, psSrvs);
		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &AppContext->defaultPointSamplerState);
		AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->noDepthState, 0);
		AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
		AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);

		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, nullSRV);
	}

	{
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->CopyResource(Project->coverageStagingBuffer, Project->coverageBuffer);
		AppContext->d3dDeviceContext->Map(Project->coverageStagingBuffer, 0, D3D11_MAP_READ, 0, &ms);
		int* coverageBufferData = (int*)ms.pData;

		int total = 0;

		for (int i = 0; i < (int)Project->surfels.size(); ++i) {
			if (coverageBufferData[i] != 0) {
				total += 1;
			}
		}

		Project->coverageSurfelCount = total;

		AppContext->d3dDeviceContext->Unmap(Project->coverageStagingBuffer, 0);
	}
}

void projectShowUi(ldiApp* AppContext, ldiProjectContext* Project, ldiCamera* TempCam) {
	ldiCalibrationJob* calibJob = &AppContext->calibJob;

	if (ImGui::Begin("Project inspector", 0, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
		if (ImGui::CollapsingHeader("Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("File path: %s", Project->path.empty() ? "(not saved)" : Project->path.c_str());

			ImGui::Separator();
			ImGui::Text("Calibration");
			ImGui::Text("Initial estimations: %s", calibJob->initialEstimations ? "Yes" : "No");
			ImGui::Text("Metrics calibrated: %s", calibJob->metricsCalculated ? "Yes" : "No");
			ImGui::Text("Scanner calibrated: %s", calibJob->scannerCalibrated ? "Yes" : "No");
			ImGui::Text("Galvo calibrated: %s", calibJob->galvoCalibrated ? "Yes" : "No");

			ImGui::Separator();
			ImGui::Text("Scan: %s", Project->scanLoaded ? "Yes" : "No");

			ImGui::Separator();
			ImGui::Text("Source model: %s", Project->sourceModelLoaded ? "Yes" : "No");
			ImGui::Text("Source texture: %s", Project->sourceTextureLoaded ? "Yes" : "No");
			if (Project->sourceModelLoaded) {
				ImGui::Text("Vertex count: %d", Project->sourceModel.verts.size());
				ImGui::Text("Triangle count: %d", Project->sourceModel.indices.size() / 3);
			}

			ImGui::Separator();
			ImGui::Text("Quad model: %s", Project->quadModelLoaded ? "Yes" : "No");
			if (Project->quadModelLoaded) {
				ImGui::Text("Quad count: %d", Project->quadModel.indices.size() / 4);
			}
			
			ImGui::Separator();
			ImGui::Text("Registered model: %s", Project->registeredModelLoaded ? "Yes" : "No");
			
			ImGui::Separator();
			ImGui::Text("Surfels: %s", Project->surfelsLoaded ? "Yes" : "No");
			if (Project->surfelsLoaded) {
				//ImGui::Checkbox("Show surfels (high res)", &tool->showSurfels);
				//ImGui::Checkbox("Show spatial bounds", &tool->showSpatialBounds);
				//ImGui::Checkbox("Show spatial occupied cells", &tool->showSpatialCells);
			}
		}

		if (ImGui::CollapsingHeader("Source model")) {
			ImGui::Text("Source model");

			ImGui::SliderFloat("Scale", &Project->sourceModelScale, 0.001f, 100.0f);
			ImGui::DragFloat3("Rotation", (float*)&Project->sourceModelRotate, 0.1f);
			ImGui::DragFloat3("Translation", (float*)&Project->sourceModelTranslate, 0.1f, -100.0f, 100.0f);
			if (ImGui::Button("Import model")) {
				std::string filePath;
				if (showOpenFileDialog(AppContext->hWnd, AppContext->currentWorkingDir, filePath, L"OBJ file", L"*.obj")) {
					std::cout << "Import model: " << filePath << "\n";
					projectImportModel(AppContext, Project, filePath.c_str());
				}
			}
		}

		if (ImGui::CollapsingHeader("Source texture")) {
			if (ImGui::Button("Import texture")) {
				std::string filePath;
				if (showOpenFileDialog(AppContext->hWnd, AppContext->currentWorkingDir, filePath, L"PNG file", L"*.png")) {
					std::cout << "Import texture: " << filePath << "\n";
					projectImportTexture(AppContext, Project, filePath.c_str());
				}
			}

			if (Project->sourceTextureLoaded) {
				float w = ImGui::GetContentRegionAvail().x;

				if (ImGui::BeginTabBar("textureChannelsTabs")) {
					if (ImGui::BeginTabItem("sRGB")) {
						ImGui::Image(Project->sourceTextureSrv, ImVec2(w, w));
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("C")) {
						ImGui::Image(Project->sourceTextureCmykSrv[0], ImVec2(w, w));
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("M")) {
						ImGui::Image(Project->sourceTextureCmykSrv[1], ImVec2(w, w));
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Y")) {
						ImGui::Image(Project->sourceTextureCmykSrv[2], ImVec2(w, w));
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("K")) {
						ImGui::Image(Project->sourceTextureCmykSrv[3], ImVec2(w, w));
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			}
		}

		if (ImGui::CollapsingHeader("Quad model")) {
			if (ImGui::Button("Create quad model")) {
				projectCreateQuadModel(AppContext, Project);
			}

			/*if (Project->quadModelLoaded) {
				//ImGui::Checkbox("Canvas model", &tool->showQuadMeshWhite);
				//ImGui::ColorEdit3("Canvas", (float*)&tool->modelCanvasColor);
				//ImGui::Checkbox("Area debug", &tool->quadMeshShowDebug);
				//ImGui::Checkbox("Quad wireframe", &tool->quadMeshShowWireframe);

				if (ImGui::Button("Save point cloud")) {
					std::string filePath;
					if (showSaveFileDialog(tool->appContext->hWnd, tool->appContext->currentWorkingDir, filePath, L"Polygon", L"*.ply", L"ply")) {
						ldiQuadModel* quadModel = &tool->appContext->projectContext->quadModel;
						ldiPointCloud pcl = {};
						pcl.points.resize(quadModel->verts.size());

						for (size_t i = 0; i < quadModel->verts.size(); ++i) {
							pcl.points[i].position = quadModel->verts[i];
						}

						plySavePoints(filePath.c_str(), &pcl);
					}
				}
			}*/
		}

		if (ImGui::CollapsingHeader("Register model")) {
			if (ImGui::Button("Register quad model")) {
				//projectRegisterModel(AppContext, Project);
			}
		}

		if (ImGui::CollapsingHeader("Surfels")) {
			if (ImGui::Button("Create surfels")) {
				projectCreateSurfels(AppContext, Project);
			}

			if (Project->sourceTextureLoaded) {
				float w = ImGui::GetContentRegionAvail().x;
				ImGui::Image(Project->surfelsSamplesTextureSrv, ImVec2(w, w));
			}
		}

		if (ImGui::CollapsingHeader("Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Process")) {
				projectProcess(AppContext, Project);
			}

			ImGui::SliderFloat("Camera FOV X", &Project->fovX, 1.0f, 180.0f);
			ImGui::SliderFloat("Camera FOV Y", &Project->fovY, 1.0f, 180.0f);
		}
	}
	ImGui::End();

	if (Project->processed) {
		if (ImGui::Begin("Tool head view")) {
			ldiCamera orthoCam = {};
			//orthoCam.

			//projectRenderToolHeadView(AppContext, Project, &orthoCam);
			projectRenderToolHeadView(AppContext, Project, &Project->galvoCam);

			ImGui::Image(Project->toolViewTexView, ImVec2(512, 512));

			ImGui::Text("Prime surfels: %d", Project->coverageSurfelCount);
		}
	}
	ImGui::End();
}

