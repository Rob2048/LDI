#pragma once

struct ldiScan {
	ldiPointCloud				pointCloud = {};
	ldiRenderPointCloud			pointCloudRenderModel = {};
};

void scanUpdateInternalPoints(ldiApp* AppContext, ldiScan* Scan) {
	if (Scan->pointCloudRenderModel.vertexBuffer) {
		Scan->pointCloudRenderModel.vertexBuffer->Release();
	}

	if (Scan->pointCloudRenderModel.indexBuffer) {
		Scan->pointCloudRenderModel.indexBuffer->Release();
	}

	Scan->pointCloudRenderModel = gfxCreateRenderPointCloud(AppContext, &Scan->pointCloud);
}

void scanUpdatePoints(ldiApp* AppContext, ldiScan* Scan, const std::vector<ldiPointCloudVertex>& Points) {
	Scan->pointCloud.points = Points;
	scanUpdateInternalPoints(AppContext, Scan);
}

void scanUpdatePoints(ldiApp* AppContext, ldiScan* Scan, const std::vector<vec3>& Points) {
	std::vector<ldiPointCloudVertex> points;
	points.resize(Points.size());

	for (size_t pointIter = 0; pointIter < Points.size(); ++pointIter) {
		ldiPointCloudVertex vert;
		vert.position = Points[pointIter];
		vert.color = vert.position / 10.0f + 0.5f;
		vert.normal = vec3(1, 0, 0);

		points[pointIter] = vert;
	}

	scanUpdatePoints(AppContext, Scan, points);
}

void scanClearPoints(ldiApp* AppContext, ldiScan* Scan) {
	std::vector<ldiPointCloudVertex> points;
	scanUpdatePoints(AppContext, Scan, points);
}

void scanSaveFile(ldiScan* Scan, const std::string& Filename) {
	std::cout << "Save scan file: " << Filename << "\n";

	FILE* f;
	fopen_s(&f, Filename.c_str(), "wb");

	int pointCount = Scan->pointCloud.points.size();
	fwrite(&pointCount, sizeof(int), 1, f);

	for (auto& point : Scan->pointCloud.points) {
		fwrite(&point, sizeof(point), 1, f);
	}

	fclose(f);
}

void scanLoadFile(ldiApp* AppContext, ldiScan* Scan, const std::string& Filename) {
	std::cout << "Load scan file: " << Filename << "\n";

	FILE* f;
	fopen_s(&f, Filename.c_str(), "rb");

	int pointCount;
	fread(&pointCount, sizeof(int), 1, f);

	std::cout << "Points: " << pointCount << "\n";

	Scan->pointCloud.points.resize(pointCount);
	for (auto& point : Scan->pointCloud.points) {
		fread(&point, sizeof(point), 1, f);
	}

	/*for (int i = 0; i < pointCount; ++i) {
		fread(&Scan->pointCloud.points[i], sizeof(ldiPointCloudVertex), 1, f);
	}*/

	fclose(f);

	scanUpdateInternalPoints(AppContext, Scan);
}