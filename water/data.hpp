#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <array>

class Camera {
private:
    float ROTATION_SPEED = glm::pi<float>() / 2;
    float MOVEMENT_SPEED = 20.f;
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
        
    Camera() {}

    void RotateLeft(const float dt) {
        direction = glm::mat3(glm::rotate(glm::mat4(1.0f), ROTATION_SPEED * dt, glm::vec3(0.f, 1.f, 0.f))) * direction;
        up = glm::mat3(glm::rotate(glm::mat4(1.0f), ROTATION_SPEED * dt, glm::vec3(0.f, 1.f, 0.f))) * up;
    }

    void RotateRight(const float dt) {
        direction = glm::mat3(glm::rotate(glm::mat4(1.0f), -ROTATION_SPEED * dt, glm::vec3(0.f, 1.f, 0.f))) * direction;
        up = glm::mat3(glm::rotate(glm::mat4(1.0f), -ROTATION_SPEED * dt, glm::vec3(0.f, 1.f, 0.f))) * up;
    }

    void RotateUp(const float dt) {
        glm::vec3 right = glm::cross(direction, up);
        direction = glm::mat3(glm::rotate(glm::mat4(1.0f), ROTATION_SPEED * dt, right)) * direction;
        up = glm::mat3(glm::rotate(glm::mat4(1.0f), ROTATION_SPEED * dt, right)) * up;
    }

    void RotateDown(const float dt) {
        glm::vec3 right = glm::cross(direction, up);
        direction = glm::mat3(glm::rotate(glm::mat4(1.0f), -ROTATION_SPEED * dt, right)) * direction;
        up = glm::mat3(glm::rotate(glm::mat4(1.0f), -ROTATION_SPEED * dt, right)) * up;
    }

    void MoveForward(const float dt) {
        position += direction * MOVEMENT_SPEED * dt;
    }

    void MoveBackward(const float dt) {
        position -= direction * MOVEMENT_SPEED * dt;
    }

    void MoveLeft(const float dt) {
        glm::vec3 right = glm::cross(direction, up);
        position -= right * MOVEMENT_SPEED * dt;
    }

    void MoveRight(const float dt) {
        glm::vec3 right = glm::cross(direction, up);
        position += right * MOVEMENT_SPEED * dt;
    }

    void MoveUpward(const float dt) {
        position += up * MOVEMENT_SPEED * dt;
    }

    void MoveDownward(const float dt) {
        position -= up * MOVEMENT_SPEED * dt;
    }

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

struct vertex
{
    std::array<float, 3> position;
};
