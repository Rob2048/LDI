#include "plyLoader.h"
#include <iostream>

// Read alpha numeric characters until space or newline.
bool _compareWord(char* Data, const char* Word) {
	int count = 0;

	while (true) {
		char s = Data[count];
		char t = Word[count];

		// Skip whitespace.

		// End of target word.
		if (t == 0) {
			if (s == '\n' || s == 0 || s == ' ') {
				return true;
			}
			
			return false;
		} else {
			if (t != s) {
				return false;
			}
		}

		++count;
	}
}

char* _readInt(char* Data, int* Value) {
	char* result = Data;
	char buffer[64];
	int bufferSize = 0;

	while (true) {
		char c = result[0];

		if (c == 0) {
			break;
		} else if (c == ' ') {
			++result;
			break;
		} else if (c == '\n') {
			break;
		} else {
			buffer[bufferSize++] = c;
		}

		++result;
	}

	buffer[bufferSize] = 0;
	*Value = atoi(buffer);

	return result;
}

char* _advanceWord(char* Data) {
	char* result = Data;

	while (true) {
		char c = result[0];

		if (c == 0) {
			break;
		} else if (c == ' ') {
			++result;
			break;
		} else if (c == '\n') {
			break;
		}

		++result;
	}

	return result;
}

char* _advanceLine(char* Data) {
	char* result = Data;
	
	while (true) {
		if (result[0] == 0) {
			break;
		} else if (result[0] == '\n') {
			++result;
			break;
		}

		++result;
	}

	return result;
}

char* _readLine(char* Data, char* Buffer) {
	char* result = Data;
	int bufferLen = 0;

	while (true) {
		if (result[0] == 0) {
			break;
		}
		else if (result[0] == '\n') {
			++result;
			break;
		} else {
			Buffer[bufferLen++] = result[0];
		}

		++result;
	}

	Buffer[bufferLen] = 0;

	return result;
}

uint8_t _readUint8(char** Data) {
	uint8_t result;

	memcpy(&result, *Data, sizeof(uint8_t));
	*Data += sizeof(uint8_t);

	return result;
}

float _readFloatLE(char** Data) {
	float result;

	memcpy(&result, *Data, sizeof(float));
	*Data += sizeof(float);

	return result;
}

uint32_t _readUint32LE(char** Data) {
	uint32_t result;

	memcpy(&result, *Data, sizeof(uint32_t));
	*Data += sizeof(uint32_t);

	return result;
}

bool plyLoadPoints(const char* FileName, ldiPointCloud* PointCloud) {
	PointCloud->points.clear();

	std::cout << "Loading point cloud: " << FileName << "\n";

	FILE* file;
	if (fopen_s(&file, FileName, "rb") != 0) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	int fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fileBuffer = new char[fileLen + 1];
	fread(fileBuffer, 1, fileLen, file);
	fileBuffer[fileLen] = 0;
	fclose(file);

	std::cout << "File size: " << (fileLen / 1024.0 / 1024.0) << " MB\n";

	int elementCount = 0;

	char* dataOffset = fileBuffer;

	// Read header
	if (!_compareWord(dataOffset, "ply")) {
		std::cout << "Not a valid PLY file\n";
		return false;
	}

	char lineBuffer[256];

	dataOffset = _advanceLine(dataOffset);

	while (true) {
		if (_compareWord(dataOffset, "end_header")) {
			std::cout << "Found end_header\n";
			dataOffset = _advanceLine(dataOffset);
			break;
		} else if (_compareWord(dataOffset, "format")) {
			std::cout << "Format\n";

			dataOffset = _advanceWord(dataOffset);

			if (_compareWord(dataOffset, "ascii")) {
				std::cout << "ASCII\n";
			} else if (_compareWord(dataOffset, "binary_little_endian")) {
				std::cout << "Binary little endian" << "\n";
			} else if (_compareWord(dataOffset, "binary_big_endian")) {
				std::cout << "Binary big endian" << "\n";
			}

			dataOffset = _advanceLine(dataOffset);
		} else if (_compareWord(dataOffset, "comment")) {
			std::cout << "Comment\n";
			dataOffset = _advanceLine(dataOffset);
		} else if (_compareWord(dataOffset, "element")) {
			
			dataOffset = _advanceWord(dataOffset);

			if (_compareWord(dataOffset, "vertex")) {
				dataOffset = _advanceWord(dataOffset);
				dataOffset = _readInt(dataOffset, &elementCount);

				std::cout << "Elements: " << elementCount << "\n";
			}

			dataOffset = _advanceLine(dataOffset);
		} else {
			dataOffset = _readLine(dataOffset, lineBuffer);
			std::cout << "Line str: " << lineBuffer << "\n";
		}
	}

	PointCloud->points.resize(elementCount);

	for (int i = 0; i < elementCount; ++i) {
		float x = _readFloatLE(&dataOffset);
		float y = _readFloatLE(&dataOffset);
		float z = _readFloatLE(&dataOffset);

		ldiPointCloudVertex vert = {};
		vert.position = vec3(x, y, z);
		vert.color = vec3(x / 5.0f, y / 10.0f, z / 5.0f);

		PointCloud->points[i] = vert;
	}

	delete[] fileBuffer;

	return true;
}

bool plyLoadQuadMesh(const char* FileName, ldiQuadModel* Model) {
	std::cout << "Loading point cloud: " << FileName << "\n";

	FILE* file;
	if (fopen_s(&file, FileName, "rb") != 0) {
		return false;
	}

	fseek(file, 0, SEEK_END);
	int fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fileBuffer = new char[fileLen + 1];
	fread(fileBuffer, 1, fileLen, file);
	fileBuffer[fileLen] = 0;
	fclose(file);

	std::cout << "File size: " << (fileLen / 1024.0 / 1024.0) << " MB\n";

	int vertCount = 0;
	int quadCount = 0;

	char* dataOffset = fileBuffer;

	// Read header
	if (!_compareWord(dataOffset, "ply")) {
		std::cout << "Not a valid PLY file\n";
		return false;
	}

	char lineBuffer[256];

	dataOffset = _advanceLine(dataOffset);

	while (true) {
		if (_compareWord(dataOffset, "end_header")) {
			std::cout << "Found end_header\n";
			dataOffset = _advanceLine(dataOffset);
			break;
		}
		else if (_compareWord(dataOffset, "format")) {
			std::cout << "Format\n";

			dataOffset = _advanceWord(dataOffset);

			if (_compareWord(dataOffset, "ascii")) {
				std::cout << "ASCII\n";
			}
			else if (_compareWord(dataOffset, "binary_little_endian")) {
				std::cout << "Binary little endian" << "\n";
			}
			else if (_compareWord(dataOffset, "binary_big_endian")) {
				std::cout << "Binary big endian" << "\n";
			}

			dataOffset = _advanceLine(dataOffset);
		}
		else if (_compareWord(dataOffset, "comment")) {
			std::cout << "Comment\n";
			dataOffset = _advanceLine(dataOffset);
		}
		else if (_compareWord(dataOffset, "element")) {

			dataOffset = _advanceWord(dataOffset);

			if (_compareWord(dataOffset, "vertex")) {
				dataOffset = _advanceWord(dataOffset);
				dataOffset = _readInt(dataOffset, &vertCount);

				std::cout << "Verts: " << vertCount << "\n";
			} else if (_compareWord(dataOffset, "face")) {
				dataOffset = _advanceWord(dataOffset);
				dataOffset = _readInt(dataOffset, &quadCount);

				std::cout << "Faces: " << quadCount << "\n";
			}

			dataOffset = _advanceLine(dataOffset);
		}
		else {
			dataOffset = _readLine(dataOffset, lineBuffer);
			std::cout << "Line str: " << lineBuffer << "\n";
		}
	}

	Model->verts.resize(vertCount);
	Model->indices.resize(quadCount * 4);

	for (int i = 0; i < vertCount; ++i) {
		float x = _readFloatLE(&dataOffset);
		float y = _readFloatLE(&dataOffset);
		float z = _readFloatLE(&dataOffset);

		Model->verts[i] = vec3(x, y, z);
	}

	for (int i = 0; i < quadCount; ++i) {
		uint8_t faceVertCount = _readUint8(&dataOffset);

		if (faceVertCount != 4) {
			std::cout << "Quad mesh PLY should only contain quads\n";
			return false;
		}

		Model->indices[i * 4 + 0] = _readUint32LE(&dataOffset);
		Model->indices[i * 4 + 1] = _readUint32LE(&dataOffset);
		Model->indices[i * 4 + 2] = _readUint32LE(&dataOffset);
		Model->indices[i * 4 + 3] = _readUint32LE(&dataOffset);

		float nX = _readFloatLE(&dataOffset);
		float nY = _readFloatLE(&dataOffset);
		float nZ = _readFloatLE(&dataOffset);
	}

	delete[] fileBuffer;
	  
	return true;
}