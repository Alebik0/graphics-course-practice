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
    GLuint debug_program;
    GLuint debug_vertex_shader;
    GLuint debug_fragment_shader;

    GLuint debug__shadowmap_texture;

    GLuint debug_vao;
};

gl_data init_gl(const int width, const int height, const obj_data & scene);
