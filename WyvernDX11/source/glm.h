#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

typedef glm::vec2 vec2;
typedef glm::f64vec2 vec2d;
typedef glm::vec3 vec3;
typedef glm::f64vec3 vec3d;
typedef glm::vec4 vec4;
typedef glm::f64vec4 vec4d;
typedef glm::mat4 mat4;
typedef glm::f64mat4 mat4d;
typedef glm::quat quat;

typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

const vec3 vec3Forward(0, 0, -1);
const vec3 vec3Right(1, 0, 0);
const vec3 vec3Up(0, 1, 0);
const vec3 vec3Zero(0, 0, 0);
const vec3 vec3One(1, 1, 1);