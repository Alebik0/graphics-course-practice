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

out vec3 position;
out vec3 normal;

void main()
{
    position = (model * vec4(in_position, 1.0)).xyz;
    gl_Position = projection * view * vec4(position, 1.0);
    normal = normalize(mat3(model) * in_normal);
}
)";

const char fragment_shader_source[] =
R"(#version 330 core

uniform vec3 camera_position;
uniform vec3 albedo;

uniform vec3 ambient_light;
uniform vec3 sun_direction;
uniform vec3 sun_color;

in vec3 position;
in vec3 normal;

layout (location = 0) out vec4 out_color;

vec3 diffuse(vec3 direction)
{
    return albedo * max(0.0, dot(normal, direction));
}

void main()
{
    vec3 ambient_color = albedo * ambient_light;
    vec3 sun_color = diffuse(sun_direction) * sun_color;
    vec3 color = ambient_color + sun_color;
    
    out_color = vec4(color, 1.0);
}
)";

struct source_shader_program
{
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;

    GLuint model_location;
    GLuint view_location;
    GLuint projection_location;

    GLuint camera_position_location;
    GLuint albedo_location;
    GLuint ambient_light_location;
    GLuint sun_direction_location;
    GLuint sun_color_location;
    GLuint point_light_position_location;
    GLuint point_light_color_location;
    GLuint point_light_attenuation_location;
    GLuint glossiness_location;
    GLuint roughness_location;

    source_shader_program(GLuint program, GLuint vertex_shader, GLuint fragment_shader) :
        program(program),
        vertex_shader(vertex_shader),
        fragment_shader(fragment_shader),
        model_location(glGetUniformLocation(program, "model")),
        view_location(glGetUniformLocation(program, "view")),
        projection_location(glGetUniformLocation(program, "projection")),
        camera_position_location(glGetUniformLocation(program, "camera_position")),
        albedo_location(glGetUniformLocation(program, "albedo")),
        ambient_light_location(glGetUniformLocation(program, "ambient_light")),
        sun_direction_location(glGetUniformLocation(program, "sun_direction")),
        sun_color_location(glGetUniformLocation(program, "sun_color")) {}
};
