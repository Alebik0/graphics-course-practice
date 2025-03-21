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
    position = (model * vec4(in_position, 1.0)).xyz;
    gl_Position = projection * view * vec4(position, 1.0);
    normal = normalize(mat3(model) * in_normal);
    texcoord = in_texcoord;
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
in vec2 texcoord;

layout (location = 0) out vec4 out_color;

uniform sampler2D textureSample;

void main()
{
    vec3 alb = texture(textureSample, texcoord).rgb;
    vec3 ambient_color = alb * ambient_light;
    vec3 sun_color = alb * max(0.0, dot(normal, sun_direction)) * sun_color;
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
    GLuint texture_location;

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
        sun_color_location(glGetUniformLocation(program, "sun_color")),
        texture_location(glGetUniformLocation(program, "textureSample")) {}
};
