#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stdexcept>

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

        position = glm::vec3(1000.f, 125.f, 0.f);
        angle = 0.f;
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
