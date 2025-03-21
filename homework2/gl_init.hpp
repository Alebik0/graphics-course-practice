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
    GLuint source_program;

    GLuint source__model;
    GLuint source__view;
    GLuint source__projection;

    GLuint source__ambient_light;
    GLuint source__sun_direction;
    GLuint source__sun_color;
    GLuint source__point_light_position;
    GLuint source__point_light_color;
    GLuint source__point_light_attenuation;
    GLuint source__glossiness;
    GLuint source__roughness;
    GLuint source__albedo_texture;
    GLuint source__alpha_texture;
    GLuint source__has_alpha_texture;
    GLuint source__shadowmap_texture;
    GLuint source__shadowmap_projection;

    GLuint debug_program;
    GLuint debug_vertex_shader;
    GLuint debug_fragment_shader;

    GLuint debug__shadowmap_texture;

    GLuint shadowmap_program;
    GLuint shadowmap_vertex_shader;
    GLuint shadowmap_fragment_shader;

    GLuint shadowmap__model;
    GLuint shadowmap__projection;

    GLuint vao;
    GLuint vbo;
    GLuint debug_vao;
    GLuint shadow_map;
    GLuint shadow_fbo;
};

gl_data init_gl(const obj_data & scene);
