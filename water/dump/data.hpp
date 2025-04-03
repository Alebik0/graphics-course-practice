#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <array>

struct Camera {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
};

struct Settings {
    int width = 800;
    int height = 800;
    float near = 0.1f;
    float far = 4000.f;

    float clear_r = 0.8f;
    float clear_g = 0.8f;
    float clear_b = 1.f;
};

struct vertex
{
    std::array<float, 3> position;
};
