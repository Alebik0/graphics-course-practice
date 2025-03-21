#pragma once

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include "source_shaders.hpp"
#include "obj_parser.hpp"

struct gl_data {
    GLuint source_vertex_shader;
    GLuint source_fragment_shader;
    GLuint debug_vertex_shader;
    GLuint debug_fragment_shader;

    source_shader_program source_program;
    GLuint debug_program;

    GLuint vao;
    GLuint vbo;
    GLuint debug_vao;
    GLuint shadow_map;
    GLuint shadow_fbo;
};

gl_data init_gl(const obj_data & scene);
