#pragma once

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

const float CAMERA_SPEED = 250.f;
const float CAMERA_ROTATION_SPEED = glm::pi<float>() / 2;
const glm::vec3 UP(0.f, 1.f, 0.f);
const glm::vec3 RGH(1.f, 0.f, 0.f);
const glm::vec3 FWD(0.f, 0.f, 1.f);
const glm::vec3 CLEAR_COLOR(0.8f, 0.8f, 1.0f);
const glm::vec3 AMBIENT_COLOR(0.02f, 0.02f, 0.02f);
const glm::vec3 SUN_COLOR(1.0f, 0.95f, 0.9f);
const glm::vec3 SUN_DIRECTION = UP + RGH;
const glm::vec3 LIGHT_COLOR(0.4f, 0.3f, 0.1f);
const glm::vec3 LIGHT_ATTENUATION(0.4f, 0.f, 0.00005f);
const glm::vec3 LIGHT_POSITION(1250.f, 100.f, 0.f);
const glm::vec3 LIGHT_DELTA(0.f, 0.f, 500.f);
const GLsizei SHADOWMAP_RESOLUTION = 2048;
const GLsizei REFLECTION_CUBEMAP_RESOLUTION = 256;
