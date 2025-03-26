#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

struct SunLight {
    glm::vec3 direction;
    glm::vec3 color;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 attenuation;
};
