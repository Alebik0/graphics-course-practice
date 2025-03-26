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

#include <stdexcept>

class DebugShader {
    static inline char debug_vertex_shader[] = 
R"(#version 330 core

vec2 vertices[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

out vec2 texcoord;

void main()
{
    vec2 position = vertices[gl_VertexID];
    gl_Position = vec4(position * 0.25 + vec2(-0.75, -0.75), 0.0, 1.0);
    texcoord = position * 0.5 + vec2(0.5);
}
)";
    
    static inline char debug_fragment_shader[] = 
R"(#version 330 core

uniform sampler2D shadowmapTexture;

in vec2 texcoord;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = texture(shadowmapTexture, texcoord);
}
)";

    GLuint program;

    GLuint shadowmapTexture_location;
    GLuint vao;

    static GLuint create_shader(GLenum type, const char * source) {
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

    static GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
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
public:
    GLuint shadowmapTexture;

    DebugShader() {
        // Init program:
        GLuint debug_vertex_shader = create_shader(GL_VERTEX_SHADER, DebugShader::debug_vertex_shader);
        GLuint debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, DebugShader::debug_fragment_shader);
        program = create_program(debug_vertex_shader, debug_fragment_shader);
        
        shadowmapTexture_location = glGetUniformLocation(program, "shadowmapTexture");

        // Make vertex arrays:
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    void Draw() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
        glUniform1i(shadowmapTexture_location, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};
