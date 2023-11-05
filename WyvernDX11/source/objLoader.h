#pragma once

#include <string>
#include "model.h"

ldiModel objLoadModel(uint8_t* Data, int Size);
ldiModel objLoadModel(const std::string& FileName);
ldiModel objLoadQuadModel(const char* FileName);