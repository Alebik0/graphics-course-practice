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

    // Make uniform locations:
    result.debug__shadowmap_texture = glGetUniformLocation(result.debug_program, "shadowmapTexture");

    // Make vertex arrays:
    glGenVertexArrays(1, &result.debug_vao);
    glBindVertexArray(result.debug_vao);

    return result;
}
