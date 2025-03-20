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

out vec3 normal;

void main()
{
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    normal = normalize(mat3(model) * in_normal);
}
)";

const char fragment_shader_source[] =
R"(#version 330 core

uniform vec3 camera_position;

in vec3 normal;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 ambient_dir = vec3(0.0, 1.0, 0.0);
    vec3 ambient_color = vec3(0.2);

    vec3 light1_dir = normalize(vec3( 3.0, 2.0,  1.0));
    vec3 light2_dir = normalize(vec3(-3.0, 2.0, -1.0));

    vec3 light1_color = vec3(1.0,  0.5, 0.25);
    vec3 light2_color = vec3(0.25, 0.5, 1.0 );

    vec3 n = normalize(normal);

    vec3 color = (0.5 + 0.5 * dot(n, ambient_dir)) * ambient_color
        + max(0.0, dot(n, light1_dir)) * light1_color
        + max(0.0, dot(n, light2_dir)) * light2_color
        ;

    float gamma = 1.0 / 2.2;
    out_color = vec4(pow(min(vec3(1.0), color), vec3(gamma)), 1.0);
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

    source_shader_program(GLuint program, GLuint vertex_shader, GLuint fragment_shader) :
        program(program),
        vertex_shader(vertex_shader),
        fragment_shader(fragment_shader),
        model_location(glGetUniformLocation(program, "model")),
        view_location(glGetUniformLocation(program, "view")),
        projection_location(glGetUniformLocation(program, "projection")),
        camera_position_location(glGetUniformLocation(program, "camera_position")) {}
};
