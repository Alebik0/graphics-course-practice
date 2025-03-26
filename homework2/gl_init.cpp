#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <stdexcept>

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

gl_data init_gl(
    const int width,
    const int height,
    const obj_data & scene
) {
    gl_data result;

    // Make shaders:
    result.debug_vertex_shader = create_shader(GL_VERTEX_SHADER, debug_vertex_shader);
    result.debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, debug_fragment_shader);
    result.debug_program = create_program(result.debug_vertex_shader, result.debug_fragment_shader);

    result.shadowmap_vertex_shader = create_shader(GL_VERTEX_SHADER, shadowmap_vertex_shader);
    result.shadowmap_fragment_shader = create_shader(GL_FRAGMENT_SHADER, shadowmap_fragment_shader);
    result.shadowmap_program = create_program(result.shadowmap_vertex_shader, result.shadowmap_fragment_shader);

    // Make uniform locations:
    result.debug__shadowmap_texture = glGetUniformLocation(result.debug_program, "shadowmapTexture");

    result.shadowmap__model = glGetUniformLocation(result.shadowmap_program, "model");
    result.shadowmap__projection = glGetUniformLocation(result.shadowmap_program, "projection");

    // Make vertex arrays:
    glGenVertexArrays(1, &result.debug_vao);
    glBindVertexArray(result.debug_vao);

    // Make textures:
    glGenTextures(1, &result.shadow_map);
    glBindTexture(GL_TEXTURE_2D, result.shadow_map);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Make framebuffers:
    glGenRenderbuffers(1, &result.shadow_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, result.shadow_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);

    glGenFramebuffers(1, &result.shadow_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, result.shadow_fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, result.shadow_map, 0);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer!");

    return result;
}
