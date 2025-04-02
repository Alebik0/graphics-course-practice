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

#include <array>
#include <vector>
#include <string>

class Camera {
private:
    static inline const float CAMERA_SPEED = 250.f;
    static inline const float CAMERA_ROTATION_SPEED = glm::pi<float>() / 2;

    glm::vec3 forward_direction;
    glm::vec3 right_direction;
    glm::vec3 up_direction;
public:
    glm::vec3 position;
    float angle;

    Camera() {
        forward_direction = glm::vec3(0.f, 0.f, 1.f);
        right_direction = glm::vec3(1.f, 0.f, 0.f);
        up_direction = glm::vec3(0.f, 1.f, 0.f);

        position = glm::vec3(1100.f, 125.f, 200.f);
        angle = glm::pi<float>();
    }

    void MoveForward(const float dt) {
        position += (-forward_direction * std::cos(angle) + right_direction * std::sin(angle)) * CAMERA_SPEED * dt;
    }

    void MoveBackward(const float dt) {
        position -= (-forward_direction * std::cos(angle) + right_direction * std::sin(angle)) * CAMERA_SPEED * dt;
    }

    void MoveUpward(const float dt) {
        position += up_direction * dt * CAMERA_SPEED;
    }

    void MoveDownward(const float dt) {
        position -= up_direction * dt * CAMERA_SPEED;
    }
    
    void RotateLeft(const float dt) {
        angle -= CAMERA_ROTATION_SPEED * dt;
    }

    void RotateRight(const float dt) {
        angle += CAMERA_ROTATION_SPEED * dt;
    }
};

struct SunLight {
    glm::vec3 direction;
    glm::vec3 color;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 attenuation;
};

struct Settings {
    int width = 800;
    int height = 800;
    float near = 0.1f;
    float far = 4000.f;

    float clear_r = 0.8f;
    float clear_g = 0.8f;
    float clear_b = 1.f;
};

struct obj_data
{
    struct vertex
    {
        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 2> texcoord;
    };

    struct face_data
    {
        float firstVertex;
        float countVertex;
        std::string material_name;
        std::string albedo_texname;             // TinyObj -> ambient_texname
        std::string alpha_texname;              // TinyObj -> alpha_texname
        std::string bump_texname;               // TinyObj -> bump_texname
        std::string specular_texname;           // TinyObj -> specular_texname
        std::array<float, 3> glossiness;        // TinyObj -> specular
        float power;                            // TinyObj -> shininess
        GLuint albedo_texture;
        GLuint alpha_texture;
        GLuint bump_texture;
        GLuint specular_texture;
;
    };

    std::vector<vertex> vertices;
    std::vector<face_data> faces;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
};
