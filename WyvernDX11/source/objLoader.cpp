#include "objLoader.h"
#include <iostream>

float _ParseInt(char* Buffer, int* Index, int Length) {
	int part = 0;
	bool neg = false;

	int ret;

	while (*Index < Length && (Buffer[*Index] > '9' || Buffer[*Index] < '0') && Buffer[*Index] != '-')
		(*Index)++;

	// sign
	if (Buffer[*Index] == '-')
	{
		neg = true;
		(*Index)++;
	}

	// integer part
	while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		part = part * 10 + (Buffer[(*Index)++] - '0');

	ret = neg ? (part * -1) : part;
	return ret;
}

float _ParseFloat(char* Buffer, int* Index, int Length) {
	int part = 0;
	bool neg = false;

	float ret;

	// find start
	while (*Index < Length && (Buffer[*Index] < '0' || Buffer[*Index] > '9') && Buffer[*Index] != '-' && Buffer[*Index] != '.')
		(*Index)++;

	// sign
	if (Buffer[*Index] == '-')
	{
		neg = true;
		(*Index)++;
	}

	// integer part
	while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		part = part * 10 + (Buffer[(*Index)++] - '0');

	ret = neg ? (float)(part * -1) : (float)part;

	// float part
	if (*Index < Length && Buffer[*Index] == '.')
	{
		(*Index)++;
		double mul = 1;
		part = 0;

		while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		{
			part = part * 10 + (Buffer[*Index] - '0');
			mul *= 10;
			(*Index)++;
		}

		if (neg)
			ret -= (float)part / (float)mul;
		else
			ret += (float)part / (float)mul;

	}

	// scientific part
	if (*Index < Length && (Buffer[*Index] == 'e' || Buffer[*Index] == 'E'))
	{
		(*Index)++;
		neg = (Buffer[*Index] == '-'); *Index++;
		part = 0;
		while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		{
			part = part * 10 + (Buffer[(*Index)++] - '0');
		}

		if (neg)
			ret /= (float)pow(10.0, (double)part);
		else
			ret *= (float)pow(10.0, (double)part);
	}

	return ret;
}

bool _getNextChar(char* FileBuffer, int FileSize, int* Position, char* C) {
	if (*Position >= FileSize) {
		return false;
	}

	*C = FileBuffer[*Position];
	*Position += 1;

	return true;
}

struct HashNode
{
	int			vertId;
	HashNode*	next;
};

#define HASHMAP_SIZE	64007

HashNode** hashMap = new HashNode*[HASHMAP_SIZE];
HashNode* hashNodes = new HashNode[4000000];
int hashNodeCount = 0;

ldiMeshVertex*	verts = new ldiMeshVertex[4000000];
vec3*			tempVerts = new vec3[4000000];
vec2*			tempUVs = new vec2[4000000];
vec3*			tempNormals = new vec3[4000000];
uint32_t*		tris = new uint32_t[8000000];

int			tempVertCount = 0;
int			tempUVCount = 0;
int			tempNormalCount = 0;
int			triCount = 0;
int			vertCount = 0;

int _GetUniqueVert(char* Buffer, int* Index, int Length)
{
	int vertId = _ParseInt(Buffer, Index, Length); (*Index)++;
	int uvId = _ParseInt(Buffer, Index, Length); (*Index)++;
	int normalId = _ParseInt(Buffer, Index, Length);

	uint32_t hash = 0;
	hash += ((int*)(tempVerts + (vertId - 1)))[0];
	hash += ((int*)(tempVerts + (vertId - 1)))[1];
	hash += ((int*)(tempVerts + (vertId - 1)))[2];

	hash += ((int*)(tempUVs + (uvId - 1)))[0];
	hash += ((int*)(tempUVs + (uvId - 1)))[1];

	hash += ((int*)(tempNormals + (normalId - 1)))[0];
	hash += ((int*)(tempNormals + (normalId - 1)))[1];
	hash += ((int*)(tempNormals + (normalId - 1)))[2];

	hash %= HASHMAP_SIZE;

	// See if hash exists
	int vertIndex = -1;
	HashNode* next = hashMap[hash];
	while (next != NULL)
	{
		if (verts[next->vertId].pos == tempVerts[vertId - 1] &&
			verts[next->vertId].uv == tempUVs[uvId - 1] &&
			verts[next->vertId].normal == tempNormals[normalId - 1])
		{
			vertIndex = next->vertId;
			break;
		}
		else
		{
			next = next->next;
		}
	}

	if (vertIndex == -1)
	{
		verts[vertCount].pos = tempVerts[vertId - 1];
		verts[vertCount].uv = tempUVs[uvId - 1];
		verts[vertCount].normal = tempNormals[normalId - 1];
		
		HashNode* hashNode = &hashNodes[hashNodeCount++];
		hashNode->next = hashMap[hash];
		hashNode->vertId = vertCount;
		hashMap[hash] = hashNode;

		return vertCount++;
	}
	
	return vertIndex;
}

ldiModel objLoadModel(uint8_t* Data, int Size) {
	ldiModel result = {};

	tempVertCount = 0;
	tempUVCount = 0;
	tempNormalCount = 0;
	triCount = 0;
	vertCount = 0;
	memset(hashMap, 0, sizeof(hashMap) * HASHMAP_SIZE);
	
	int fileLen = Size;
	char* fileBuffer = (char*)Data;
	
	int idx = 0;
	char c = 0;
	_getNextChar(fileBuffer, fileLen, &idx, &c);

	while (true)
	{
		if (c == 'v')
		{
			if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }

			if (c == ' ')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;
				//attr.x = -attr.x;

				tempVerts[tempVertCount++] = attr;

				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}
			else if (c == 't')
			{
				vec2 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				
				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				
				tempUVs[tempUVCount++] = attr;

				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}
			else if (c == 'n')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				
				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;
				
				tempNormals[tempNormalCount++] = attr;

				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}
		}
		else if (c == 'f')
		{
			if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }

			int rootVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
			int currVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
			
			if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }

			while (c == ' ')
			{
				int nextVertId = currVertId;
				currVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
				tris[triCount++] = rootVertId;
				tris[triCount++] = currVertId;
				tris[triCount++] = nextVertId;
				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}
		}
		else
		{
			while (c != '\n') {
				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}

			if (c == '\n') {
				if (!_getNextChar(fileBuffer, fileLen, &idx, &c)) { break; }
			}
		}
	}

	result.verts.resize(vertCount);
	memcpy(result.verts.data(), verts, sizeof(ldiMeshVertex) * vertCount);
	result.indices.resize(triCount);
	memcpy(result.indices.data(), tris, sizeof(uint32_t) * triCount);

	std::cout << "Loaded OBJ - Verts: " << vertCount << " Tris: " << (triCount / 3) << "\n";

	return result;
}

ldiModel objLoadModel(const std::string& FileName) {
	FILE* file;
	fopen_s(&file, FileName.c_str(), "rb");
	fseek(file, 0, SEEK_END);
	int fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	uint8_t* fileBuffer = new uint8_t[fileLen + 1];
	fread(fileBuffer, 1, fileLen, file);
	fileBuffer[fileLen] = 0;

	ldiModel result = objLoadModel(fileBuffer, fileLen);

	delete[] fileBuffer;
	fclose(file);

	return result;
}

ldiModel objLoadQuadModel(const char* FileName) {
	ldiModel result = {};

	tempVertCount = 0;
	tempUVCount = 0;
	tempNormalCount = 0;
	triCount = 0;
	vertCount = 0;
	memset(hashMap, 0, sizeof(hashMap) * HASHMAP_SIZE);

	FILE* file;
	fopen_s(&file, FileName, "rb");
	fseek(file, 0, SEEK_END);
	int fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fileBuffer = new char[fileLen + 1];
	fread(fileBuffer, 1, fileLen, file);
	fileBuffer[fileLen] = 0;

	int idx = 0;
	char c = fileBuffer[idx++];

	while (c != 0)
	{
		if (c == 'v')
		{
			c = fileBuffer[idx++];

			if (c == ' ')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;
				//attr.x = -attr.x;

				tempVerts[tempVertCount++] = attr;
			}
			else if (c == 't')
			{
				vec2 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;

				tempUVs[tempUVCount++] = attr;
			}
			else if (c == 'n')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;

				tempNormals[tempNormalCount++] = attr;
			}
		}
		else if (c == 'f')
		{
			c = fileBuffer[idx++];
			int vertCount = 0;

			int v0 = _GetUniqueVert(fileBuffer, &idx, fileLen);
			int v1 = _GetUniqueVert(fileBuffer, &idx, fileLen);
			int v2 = _GetUniqueVert(fileBuffer, &idx, fileLen);
			int v3 = _GetUniqueVert(fileBuffer, &idx, fileLen);
			c = fileBuffer[idx++];

			tris[triCount++] = v0;
			tris[triCount++] = v1;
			tris[triCount++] = v2;
			tris[triCount++] = v3;
		}
		else
		{
			while (c != '\n' && c != 0)
				c = fileBuffer[idx++];

			if (c == '\n')
				c = fileBuffer[idx++];
		}
	}

	delete[] fileBuffer;
	fclose(file);

	result.verts.resize(vertCount);
	memcpy(result.verts.data(), verts, sizeof(ldiMeshVertex) * vertCount);
	result.indices.resize(triCount);
	memcpy(result.indices.data(), tris, sizeof(uint32_t) * triCount);

	/*result.verts = verts;
	result.indices = tris;
	result.vertCount = vertCount;
	result.indexCount = triCount;*/

	std::cout << "Loaded OBJ Quads:" << FileName << " Verts: " << vertCount << " Quads: " << (triCount / 4) << "\n";

	return result;
}