#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <stdexcept>

#include "source_shaders.hpp"
#include "debug_shaders.hpp"
#include "shadowmap_shaders.hpp"
#include "globals.hpp"
#include "obj_parser.hpp"
#include "gl_init.hpp"

GLuint create_shader(GLenum type, const char * source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

gl_data init_gl(const obj_data & scene) {
    gl_data result;

    // Make shaders:
    result.source_vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    result.source_fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    result.source_program = create_program(result.source_vertex_shader, result.source_fragment_shader);

    result.debug_vertex_shader = create_shader(GL_VERTEX_SHADER, debug_vertex_shader);
    result.debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, debug_fragment_shader);
    result.debug_program = create_program(result.debug_vertex_shader, result.debug_fragment_shader);

    result.shadowmap_vertex_shader = create_shader(GL_VERTEX_SHADER, shadowmap_vertex_shader);
    result.shadowmap_fragment_shader = create_shader(GL_FRAGMENT_SHADER, shadowmap_fragment_shader);
    result.shadowmap_program = create_program(result.shadowmap_vertex_shader, result.shadowmap_fragment_shader);

    // Make uniform locations:
    result.source__model = glGetUniformLocation(result.source_program, "model");
    result.source__view = glGetUniformLocation(result.source_program, "view");
    result.source__projection = glGetUniformLocation(result.source_program, "projection");
    result.source__ambient_light = glGetUniformLocation(result.source_program, "ambient_light");
    result.source__sun_direction = glGetUniformLocation(result.source_program, "sun_direction");
    result.source__sun_color = glGetUniformLocation(result.source_program, "sun_color");
    result.source__point_light_position = glGetUniformLocation(result.source_program, "point_light_position");
    result.source__point_light_color = glGetUniformLocation(result.source_program, "point_light_color");
    result.source__point_light_attenuation = glGetUniformLocation(result.source_program, "point_light_attenuation");
    result.source__glossiness = glGetUniformLocation(result.source_program, "glossiness");
    result.source__roughness = glGetUniformLocation(result.source_program, "roughness");
    result.source__albedo_texture = glGetUniformLocation(result.source_program, "albedoTexture");
    result.source__alpha_texture = glGetUniformLocation(result.source_program, "alphaTexture");
    result.source__has_alpha_texture = glGetUniformLocation(result.source_program, "hasAlphaTexture");

    result.debug__shadowmap_texture = glGetUniformLocation(result.debug_program, "shadowmapTexture");

    result.shadowmap__model = glGetUniformLocation(result.shadowmap_program, "model");
    result.shadowmap__projection = glGetUniformLocation(result.shadowmap_program, "projection");

    // Make vertex arrays:
    glGenVertexArrays(1, &result.vao);
    glBindVertexArray(result.vao);

    glGenBuffers(1, &result.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, scene.vertices.size() * sizeof(scene.vertices[0]), scene.vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(12));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(24));
    
    glGenVertexArrays(1, &result.debug_vao);
    glBindVertexArray(result.debug_vao);

    // Make textures:
    glGenTextures(1, &result.shadow_map);
    glBindTexture(GL_TEXTURE_2D, result.shadow_map);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Make framebuffers:
    glGenFramebuffers(1, &result.shadow_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, result.shadow_fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, result.shadow_map, 0);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer!");

    return result;
}
