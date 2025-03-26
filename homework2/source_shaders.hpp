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
#include "camera.hpp"
#include "settings.hpp"
#include "light.hpp"
#include "obj_parser.hpp"

class SourceShader {
private:
    static inline char vertex_shader_source[] =  
R"(#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

out vec3 position;
out vec3 normal;
out vec2 texcoord;

void main()
{
    position = in_position;
    gl_Position = projection * view * vec4(position, 1.0);
    normal = normalize(mat3(model) * in_normal);
    texcoord = in_texcoord;
}
)";

    static inline char fragment_shader_source[] = 
R"(#version 330 core

vec3 albedo;

uniform vec3 ambient_light;

uniform vec3 sun_direction;
uniform vec3 sun_color;

uniform vec3 point_light_position;
uniform vec3 point_light_color;
uniform vec3 point_light_attenuation;

uniform vec3 glossiness;
uniform float roughness;

uniform sampler2D albedoTexture;
uniform sampler2D alphaTexture;
uniform float hasAlphaTexture;

uniform sampler2DShadow shadowmapTexture;
uniform mat4 shadowmap_projection;

in vec3 position;
in vec3 normal;
in vec2 texcoord;

layout (location = 0) out vec4 out_color;

vec3 specular(vec3 direction)
{
    float power = roughness;
    return glossiness * albedo * pow(max(0.0, dot(normal, direction)), power);
}

vec3 diffuse(vec3 direction)
{
    return albedo * max(0.0, dot(normal, direction));
}

void main()
{
    if (hasAlphaTexture > 0.5 && texture(alphaTexture, texcoord).r < 0.5)
        discard;

    albedo = texture(albedoTexture, texcoord).rgb;
    vec3 color = vec3(0.0);
    
    vec3 ambient_color = albedo * ambient_light;
    color += ambient_color;

    vec3 sun_color = diffuse(sun_direction) * sun_color;
    
    vec4 ndc = shadowmap_projection * vec4(position, 1.0);
    
    if (abs(ndc.x) <= 1 && abs(ndc.y) <= 1) {
        vec3 shadowmap_texcoord = ndc.xyz * 0.5 + 0.5;
        float shadow_depth = ndc.z * 0.5 + 0.5;

        if (texture(shadowmapTexture, shadowmap_texcoord) < 0.5) {
        } else {
            color += sun_color;
        }
    } else {
        color += sun_color;
    }    
    
    float distance = length(point_light_position - position);
    float divider = point_light_attenuation.x + distance * point_light_attenuation.y + distance * distance * point_light_attenuation.z;
    float light_attenuation = 1.0 / divider;
    vec3 light_vector = normalize(position - point_light_position);
    vec3 light_color = (diffuse(-light_vector) + specular(-light_vector)) * light_attenuation * point_light_color;
    color += light_color;

    out_color = vec4(color, 1.0);
}
)";

    GLuint program;

    GLuint model_location;
    GLuint view_location;
    GLuint projection_location;

    GLuint ambient_light_location;

    GLuint sun_direction_location;
    GLuint sun_color_location;

    GLuint point_light_position_location;
    GLuint point_light_color_location;
    GLuint point_light_attenuation_location;

    GLuint glossiness_location;
    GLuint roughness_location;

    GLuint albedoTexture_location;
    GLuint alphaTexture_location;
    GLuint hasAlphaTexture_location;

    GLuint shadowmapTexture_location;
    GLuint shadowmap_projection_location;

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
    GLuint vao, vbo;

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    glm::vec3 ambient_light;

    SunLight sun;
    PointLight light;
    
    GLuint shadowmapTexture;
    glm::mat4 shadowmap_projection;

    SourceShader() {
        // Init program:
        GLuint source_vertex_shader = SourceShader::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint source_fragment_shader = SourceShader::create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = create_program(source_vertex_shader, source_fragment_shader);

        model_location = glGetUniformLocation(program, "model");
        view_location = glGetUniformLocation(program, "view");
        projection_location = glGetUniformLocation(program, "projection");
        ambient_light_location = glGetUniformLocation(program, "ambient_light");
        sun_direction_location = glGetUniformLocation(program, "sun_direction");
        sun_color_location = glGetUniformLocation(program, "sun_color");
        point_light_position_location = glGetUniformLocation(program, "point_light_position");
        point_light_color_location = glGetUniformLocation(program, "point_light_color");
        point_light_attenuation_location = glGetUniformLocation(program, "point_light_attenuation");
        glossiness_location = glGetUniformLocation(program, "glossiness");
        roughness_location = glGetUniformLocation(program, "roughness");
        albedoTexture_location = glGetUniformLocation(program, "albedoTexture");
        alphaTexture_location = glGetUniformLocation(program, "alphaTexture");
        hasAlphaTexture_location = glGetUniformLocation(program, "hasAlphaTexture");
        shadowmapTexture_location = glGetUniformLocation(program, "shadowmapTexture");
        shadowmap_projection_location = glGetUniformLocation(program, "shadowmap_projection");

        // Init vao
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(12));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(24));
    }

    void UpdateBufferData(const obj_data & scene) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.vertices.size() * sizeof(scene.vertices[0]), scene.vertices.data(), GL_STATIC_DRAW);
    }

    void Draw(
        const Settings & settings,
        const Camera & camera,
        const obj_data & scene
    ) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(settings.clear_r, settings.clear_g, settings.clear_b, 1.0f);
        glViewport(0, 0, settings.width, settings.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(ambient_light_location, ambient_light.r, ambient_light.g, ambient_light.b);
        
        glUniform3f(sun_direction_location, sun.direction.x, sun.direction.y, sun.direction.z);
        glUniform3f(sun_color_location, sun.color.r, sun.color.g, sun.color.b);

        glUniform3f(point_light_position_location, light.position.x, light.position.y, light.position.z);
        glUniform3f(point_light_color_location, light.color.r, light.color.g, light.color.b);
        glUniform3f(point_light_attenuation_location, light.attenuation.x, light.attenuation.y, light.attenuation.z);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
        glUniform1i(shadowmapTexture_location, 2);
        glUniformMatrix4fv(shadowmap_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&shadowmap_projection));
        
        { // Draw full scene
            glBindVertexArray(vao);

            for (obj_data::face_data face : scene.faces) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, face.albedo_texture);
                glUniform1i(albedoTexture_location, 0);

                if (face.alpha_texture != 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, face.alpha_texture);
                    glUniform1i(alphaTexture_location, 1);
                    glUniform1f(hasAlphaTexture_location, 1.0f);
                } else {
                    glUniform1f(hasAlphaTexture_location, 0.0f);
                }

                glUniform3f(glossiness_location, face.glossiness[0], face.glossiness[1], face.glossiness[2]);
                glUniform1f(roughness_location, face.power);

                glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
            }
        }
    }
};
