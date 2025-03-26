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
#include "globals.hpp"
#include "source_shaders.hpp"
#include "obj_parser.hpp"

class ShadowmapShader {
private:
    static inline char shadowmap_vertex_shader[] =
R"(#version 330 core

uniform mat4 model;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;

void main()
{
    gl_Position = projection * model * vec4(in_position, 1.0);
}
)";
    
    static inline char shadowmap_fragment_shader[] =
R"(#version 330 core

out vec4 shadow_output;

void main()
{
    float z = gl_FragCoord.z;
    shadow_output = vec4(z, z * z + 1.0 / 4 * (dFdx(z) * dFdx(z) + dFdy(z) * dFdy(z)), 0, 0);
}
)";

    GLuint program;
    
    GLuint model_location;
    GLuint projection_location;

    GLuint shadowmap_rbo, shadowmap_fbo;

    static GLuint create_shader(GLenum type, const char * source)
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

    glm::mat4 model;
    glm::mat4 projection;

    ShadowmapShader() {
        // Init program:
        GLuint shadowmap_vertex_shader = create_shader(GL_VERTEX_SHADER, ShadowmapShader::shadowmap_vertex_shader);
        GLuint shadowmap_fragment_shader = create_shader(GL_FRAGMENT_SHADER, ShadowmapShader::shadowmap_fragment_shader);
        program = create_program(shadowmap_vertex_shader, shadowmap_fragment_shader);
        
        model_location = glGetUniformLocation(program, "model");
        projection_location = glGetUniformLocation(program, "projection");

        // Make textures:
        glGenTextures(1, &shadowmapTexture);
        glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // Make framebuffers:
        glGenRenderbuffers(1, &shadowmap_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, shadowmap_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);

        glGenFramebuffers(1, &shadowmap_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowmap_fbo);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmapTexture, 0);

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Incomplete framebuffer!");
    }

    void Draw(
        const SourceShader & sourceShader,
        const obj_data & scene
    ) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowmap_fbo);
        glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glUseProgram(program);

        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        { // Draw full scene
            glBindVertexArray(sourceShader.vao);

            for (obj_data::face_data face : scene.faces) {
                glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
            }
        }
    }
};