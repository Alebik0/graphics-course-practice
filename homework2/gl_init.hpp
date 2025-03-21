#pragma once

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include "obj_parser.hpp"

struct gl_data {
    GLuint vao;
    GLuint vbo;
    GLuint debug_vao;
    GLuint shadow_map;
    GLuint shadow_fbo;
};

gl_data init_gl(const obj_data & scene);
