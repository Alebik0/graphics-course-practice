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

    GLuint shadowmap_program;
    GLuint shadowmap_vertex_shader;
    GLuint shadowmap_fragment_shader;

    GLuint shadowmap__model;
    GLuint shadowmap__projection;

    GLuint debug_vao;
    GLuint shadow_map;
    GLuint shadow_fbo;
    GLuint shadow_rbo;
};

gl_data init_gl(const int width, const int height, const obj_data & scene);
