#pragma once

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

const char vertex_shader_source[] =
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

const char fragment_shader_source[] =
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
    if (hasAlphaTexture > 0.5 && texture(alphaTexture, texcoord).r < 0.5) {
        discard;
    } else {
        albedo = texture(albedoTexture, texcoord).rgb;
        vec3 ambient_color = albedo * ambient_light;
        vec3 sun_color = diffuse(sun_direction) * sun_color;
    
        float distance = length(point_light_position - position);
        float divider = point_light_attenuation.x + distance * point_light_attenuation.y + distance * distance * point_light_attenuation.z;
        float light_attenuation = 1.0 / divider;
        vec3 light_vector = normalize(position - point_light_position);
        vec3 light_color = (diffuse(-light_vector) + specular(-light_vector)) * light_attenuation * point_light_color;

        vec3 color = ambient_color + sun_color + light_color;
        
        out_color = vec4(color, 1.0);
    }
}
)";

struct source_shader_program
{
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;

    GLuint model;
    GLuint view;
    GLuint projection;

    GLuint ambient_light;
    GLuint sun_direction;
    GLuint sun_color;
    GLuint point_light_position;
    GLuint point_light_color;
    GLuint point_light_attenuation;
    GLuint glossiness;
    GLuint roughness;
    GLuint albedo_texture;
    GLuint alpha_texture;
    GLuint has_alpha_texture;

    source_shader_program(GLuint program, GLuint vertex_shader, GLuint fragment_shader) :
        program(program),
        vertex_shader(vertex_shader),
        fragment_shader(fragment_shader),
        model(glGetUniformLocation(program, "model")),
        view(glGetUniformLocation(program, "view")),
        projection(glGetUniformLocation(program, "projection")),
        ambient_light(glGetUniformLocation(program, "ambient_light")),
        sun_direction(glGetUniformLocation(program, "sun_direction")),
        sun_color(glGetUniformLocation(program, "sun_color")),
        point_light_position(glGetUniformLocation(program, "point_light_position")),
        point_light_color(glGetUniformLocation(program, "point_light_color")),
        point_light_attenuation(glGetUniformLocation(program, "point_light_attenuation")),
        glossiness(glGetUniformLocation(program, "glossiness")),
        roughness(glGetUniformLocation(program, "roughness")),
        albedo_texture(glGetUniformLocation(program, "albedoTexture")),
        alpha_texture(glGetUniformLocation(program, "alphaTexture")),
        has_alpha_texture(glGetUniformLocation(program, "hasAlphaTexture")) {}

    source_shader_program() {}
};
