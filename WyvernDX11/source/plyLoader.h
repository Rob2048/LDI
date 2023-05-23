#pragma once

#include "model.h"

bool plyLoadPoints(const char* FileName, ldiPointCloud* PointCloud);
bool plySaveModel(const char* FileName, ldiModel* Model);
bool plyLoadQuadMesh(const char* FileName, ldiQuadModel* Model);
bool plyLoadModel(const char* FileName, ldiModel* Model);
