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
    position    = vec3(model * vec4(in_position, 1.0));
    normal      = mat3(transpose(inverse(model))) * in_normal;
    texcoord    = in_texcoord;
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

    static inline char fragment_shader_source[] = 
R"(#version 330 core

uniform vec3 camera_position;

uniform vec3 ambient_light;

uniform vec3 sun_direction;
uniform vec3 sun_color;

uniform vec3 point_light_position;
uniform vec3 point_light_color;
uniform vec3 point_light_attenuation;

uniform vec3 glossiness;
uniform float shininess;

uniform sampler2D albedoTexture;
uniform sampler2D alphaTexture;
uniform float has_alpha;
uniform sampler2D bumpTexture;
uniform float has_bump;
uniform sampler2D specularTexture;
uniform float has_specular;

uniform sampler2DShadow shadowmapTexture;
uniform mat4 shadowmap_projection;

uniform float has_gamma_correction;
uniform float has_aces_correction;
uniform float reflection_draw_mode;

uniform samplerCube reflection_texture;

in vec3 position;
in vec3 normal;
in vec2 texcoord;

layout (location = 0) out vec4 out_color;

vec3 ReflectionColor() {
    vec3 real_normal = normalize(normal);
    vec3 view_direction = normalize(position - camera_position);
    vec3 reflected_direction = reflect(view_direction, real_normal);
    vec3 color = texture(reflection_texture, reflected_direction).rgb;

    return color;
}

vec3 diffuse(vec3 real_normal, vec3 direction) {
    return vec3(max(0.0, dot(real_normal, direction)));
}

vec3 specular(vec3 real_normal, vec3 direction) {
    vec3 reflected_direction = 2.0 * real_normal * dot(real_normal, direction) - direction;
    vec3 view_direction = normalize(camera_position - position);
    float spec = pow(max(dot(reflected_direction, view_direction), 0.0), shininess);
    
    if (has_specular > 0.5)
        return glossiness * spec * texture(specularTexture, texcoord).rgb;
    else
        return glossiness * spec;
}

vec3 phong(vec3 real_normal, vec3 direction) {
    vec3 albedo = texture(albedoTexture, texcoord).rgb;

    return albedo * (diffuse(real_normal, direction) + specular(real_normal, direction));
}

vec3 PerturbNormal(vec3 surf_pos, vec3 surf_norm) {
    vec3 vSigmaS = dFdx ( surf_pos );
    vec3 vSigmaT = dFdy ( surf_pos );
    vec3 vN = normalize(surf_norm) ; // normalized
    vec3 vR1 = cross ( vSigmaT , vN );
    vec3 vR2 = cross (vN , vSigmaS );
    float fDet = dot ( vSigmaS , vR1 );

    float dBs, dBt;
    { // Screen–space height derivative evaluation by forward differencing.
        vec2 TexDx = dFdx ( texcoord );
        vec2 TexDy = dFdx ( texcoord );
        vec2 STll = texcoord;
        vec2 STlr = texcoord + TexDx ;
        vec2 STul = texcoord + TexDy ;
        float Hll = texture(bumpTexture, STll).x;
        float Hlr = texture(bumpTexture, STlr).x;
        float Hul = texture(bumpTexture, STul).x;
        dBs = Hlr - Hll ;
        dBt = Hul - Hll ;
    }

    vec3 vSurfGrad = sign ( fDet ) * ( dBs * vR1 + dBt * vR2 );

    return normalize ( abs ( fDet )*vN - vSurfGrad );
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 MakeRealNormal() {
    if (has_bump > 0.5) {
        return normalize(PerturbNormal(position, normal));
    } else {
        return normalize(normal);
    }
}

vec3 MaterialColor() {
    if (has_alpha > 0.5 && texture(alphaTexture, texcoord).r < 0.5)
        discard;

    vec3 real_normal = MakeRealNormal();

    vec3 albedo = texture(albedoTexture, texcoord).rgb;
    vec3 color = vec3(0.0);
    
    { // Add ambient color
        vec3 ambient_color = albedo * ambient_light;
        color += ambient_color;
    }

    { // Add sun color
        vec4 ndc = shadowmap_projection * vec4(position, 1.0);
        
        if (abs(ndc.x) <= 1 && abs(ndc.y) <= 1) {
            vec3 shadowmap_texcoord = ndc.xyz * 0.5 + 0.5;
            
            float sum = 0.0;
            float sum_w = 0.0;
            const int N = 5;
            float radius = 7.0;
            for (int x = -N; x <= N; x += 1) {
                for (int y = -N; y <= N; y += 1) {
                    float c = exp(-float(x * x + y * y) / (radius*radius));
                    sum_w += c;
                    sum += c * texture(shadowmapTexture, shadowmap_texcoord + vec3(x, y, 0.0) / vec3(textureSize(shadowmapTexture, 0), 1.0));
                }
            }
    
            color += sum / sum_w * phong(real_normal, sun_direction) * sun_color;
        }
    }  
    
    { // Add light color
        float distance = length(point_light_position - position);
        float divider = point_light_attenuation.x + distance * point_light_attenuation.y + distance * distance * point_light_attenuation.z;
        float light_attenuation = 1.0 / divider;
        vec3 light_vector = normalize(point_light_position - position);

        color += phong(real_normal, light_vector) * light_attenuation * point_light_color;
    }

    return color;
}

void main()
{
    vec3 color;
    if (reflection_draw_mode > 0.5) {
        color = ReflectionColor();
    } else {
        color = MaterialColor();
    }

    { // Gamma correction:
        float gamma = 2.2;

        if (has_aces_correction > 0.5)
            color = ACESFilm(color);
        if (has_gamma_correction > 0.5)
            color = pow(color, vec3(1.0 / gamma));
    }

    out_color = vec4(color, 1.0);
}
)";

    GLuint program;

    GLuint model_location;
    GLuint view_location;
    GLuint projection_location;

    GLuint camera_position_location;

    GLuint ambient_light_location;

    GLuint sun_direction_location;
    GLuint sun_color_location;

    GLuint point_light_position_location;
    GLuint point_light_color_location;
    GLuint point_light_attenuation_location;

    GLuint glossiness_location;
    GLuint shininess_location;

    GLuint albedoTexture_location;
    GLuint alphaTexture_location;
    GLuint has_alpha_location;
    GLuint bumpTexture_location;
    GLuint has_bump_location;
    GLuint specularTexture_location;
    GLuint has_specular_location;

    GLuint shadowmapTexture_location;
    GLuint shadowmap_projection_location;

    GLuint has_gamma_correction_location;
    GLuint has_aces_correction_location;

    GLuint reflection_draw_mode_location;
    GLuint reflection_texture_location;

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
        } else {
            std::cout << "Shader compilation successful" << std::endl;
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

    Camera camera;
    SunLight sun;
    PointLight light;
    
    GLuint shadowmapTexture;
    glm::mat4 shadowmap_projection;

    GLuint bunny_reflection_texture;
    GLuint bunny_reflection_rbo[6];
    GLuint bunny_reflection_fbo[6];

    bool bump_mark;
    bool specular_mark;
    bool gamma_correction_mark;
    bool aces_correction_mark;

    SourceShader() {
        // Init program:
        GLuint source_vertex_shader = SourceShader::create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint source_fragment_shader = SourceShader::create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = create_program(source_vertex_shader, source_fragment_shader);

        model_location = glGetUniformLocation(program, "model");
        view_location = glGetUniformLocation(program, "view");
        projection_location = glGetUniformLocation(program, "projection");
        camera_position_location = glGetUniformLocation(program, "camera_position");
        ambient_light_location = glGetUniformLocation(program, "ambient_light");
        sun_direction_location = glGetUniformLocation(program, "sun_direction");
        sun_color_location = glGetUniformLocation(program, "sun_color");
        point_light_position_location = glGetUniformLocation(program, "point_light_position");
        point_light_color_location = glGetUniformLocation(program, "point_light_color");
        point_light_attenuation_location = glGetUniformLocation(program, "point_light_attenuation");
        glossiness_location = glGetUniformLocation(program, "glossiness");
        shininess_location = glGetUniformLocation(program, "shininess");

        albedoTexture_location = glGetUniformLocation(program, "albedoTexture");
        alphaTexture_location = glGetUniformLocation(program, "alphaTexture");
        bumpTexture_location = glGetUniformLocation(program, "bumpTexture");
        specularTexture_location = glGetUniformLocation(program, "specularTexture");
        has_alpha_location = glGetUniformLocation(program, "has_alpha");
        has_bump_location = glGetUniformLocation(program, "has_bump");
        has_specular_location = glGetUniformLocation(program, "has_specular");

        shadowmapTexture_location = glGetUniformLocation(program, "shadowmapTexture");
        shadowmap_projection_location = glGetUniformLocation(program, "shadowmap_projection");

        has_gamma_correction_location = glGetUniformLocation(program, "has_gamma_correction");
        has_aces_correction_location = glGetUniformLocation(program, "has_aces_correction");

        reflection_draw_mode_location = glGetUniformLocation(program, "reflection_draw_mode");
        reflection_texture_location = glGetUniformLocation(program, "reflection_texture");
        

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

        // Cubemap reflection
        glGenTextures(1, &bunny_reflection_texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, bunny_reflection_texture);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, REFLECTION_CUBEMAP_RESOLUTION, REFLECTION_CUBEMAP_RESOLUTION, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glGenRenderbuffers(1, bunny_reflection_rbo + i);
            glBindRenderbuffer(GL_RENDERBUFFER, bunny_reflection_rbo[i]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, REFLECTION_CUBEMAP_RESOLUTION, REFLECTION_CUBEMAP_RESOLUTION);
    
            glGenFramebuffers(1, bunny_reflection_fbo + i);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bunny_reflection_fbo[i]);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, bunny_reflection_texture, 0);
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, bunny_reflection_rbo[i]);
    
            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Incomplete framebuffer!");
        }
    }

    void UpdateBufferData(const std::vector<obj_data::vertex> & vertices) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    }

    void PrepareDraw(
        const Settings & settings
    ) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(settings.clear_r, settings.clear_g, settings.clear_b, 1.0f);
        glViewport(0, 0, settings.width, settings.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_FRAMEBUFFER_SRGB); 
        glCullFace(GL_BACK);
    }

    void DrawScene(
        const Settings & settings,
        const Camera & camera,
        const obj_data & scene
    ) {
        glUseProgram(program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(camera_position_location, camera.position.x, camera.position.y, camera.position.z);

        glUniform3f(ambient_light_location, ambient_light.r, ambient_light.g, ambient_light.b);
        
        glUniform3f(sun_direction_location, sun.direction.x, sun.direction.y, sun.direction.z);
        glUniform3f(sun_color_location, sun.color.r, sun.color.g, sun.color.b);

        glUniform3f(point_light_position_location, light.position.x, light.position.y, light.position.z);
        glUniform3f(point_light_color_location, light.color.r, light.color.g, light.color.b);
        glUniform3f(point_light_attenuation_location, light.attenuation.x, light.attenuation.y, light.attenuation.z);
        
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
        glUniform1i(shadowmapTexture_location, 5);
        
        glUniformMatrix4fv(shadowmap_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&shadowmap_projection));
        
        glUniform1f(has_gamma_correction_location, gamma_correction_mark ? 1.0f : 0.0f);
        glUniform1f(has_aces_correction_location, aces_correction_mark ? 1.0f : 0.0f);
        
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
                    glUniform1f(has_alpha_location, 1.0f);
                } else {
                    glUniform1f(has_alpha_location, 0.0f);
                }

                if (face.bump_texture != 0 && bump_mark) {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, face.bump_texture);
                    glUniform1i(bumpTexture_location, 2);
                    glUniform1f(has_bump_location, 1.0f);
                } else {
                    glUniform1f(has_bump_location, 0.0f);
                }

                if (face.specular_texture != 0 && specular_mark) {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, face.specular_texture);
                    glUniform1i(specularTexture_location, 3);
                    glUniform1f(has_specular_location, 1.0f);
                } else {
                    glUniform1f(has_specular_location, 0.0f);
                }

                glUniform3f(glossiness_location, face.glossiness[0], face.glossiness[1], face.glossiness[2]);
                glUniform1f(shininess_location, face.power);
                glUniform1f(reflection_draw_mode_location, 0.0f);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, bunny_reflection_texture);
                glUniform1i(reflection_texture_location, 4);

                glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
            }
        }
    }

    void DrawBunny(
        const Settings & settings,
        const Camera & camera,
        const obj_data & scene,
        const obj_data & bunny
    ) {
        glUseProgram(program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glUniform3f(camera_position_location, camera.position.x, camera.position.y, camera.position.z);

        glUniform3f(ambient_light_location, ambient_light.r, ambient_light.g, ambient_light.b);
        
        glUniform3f(sun_direction_location, sun.direction.x, sun.direction.y, sun.direction.z);
        glUniform3f(sun_color_location, sun.color.r, sun.color.g, sun.color.b);

        glUniform3f(point_light_position_location, light.position.x, light.position.y, light.position.z);
        glUniform3f(point_light_color_location, light.color.r, light.color.g, light.color.b);
        glUniform3f(point_light_attenuation_location, light.attenuation.x, light.attenuation.y, light.attenuation.z);
        
        glUniform1f(has_gamma_correction_location, gamma_correction_mark ? 1.0f : 0.0f);
        glUniform1f(has_aces_correction_location, aces_correction_mark ? 1.0f : 0.0f);

        { // Draw bunny
            glBindVertexArray(vao);

            for (obj_data::face_data face : bunny.faces) {
                glUniform3f(glossiness_location, face.glossiness[0], face.glossiness[1], face.glossiness[2]);
                glUniform1f(shininess_location, face.power);
                glUniform1f(reflection_draw_mode_location, 1.0f);

                glDrawArrays(GL_TRIANGLES, scene.vertices.size() + face.firstVertex, face.countVertex);
            }
        }
    }
};


class ShadowmapShader {
private:
    static inline char shadowmap_vertex_shader[] =
R"(#version 330 core

uniform mat4 projection;

layout (location = 0) in vec3 in_position;

void main()
{
    gl_Position = projection * vec4(in_position, 1.0);
}
)";
    
    static inline char shadowmap_fragment_shader[] =
R"(#version 330 core

void main()
{
    gl_FragDepth = gl_FragCoord.z;
}
)";

    GLuint program;
    
    GLuint projection_location;

    GLuint shadowmap_fbo;

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

    glm::mat4 projection;

    ShadowmapShader() {
        // Init program:
        GLuint shadowmap_vertex_shader = create_shader(GL_VERTEX_SHADER, ShadowmapShader::shadowmap_vertex_shader);
        GLuint shadowmap_fragment_shader = create_shader(GL_FRAGMENT_SHADER, ShadowmapShader::shadowmap_fragment_shader);
        program = create_program(shadowmap_vertex_shader, shadowmap_fragment_shader);
        
        projection_location = glGetUniformLocation(program, "projection");

        // Make textures:
        glGenTextures(1, &shadowmapTexture);
        glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // Make framebuffers:
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

        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        { // Draw full scene
            glBindVertexArray(sourceShader.vao);

            for (obj_data::face_data face : scene.faces) {
                glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
            }
        }
    }
};


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

out vec3 texcoord;

void main()
{
    vec2 position = vertices[gl_VertexID];
    gl_Position = vec4(position * 0.25 + vec2(-0.75, -0.75), 0.0, 1.0);
    texcoord = vec3(position.xy, 1.0);
}
)";
    
    static inline char debug_fragment_shader[] = 
R"(#version 330 core

uniform samplerCube shadowmapTexture;

in vec3 texcoord;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(texture(shadowmapTexture, texcoord).rgb, 1.0);
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
