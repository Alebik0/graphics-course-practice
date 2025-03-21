#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <string_view>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include "obj_parser.hpp"
#include "globals.hpp"
#include "gl_init.hpp"
#include "scene_init.hpp"


std::string to_string(std::string_view str)
{
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 9",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    std::string project_root = PROJECT_ROOT;
    std::string scene_path = project_root + "/data/sponza/sponza.obj";
    std::string scene_dir = project_root + "/data/sponza/";
    obj_data scene = load_scene(scene_path, scene_dir);
    gl_data scene_gl_data = init_gl(scene);

    float time = 0.f;
    bool paused = false;
    auto last_frame_start = std::chrono::high_resolution_clock::now();
    
    std::map<SDL_Keycode, bool> button_down;

    glm::vec3 camera_position = INITIAL_CAMERA_POSITION;
    float camera_angle = glm::pi<float>() / 2;
    float near = 0.1f;
    float far = 10000.f;

    bool running = true;
    while (running)
    {
        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_WINDOWEVENT: switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                width = event.window.data1;
                height = event.window.data2;
                glViewport(0, 0, width, height);
                break;
            }
            break;
        case SDL_KEYDOWN:
            button_down[event.key.keysym.sym] = true;
            break;
        case SDL_KEYUP:
            button_down[event.key.keysym.sym] = false;
            break;
        }

        if (!running)
            break;
        
        { // Update camera position
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
            last_frame_start = now;
            time += dt;

            bool position_changed = false;

            if (button_down[SDLK_w]) {
                camera_position += UP * dt * CAMERA_SPEED;
                position_changed = true;
            }
            if (button_down[SDLK_s]) {
                camera_position -= UP * dt * CAMERA_SPEED;
                position_changed;
            }
            if (button_down[SDLK_UP]) {
                camera_position += (- FWD * std::cos(camera_angle) + RGH * std::sin(camera_angle)) * CAMERA_SPEED * dt;
                position_changed = true;
            }
            if (button_down[SDLK_DOWN]) {
                camera_position -= (- FWD * std::cos(camera_angle) + RGH * std::sin(camera_angle)) * CAMERA_SPEED * dt;
                position_changed = true;
            }
            
            if (button_down[SDLK_d])
                camera_angle += CAMERA_ROTATION_SPEED * dt;
            if (button_down[SDLK_a])
                camera_angle -= CAMERA_ROTATION_SPEED * dt;

            if (position_changed) {
                std::cout << "Position changed: " 
                    << camera_position.x << ' ' 
                    << camera_position.y << ' ' 
                    << camera_position.z << ' '
                    << "( Angle: " << camera_angle << " )"
                    << std::endl;
            }
        }

        glm::vec3 sun_direction = UP + RGH * std::sin(time * 0.1f) + FWD * std::cos(time * 0.1f);
        glm::mat4 shadowmap_projection(1.f);

        { // Draw shadowmap
            glm::mat4 model(1.f);

            { // Init shadowmap shadowmap_projection
                glm::vec3 C(0.f);
                glm::vec3 light_z = -glm::normalize(sun_direction);
                glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 1.f, 0.f}));
                glm::vec3 light_y = glm::cross(light_x, light_z);
    
                float max_value_x = 0;
                float max_value_y = 0;
                float max_value_z = 0;
                for (float v_x : {-2000.f, 2000.f}) {
                    for (float v_y : {-200.f, 200.f}) {
                        for (float v_z : {-2000.f, 2000.f}) {
                            glm::vec3 V = glm::vec3(v_x, v_y, v_z);
                            max_value_x = std::max(max_value_x, abs(glm::dot(V - C, light_x)));
                            max_value_y = std::max(max_value_y, abs(glm::dot(V - C, light_y)));
                            max_value_z = std::max(max_value_z, abs(glm::dot(V - C, light_z)));
                        }
                    }
                }
    
                shadowmap_projection[0] = {max_value_x * light_x, 0};
                shadowmap_projection[1] = {max_value_y * light_y, 0};
                shadowmap_projection[2] = {max_value_z * light_z, 0};
                shadowmap_projection[3] = {C, 1};
    
                shadowmap_projection = glm::inverse(shadowmap_projection);
            }

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_gl_data.shadow_fbo);
            glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
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

        { // Draw scene
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

        { // Draw debug
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);

            glUseProgram(scene_gl_data.debug_program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, scene_gl_data.shadow_map);
            glUniform1i(scene_gl_data.debug__shadowmap_texture, 0);

            glBindVertexArray(scene_gl_data.debug_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
