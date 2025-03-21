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

#include "obj_parser.hpp"
#include "gl_init.hpp"

void draw_shadowmap(
    const obj_data & scene,
    const gl_data & scene_gl_data,
    glm::mat4 & shadowmap_projection
);

void draw_scene(
    const obj_data & scene,
    const gl_data & scene_gl_data,
    const glm::vec3 & camera_position,
    const float camera_angle,
    const float width,
    const float height,
    const float near,
    const float far,
    const float time,
    glm::mat4 & shadowmap_projection
);

void draw_debug(
    const obj_data & scene,
    const gl_data & scene_gl_data
);
