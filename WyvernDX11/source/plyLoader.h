#pragma once

#include "model.h"

bool plyLoadPoints(const char* FileName, ldiPointCloud* PointCloud);
bool plyLoadQuadMesh(const char* FileName, ldiQuadModel* Model);
