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
	vec3						surfelsBoundsMin;
	vec3						surfelsBoundsMax;
	std::vector<ldiSurfel>		surfelsLow;
	std::vector<ldiSurfel>		surfelsHigh;
	ldiSpatialGrid				surfelLowSpatialGrid = {};
	ldiRenderModel				surfelLowRenderModel;
	ldiRenderModel				surfelHighRenderModel;
	ldiRenderModel				coveragePointModel;
	std::vector<ldiNewSurfel>	surfelsNew;
	ldiRenderModel				surfelsNewRenderModel;
	ldiImage					surfelsSamplesRaw;
	ID3D11Texture2D*			surfelsSamplesTexture;
	ID3D11ShaderResourceView*	surfelsSamplesTextureSrv;

	bool						poissonSamplesLoaded = false;
	ldiPoissonSpatialGrid		poissonSpatialGrid = {};
	ldiRenderModel				poissonSamplesRenderModel;
	ldiRenderModel				poissonSamplesCmykRenderModel[4];

	ldiRenderPointCloud			pointCloudRenderModel;
};

mat4 projectGetSourceTransformMat(ldiProjectContext* Project) {
	mat4 result = glm::identity<mat4>();

	result = glm::translate(result, vec3(Project->sourceModelTranslate));
	result = result * glm::toMat4(quat(Project->sourceModelRotate));
	result = glm::scale(result, vec3(Project->sourceModelScale));

	return result;
}

void projectInvalidateModelData(ldiApp* AppContext, ldiProjectContext* Project) {
	// TODO: Change to a general clear?

	if (Project->sourceModelLoaded) {
		Project->sourceModelLoaded = false;
		//sourceRenderModel
	}

	if (Project->quadModelLoaded) {
		Project->quadModelLoaded = false;
		//quadModel
	}

	if (Project->registeredModelLoaded) {
		Project->registeredModelLoaded = false;
	}

	// TODO: Clear surfels.
	// TODO: Clear poisson samples?
}

bool projectImportModel(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	projectInvalidateModelData(AppContext, Project);
	std::cout << "Project source mesh path: " << Path << "\n";

	uint8_t* sourceModelFile = nullptr;
	int sourceModelFileSize = readFile(Path, &sourceModelFile);

	if (sourceModelFileSize < 0) {
		std::cout << "Failed to load source model file " << Path << "\n";
		return false;
	}

	Project->sourceModel = objLoadModel(sourceModelFile, sourceModelFileSize);
	Project->sourceRenderModel = gfxCreateRenderModel(AppContext, &Project->sourceModel);
	Project->sourceModelLoaded = true;

	return true;
}

void projectInvalidateTextureData(ldiApp* AppContext, ldiProjectContext* Project) {
	// TODO: Destroy other texture resources.

	if (Project->sourceTextureLoaded) {
		Project->sourceTextureLoaded = false;
	}
}

bool projectImportTexture(ldiApp* AppContext, ldiProjectContext* Project, const char* Path) {
	projectInvalidateTextureData(AppContext, Project);

	std::cout << "Project source texture path: " << Path << "\n";

	double t0 = getTime();

	int x, y, n;
	uint8_t* imageRawPixels = imageLoadRgba(Path, &x, &y, &n);

	Project->sourceTextureRaw.width = x;
	Project->sourceTextureRaw.height = y;
	Project->sourceTextureRaw.data = imageRawPixels;

	t0 = getTime() - t0;
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ") " << t0 * 1000.0f << " ms\n";

	Project->sourceTextureLoaded = true;

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

	vec3 cmykColor[4] = {
		vec3(0, 166.0 / 255.0, 214.0 / 255.0),
		vec3(1.0f, 0, 144.0 / 255.0),
		vec3(245.0 / 255.0, 230.0 / 255.0, 23.0 / 255.0),
		vec3(0.0f, 0.0f, 0.0f)
	};

	// NOTE: Converted to sRGB for viewing purposes.
	for (int channelIter = 0; channelIter < 4; ++channelIter) {
		ldiImage* image = &Project->sourceTextureCmykChannels[channelIter];

		image->width = x;
		image->height = y;
		image->data = new uint8_t[x * y * 4];

		for (int i = 0; i < x * y; ++i) {
			float v = ((float)cmykImagePixels[i * 4 + channelIter]) / 255.0f;
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

	cmsDeleteTransform(colorTransform);
	cmsCloseProfile(srgbProfile);
	cmsCloseProfile(cmykProfile);

	if (!gfxCreateTextureR8G8B8A8Basic(AppContext, &Project->sourceTextureRaw, &Project->sourceTexture, &Project->sourceTextureSrv)) {
		return false;
	}

	for (int channelIter = 0; channelIter < 4; ++channelIter) {
		ldiImage* image = &Project->sourceTextureCmykChannels[channelIter];

		if (!gfxCreateTextureR8G8B8A8Basic(AppContext, image, &Project->sourceTextureCmykTexture[channelIter], &Project->sourceTextureCmykSrv[channelIter])) {
			return false;
		}
	}

	return true;
}

void projectInit(ldiApp* AppContext, ldiProjectContext* Project) {
	projectInvalidateModelData(AppContext, Project);
	projectInvalidateTextureData(AppContext, Project);
	*Project = {};
}

void projectSave(ldiApp* AppContext, ldiProjectContext* Project) {
	if (Project->path.empty()) {
		std::cout << "Project does not have a file path\n";
		return;
	}

	std::cout << "Saving project: " << Project->path << "\n";

	FILE* file;
	fopen_s(&file, Project->path.c_str(), "wb");

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

	}

	fwrite(&Project->surfelsLoaded, sizeof(bool), 1, file);
	if (Project->surfelsLoaded) {

	}

	fclose(file);
}

bool projectLoad(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	std::cout << "Loading project: " << Project->path << "\n";

	projectInit(AppContext, Project);
	Project->path = Path;

	FILE* file;
	fopen_s(&file, Path.c_str(), "rb");

	fclose(file);

	//char pathStr[1024];
	//sprintf_s(pathStr, "%ssource_mesh.obj", Project->dir.c_str());
	//projectLoadModel(AppContext, Project, pathStr);

	//// TODO: Check if processed model exists.
	//sprintf_s(pathStr, "%squad.ply", Project->dir.c_str());
	//if (!plyLoadQuadMesh(pathStr, &Project->quadModel)) {
	//	return 1;
	//}

	//geoPrintQuadModelInfo(&Project->quadModel);

	return true;
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

static bool projectLoadQuadModel(ldiApp* AppContext, ldiProjectContext* Project, const std::string& Path) {
	Project->quadModelLoaded = false;

	if (!plyLoadQuadMesh(Path.c_str(), &Project->quadModel)) {
		return false;
	}

	Project->quadDebugModel = gfxCreateRenderQuadModelDebug(AppContext, &Project->quadModel);
	// TODO: Share verts and normals.
	Project->quadModelWhite = gfxCreateRenderQuadModelWhite(AppContext, &Project->quadModel, vec3(0.9f, 0.9f, 0.9f));
	Project->quadMeshWire = gfxCreateRenderQuadWireframe(AppContext, &Project->quadModel);
	Project->quadModelLoaded = true;

	_printQuadModelInfo(&Project->quadModel);

	return true;
}

static bool projectCreateQuadModel(ldiApp* AppContext, ldiProjectContext* Project) {
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

	return true;
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

//struct ldiColorTransferThreadContext {
//	ldiPhysics* physics;
//	ldiPhysicsMesh* cookedMesh;
//	ldiModel* srcModel;
//	ldiImage* image;
//	std::vector<ldiSurfel>* surfels;
//	int	startIdx;
//	int	endIdx;
//};
//
//void geoTransferThreadBatch(ldiColorTransferThreadContext Context) {
//	float normalAdjust = 0.01;
//
//	for (size_t i = Context.startIdx; i < Context.endIdx; ++i) {
//		ldiSurfel* s = &(*Context.surfels)[i];
//		ldiRaycastResult result = physicsRaycast(Context.cookedMesh, s->position + s->normal * normalAdjust, -s->normal, 0.1f);
//
//		if (result.hit) {
//			ldiMeshVertex v0 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 0]];
//			ldiMeshVertex v1 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 1]];
//			ldiMeshVertex v2 = Context.srcModel->verts[Context.srcModel->indices[result.faceIdx * 3 + 2]];
//
//			float u = result.barry.x;
//			float v = result.barry.y;
//			float w = 1.0 - (u + v);
//			vec2 uv = w * v0.uv + u * v1.uv + v * v2.uv;
//
//			vec4 s0 = getColorSampleBilinear(Context.image, uv);
//			
//			//s->color = vec4(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
//			//s->color = vec4(s0.r / 255.0, s0.g / 255.0, s0.b / 255.0, s0.a / 255.0);
//			s->color = s0;
//		} else {
//			s->color = vec4(1, 0, 0, 0);
//		}
//	}
//}
//
//void geoTransferColorToSurfels(ldiApp* AppContext, ldiPhysicsMesh* CookedMesh, ldiModel* SrcModel, ldiImage* Image, std::vector<ldiSurfel>* Surfels) {
//	double t0 = getTime();
//
//	const int threadCount = 20;
//	int batchSize = Surfels->size() / threadCount;
//	int batchRemainder = Surfels->size() - (batchSize * threadCount);
//	std::thread workerThread[threadCount];
//
//	for (int t = 0; t < threadCount; ++t) {
//		ldiColorTransferThreadContext tc{};
//		tc.physics = AppContext->physics;
//		tc.cookedMesh = CookedMesh;
//		tc.srcModel = SrcModel;
//		tc.image = Image;
//		tc.surfels = Surfels;
//		tc.startIdx = t * batchSize;
//		tc.endIdx = (t + 1) * batchSize;
//
//		if (t == threadCount - 1) {
//			tc.endIdx += batchRemainder;
//		}
//
//		workerThread[t] = std::thread(geoTransferThreadBatch, tc);
//	}
//
//	for (int t = 0; t < threadCount; ++t) {
//		workerThread[t].join();
//	}
//
//	t0 = getTime() - t0;
//	std::cout << "Transfer color: " << t0 * 1000.0f << " ms\n";
//}

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

	for (int t = 0; t < threadCount; ++t) {
		ldiColorTransferThreadContext tc{};
		tc.physics = AppContext->physics;
		tc.cookedMesh = CookedMesh;
		tc.srcModel = SrcModel;
		tc.image = Image;
		tc.samplesImage = SamplesImage;
		tc.surfels = Surfels;
		tc.samplesPerSide = 4;
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
	std::cout << "Transfer color: " << t0 * 1000.0f << " ms\n";
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

bool projectCreateSurfels(ldiApp* AppContext, ldiProjectContext* Project) {
	if (!Project->quadModelLoaded || !Project->sourceTextureLoaded) {
		return false;
	}

	Project->surfelsLoaded = false;

	Project->surfelsSamplesRaw.width = 4096;
	Project->surfelsSamplesRaw.height = Project->surfelsSamplesRaw.width;
	
	if (Project->surfelsSamplesRaw.data) {
		delete Project->surfelsSamplesRaw.data;
	}

	Project->surfelsSamplesRaw.data = new uint8_t[Project->surfelsSamplesRaw.width * Project->surfelsSamplesRaw.width * 4];
	memset(Project->surfelsSamplesRaw.data, 0, Project->surfelsSamplesRaw.width * Project->surfelsSamplesRaw.width * 4);

	geoCreateSurfelsNew(&Project->quadModel, &Project->surfelsNew, Project->surfelsSamplesRaw.width, 4);
	geoCreateSurfels(&Project->quadModel, &Project->surfelsLow);
	geoCreateSurfelsHigh(&Project->quadModel, &Project->surfelsHigh);
	std::cout << "High res surfel count: " << Project->surfelsHigh.size() << "\n";

	physicsCookMesh(AppContext->physics, &Project->sourceModel, &Project->sourceCookedModel);
	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureCmyk, &Project->surfelsHigh);
	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureRaw, &Project->surfelsHigh);

	geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureCmyk, &Project->surfelsNew, &Project->surfelsSamplesRaw);
	//geoTransferColorToSurfels(AppContext, &Project->sourceCookedModel, &Project->sourceModel, &Project->sourceTextureRaw, &Project->surfelsNew, &Project->surfelsSamplesRaw);

	//imageWrite("samplestest.png", 4096, 4096, 4, 4096 * 4, Project->surfelsSamplesRaw.data);

	gfxCreateTextureR8G8B8A8Basic(AppContext, &Project->surfelsSamplesRaw, &Project->surfelsSamplesTexture, &Project->surfelsSamplesTextureSrv);

	//----------------------------------------------------------------------------------------------------
	// Create spatial structure for surfels.
	//----------------------------------------------------------------------------------------------------
	double t0 = getTime();
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
	std::cout << "Build spatial grid: " << t0 * 1000.0f << " ms\n";

	//----------------------------------------------------------------------------------------------------
	// Smooth normals.
	//----------------------------------------------------------------------------------------------------
	t0 = getTime();
	{
		const int threadCount = 20;
		int batchSize = Project->surfelsLow.size() / threadCount;
		int batchRemainder = Project->surfelsLow.size() - (batchSize * threadCount);
		std::thread workerThread[threadCount];

		for (int n = 0; n < 4; ++n) {
			for (int t = 0; t < threadCount; ++t) {
				ldiSmoothNormalsThreadContext tc{};
				tc.grid = &spatialGrid;
				tc.surfels = &Project->surfelsLow;
				tc.startIdx = t * batchSize;
				tc.endIdx = (t + 1) * batchSize;

				if (t == threadCount - 1) {
					tc.endIdx += batchRemainder;
				}

				workerThread[t] = std::move(std::thread(surfelsSmoothNormalsThreadBatch, tc));
			}

			for (int t = 0; t < threadCount; ++t) {
				workerThread[t].join();
			}

			// Apply smoothed normal back to surfel normal.
			for (int i = 0; i < Project->surfelsLow.size(); ++i) {
				ldiSurfel* srcSurfel = &Project->surfelsLow[i];
				srcSurfel->normal = srcSurfel->smoothedNormal;
			}
		}
	}
	t0 = getTime() - t0;
	std::cout << "Normal smoothing: " << t0 * 1000.0f << " ms\n";

	Project->surfelLowRenderModel = gfxCreateSurfelRenderModel(AppContext, &Project->surfelsLow);
	Project->surfelHighRenderModel = gfxCreateSurfelRenderModel(AppContext, &Project->surfelsHigh, 0.001f, 0);
	Project->coveragePointModel = gfxCreateCoveragePointRenderModel(AppContext, &Project->surfelsLow, &Project->quadModel);
	Project->surfelsNewRenderModel = gfxCreateNewSurfelRenderModel(AppContext, &Project->surfelsNew);

	Project->surfelsLoaded = true;

	return true;
}

void projectShowUi(ldiApp* AppContext, ldiProjectContext* Project) {
	ldiCalibrationJob* calibJob = &AppContext->calibJob;

	if (ImGui::Begin("Project inspector")) {
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

		if (ImGui::CollapsingHeader("Source model", ImGuiTreeNodeFlags_DefaultOpen)) {
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

		if (ImGui::CollapsingHeader("Source texture", ImGuiTreeNodeFlags_DefaultOpen)) {
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
		}
	}
	ImGui::End();
}
