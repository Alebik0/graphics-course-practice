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
#include <vector>
#include "data.hpp"


class PoolShader {
private:
    static inline char vertex_shader_source[] =  
R"(#version 330 core

uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;

out vec3 position;
out vec3 normal;

void main()
{
    const float X_LEN = 4.0;
    const float Y_LEN = 2.0;
    position    = in_position;
    normal      = vec3(0.0, 1.0, 0.0);
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

    static inline char fragment_shader_source[] = 
R"(#version 330 core

uniform vec3 camera_position;

uniform sampler2D albedoTexture;

in vec3 position;
in vec3 normal;
vec2 texcoord;

layout (location = 0) out vec4 out_color;

void main()
{
    const float X_LEN = 4.0;
    const float Y_LEN = 2.0;
    texcoord = vec2(position.x / X_LEN, position.z / Y_LEN);
    out_color = vec4(texture(albedoTexture, texcoord).rgb, 1.0);
}
)";

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

    GLuint program;

    GLuint vao, vbo;

    GLuint view_location;
    GLuint projection_location;

    GLuint camera_position_location;

    GLuint albedoTexture_location;
public:
    glm::mat4 view, projection;
    glm::vec3 camera_position;
    GLuint albedoTexture;

    PoolShader() {
        GLuint source_vertex_shader = PoolShader::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint source_fragment_shader = PoolShader::create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = create_program(source_vertex_shader, source_fragment_shader);

        view_location = glGetUniformLocation(program, "view");
        projection_location = glGetUniformLocation(program, "projection");
        camera_position_location = glGetUniformLocation(program, "camera_position");
        albedoTexture_location = glGetUniformLocation(program, "albedoTexture");

        // Init vao
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0));
    }

    void UpdateBufferData(const std::vector<vertex> & vertices) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
    }

    void Draw(const int offset, const int size) {
        glUseProgram(program);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(camera_position_location, camera_position.x, camera_position.y, camera_position.z);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoTexture);
        glUniform1i(albedoTexture_location, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, offset, size);
    }
};


class SkyboxShader {
private:
    static inline char vertex_shader_source[] =  
R"(#version 330 core

vec2 vertices[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

uniform mat4 view;
uniform mat4 projection;

out vec3 position;

void main()
{
    vec4 ndc = vec4(vertices[gl_VertexID], 0.0, 1.0);
    vec4 clip_space = inverse(projection * view) * ndc;
    position = clip_space.xyz / clip_space.w;
    gl_Position = ndc;
}
)";

    static inline char fragment_shader_source[] = 
R"(#version 330 core

uniform samplerCube environmentTexture;

uniform vec3 camera_position;

in vec3 position;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 direction = position - camera_position;

    out_color = vec4(texture(environmentTexture, direction).rgb, 1.0);
}
)";

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

    GLuint program;

    GLuint vao;

    GLuint view_location;
    GLuint projection_location;

    GLuint camera_position_location;

    GLuint environmentTexture_location;
public:
    glm::mat4 view, projection;
    glm::vec3 camera_position;
    GLuint environmentTexture;

    SkyboxShader() {
        GLuint source_vertex_shader = SkyboxShader::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint source_fragment_shader = SkyboxShader::create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = create_program(source_vertex_shader, source_fragment_shader);

        view_location = glGetUniformLocation(program, "view");
        projection_location = glGetUniformLocation(program, "projection");
        camera_position_location = glGetUniformLocation(program, "camera_position");
        environmentTexture_location = glGetUniformLocation(program, "environmentTexture");

        // Init vao
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    void Draw(const int offset, const int size) {
        glUseProgram(program);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(camera_position_location, camera_position.x, camera_position.y, camera_position.z);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentTexture);
        glUniform1i(environmentTexture_location, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, offset, size);
    }
};


class WaterShader {
private:
    static inline char vertex_shader_source[] =  
R"(#version 330 core

uniform float time;

uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;

out vec3 position;
out vec3 normal;

float WaterLevel(float x, float y, float time) {
    return 3.f + sin(x + time * 0.3);
}

vec3 WaterNormal(float x, float y, float time) {
    return vec3(-cos(x + time * 0.3), 0.f, 1.f);
}

void main()
{
    position    = vec3(in_position.x, WaterLevel(in_position.x, in_position.z, time), in_position.z);
    normal      = WaterNormal(in_position.x, in_position.z, time);
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

    static inline char fragment_shader_source[] = 
R"(#version 330 core

uniform vec3 camera_position;

uniform sampler2D albedoTexture;
uniform samplerCube environmentTexture;

in vec3 position;
in vec3 normal;
vec2 texcoord;

layout (location = 0) out vec4 out_color;

float Schlick(vec3 N, vec3 V) {
    const float n1 = 1;
    const float n2 = 1.3333;
    float R0 = (n1 - n2) * (n1 - n2) / (n1 + n2) / (n1 + n2);
    
    return R0 + (1 - R0) * pow(1 - dot(N, V), 5);
}

void main()
{
    vec3 N = normalize(normal);
    vec3 V = normalize(camera_position - position);
    float RC = Schlick(N, V);
    out_color = vec4(0.0, 0.0, RC / 2.0, 1.0);
}
)";

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

    GLuint program;

    GLuint vao, vbo;

    GLuint time_location;

    GLuint view_location;
    GLuint projection_location;

    GLuint camera_position_location;

    GLuint albedoTexture_location;
    GLuint environmentTexture_location;
public:
    glm::mat4 view, projection;
    glm::vec3 camera_position;
    GLuint albedoTexture, environmentTexture;
    float time;

    WaterShader() {
        GLuint source_vertex_shader = WaterShader::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint source_fragment_shader = WaterShader::create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = create_program(source_vertex_shader, source_fragment_shader);

        time_location = glGetUniformLocation(program, "time");
        view_location = glGetUniformLocation(program, "view");
        projection_location = glGetUniformLocation(program, "projection");
        camera_position_location = glGetUniformLocation(program, "camera_position");
        albedoTexture_location = glGetUniformLocation(program, "albedoTexture");
        environmentTexture_location = glGetUniformLocation(program, "environmentTexture");

        // Init vao
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0));
    }

    void UpdateBufferData(const std::vector<vertex> & vertices) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
    }

    void Draw(const int offset, const int size) {
        glUseProgram(program);

        glUniform1f(time_location, time);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(camera_position_location, camera_position.x, camera_position.y, camera_position.z);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, albedoTexture);
        glUniform1i(albedoTexture_location, 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentTexture);
        glUniform1i(environmentTexture_location, 1);

        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, offset, size);
    }
};
