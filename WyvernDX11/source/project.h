#pragma once

struct ldiProjectContext {
	std::string					path;

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

	bool						surfelsLoaded = false;
	vec3						surfelsBoundsMin;
	vec3						surfelsBoundsMax;
	std::vector<ldiSurfel>		surfelsLow;
	std::vector<ldiSurfel>		surfelsHigh;
	ldiSpatialGrid				surfelLowSpatialGrid = {};
	ldiRenderModel				surfelLowRenderModel;
	ldiRenderModel				surfelHighRenderModel;
	ldiRenderModel				coveragePointModel;

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
}

void projectInvalidateTextureData(ldiApp* AppContext, ldiProjectContext* Project) {
	// TODO: Destroy other texture resources.

	if (Project->sourceTextureLoaded) {
		Project->sourceTextureLoaded = false;
	}
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
