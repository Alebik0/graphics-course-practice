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

#include "obj_parser.hpp"
#include "gl_init.hpp"
#include "globals.hpp"
#include "draw.hpp"


void draw_shadowmap(
    const obj_data & scene,
    const gl_data & scene_gl_data,
    glm::mat4 & shadowmap_projection
) {
    glm::mat4 model(1.f);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_gl_data.shadow_fbo);
    glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
    glClearColor(1.f, 1.f, 0.f, 0.f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(scene_gl_data.shadowmap_program);

    glUniformMatrix4fv(scene_gl_data.shadowmap__model, 1, GL_FALSE, reinterpret_cast<float *>(&model));
    glUniformMatrix4fv(scene_gl_data.shadowmap__projection, 1, GL_FALSE, reinterpret_cast<float *>(&shadowmap_projection));

    { // Draw full scene
        glBindVertexArray(scene_gl_data.vao);

        for (obj_data::face_data face : scene.faces) {
            glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
        }
    }
}

void draw_scene(
    const obj_data & scene,
    const gl_data & scene_gl_data,
    const glm::vec3 & camera_position,
    const float camera_angle,
    const float width,
    const float height,
    const float near,
    const float far,
    const float time,
    glm::mat4 & shadowmap_projection
) {
    glm::mat4 model(1.f);

    glm::mat4 view(1.f);
    view = glm::rotate(view, camera_angle, UP);
    view = glm::translate(view, -camera_position);

    glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * width) / height, near, far);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClearColor(CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, 1.0f);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(scene_gl_data.source_program);
    glUniformMatrix4fv(scene_gl_data.source__model, 1, GL_FALSE, reinterpret_cast<float *>(&model));
    glUniformMatrix4fv(scene_gl_data.source__view, 1, GL_FALSE, reinterpret_cast<float *>(&view));
    glUniformMatrix4fv(scene_gl_data.source__projection, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

    glUniform3f(scene_gl_data.source__ambient_light, AMBIENT_COLOR.r, AMBIENT_COLOR.g, AMBIENT_COLOR.b);
    
    glUniform3f(scene_gl_data.source__sun_direction, SUN_DIRECTION.x, SUN_DIRECTION.y, SUN_DIRECTION.z);
    glUniform3f(scene_gl_data.source__sun_color, SUN_COLOR.r, SUN_COLOR.g, SUN_COLOR.b);

    glm::vec3 current_light_position = LIGHT_POSITION + LIGHT_DELTA * std::sin(time * 0.3f);
    glUniform3f(scene_gl_data.source__point_light_position, current_light_position.x, current_light_position.y, current_light_position.z);
    glUniform3f(scene_gl_data.source__point_light_color, LIGHT_COLOR.r, LIGHT_COLOR.g, LIGHT_COLOR.b);
    glUniform3f(scene_gl_data.source__point_light_attenuation, LIGHT_ATTENUATION.x, LIGHT_ATTENUATION.y, LIGHT_ATTENUATION.z);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scene_gl_data.shadow_map);
    glUniform1i(scene_gl_data.source__shadowmap_texture, 2);
    glUniformMatrix4fv(scene_gl_data.source__shadowmap_projection, 1, GL_FALSE, reinterpret_cast<float *>(&shadowmap_projection));
    
    { // Draw full scene
        glBindVertexArray(scene_gl_data.vao);

        for (obj_data::face_data face : scene.faces) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, face.albedo_texture);
            glUniform1i(scene_gl_data.source__albedo_texture, 0);

            if (face.alpha_texture != 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, face.alpha_texture);
                glUniform1i(scene_gl_data.source__alpha_texture, 1);
                glUniform1f(scene_gl_data.source__has_alpha_texture, 1.0f);
            } else {
                glUniform1f(scene_gl_data.source__has_alpha_texture, 0.0f);
            }

            glUniform3f(scene_gl_data.source__glossiness, face.glossiness[0], face.glossiness[1], face.glossiness[2]);
            glUniform1f(scene_gl_data.source__roughness, face.power);

            glDrawArrays(GL_TRIANGLES, face.firstVertex, face.countVertex);
        }
    }
}

void draw_debug(
    const obj_data & scene,
    const gl_data & scene_gl_data
) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(scene_gl_data.debug_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_gl_data.shadow_map);
    glUniform1i(scene_gl_data.debug__shadowmap_texture, 0);

    glBindVertexArray(scene_gl_data.debug_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
