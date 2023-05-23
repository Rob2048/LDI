#include "stlLoader.h"
#include <iostream>

bool stlSaveModel(const char* FileName, ldiModel* Model) {
	std::cout << "Save STL file to: " << FileName << "\n";

	uint32_t triCount = Model->indices.size() / 3;

	if (triCount * 3 != Model->indices.size()) {
		std::cout << "stlSaveModel model needs to be triangles.\n";
		return false;
	}

	FILE* file;
	if (fopen_s(&file, FileName, "wb") != 0) {
		std::cout << "stlSaveModel failed to open file: " << FileName << "\n";
		return false;
	}

	char header[80] = {};
	fwrite(header, sizeof(header), 1, file);

	fwrite(&triCount, sizeof(triCount), 1, file);

	for (int i = 0; i < triCount; ++i) {
		ldiMeshVertex v0 = Model->verts[Model->indices[i * 3 + 0]];
		ldiMeshVertex v1 = Model->verts[Model->indices[i * 3 + 1]];
		ldiMeshVertex v2 = Model->verts[Model->indices[i * 3 + 2]];

		// TODO: Should be face normal, but just using v0.
		// NOTE: STL importers seem to completely ignore this and just flat shade anyway?
		fwrite(&v0.normal.x, sizeof(float), 1, file);
		fwrite(&v0.normal.y, sizeof(float), 1, file);
		fwrite(&v0.normal.z, sizeof(float), 1, file);

		fwrite(&v2.pos.x, sizeof(float), 1, file);
		fwrite(&v2.pos.y, sizeof(float), 1, file);
		fwrite(&v2.pos.z, sizeof(float), 1, file);

		fwrite(&v1.pos.x, sizeof(float), 1, file);
		fwrite(&v1.pos.y, sizeof(float), 1, file);
		fwrite(&v1.pos.z, sizeof(float), 1, file);

		fwrite(&v0.pos.x, sizeof(float), 1, file);
		fwrite(&v0.pos.y, sizeof(float), 1, file);
		fwrite(&v0.pos.z, sizeof(float), 1, file);

		uint16_t attrCount = 0;
		fwrite(&attrCount, sizeof(attrCount), 1, file);
	}

	fclose(file);

	return true;
}