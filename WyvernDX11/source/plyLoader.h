#pragma once

#include "model.h"

bool plyLoadPoints(const char* FileName, ldiPointCloud* PointCloud);
bool plySavePoints(const char* FileName, ldiPointCloud* PointCloud);
bool plySaveModel(const char* FileName, ldiModel* Model);
bool plyLoadQuadMesh(const char* FileName, ldiQuadModel* Model);
bool plyLoadModel(const char* FileName, ldiModel* Model, bool RegMode = false);
bool plySaveQuadMesh(const std::string& FileName, ldiQuadModel* Model);