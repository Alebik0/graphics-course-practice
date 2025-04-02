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

#include "data.hpp"
#include "globals.hpp"
#include "scene_init.hpp"
#include "shaders.hpp"

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

glm::mat4 make_sun_shadowmap_projection(
    const obj_data & scene,
    const glm::vec3 & sun_direction
) {
    glm::vec3 C(0.f);
    glm::vec3 light_z = -glm::normalize(sun_direction);
    glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 1.f, 0.f}));
    glm::vec3 light_y = glm::cross(light_x, light_z);

    float max_value_x = 0;
    float max_value_y = 0;
    float max_value_z = 0;
    for (float v_x : {scene.bbox_min.x, scene.bbox_max.x}) {
        for (float v_y : {scene.bbox_min.y, scene.bbox_max.y}) {
            for (float v_z : {scene.bbox_min.z, scene.bbox_max.z}) {
                glm::vec3 V = glm::vec3(v_x, v_y, v_z);
                max_value_x = std::max(max_value_x, abs(glm::dot(V - C, light_x)));
                max_value_y = std::max(max_value_y, abs(glm::dot(V - C, light_y)));
                max_value_z = std::max(max_value_z, abs(glm::dot(V - C, light_z)));
            }
        }
    }

    glm::mat4 shadowmap_projection(1.f);
    shadowmap_projection[0] = {max_value_x * light_x, 0};
    shadowmap_projection[1] = {max_value_y * light_y, 0};
    shadowmap_projection[2] = {max_value_z * light_z, 0};
    shadowmap_projection[3] = {C, 1};
    shadowmap_projection = glm::inverse(shadowmap_projection);

    return shadowmap_projection;
    // return glm::ortho(-max_value_y, max_value_y, -max_value_x, max_value_x, -max_value_z, max_value_z) * glm::lookAt(C, C + sun_direction, light_x);
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

    Settings settings = Settings();
    SDL_GetWindowSize(window, &settings.width, &settings.height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    std::string project_root = PROJECT_ROOT;
    std::string scene_path = project_root + "/data/sponza/sponza.obj";
    std::string bunny_path = project_root + "/data/bunny/bunny.obj";
    std::string scene_dir = project_root + "/data/sponza/";
    obj_data scene = load_scene(scene_path, scene_dir);
    obj_data bunny = load_bunny(bunny_path);

    float time = 0.f;
    bool paused = false;
    bool bump_mark = false;
    bool specular_mark = false;
    bool gamma_correction_mark = false;
    bool aces_correction_mark = false;
    auto last_frame_start = std::chrono::high_resolution_clock::now();
    
    std::map<SDL_Keycode, bool> button_down;
    std::map<SDL_Keycode, bool> button_click;

    Camera camera = Camera();
    SourceShader sourceShader = SourceShader();
    ShadowmapShader shadowmapShader = ShadowmapShader();
    DebugShader debugShader = DebugShader();
    { // Fill vbo
        std::vector<obj_data::vertex> verticies;
        for (const auto v : scene.vertices)
            verticies.push_back(v);
        for (const auto v : bunny.vertices)
            verticies.push_back(v);
            
        sourceShader.UpdateBufferData(verticies);
    }

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
                settings.width = event.window.data1;
                settings.height = event.window.data2;
                break;
            }
            break;
        case SDL_KEYDOWN:
            button_click[event.key.keysym.sym] = !button_down[event.key.keysym.sym];
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

            if (!paused)
                time += dt;

            if (button_down[SDLK_w]) {
                camera.MoveForward(dt);
            }
            if (button_down[SDLK_s]) {
                camera.MoveBackward(dt);
            }
            if (button_down[SDLK_LSHIFT]) {
                camera.MoveUpward(dt);
            }
            if (button_down[SDLK_LCTRL]) {
                camera.MoveDownward(dt);
            }
            if (button_down[SDLK_LEFT]) {
                camera.RotateLeft(dt);
            }
            if (button_down[SDLK_RIGHT]) {
                camera.RotateRight(dt);
            }

            if (button_click[SDLK_SPACE]) {
                paused = !paused;
                button_click[SDLK_SPACE] = false;
            }
            if (button_click[SDLK_1]) {
                bump_mark = !bump_mark;
                button_click[SDLK_1] = false;
            }
            if (button_click[SDLK_2]) {
                specular_mark = !specular_mark;
                button_click[SDLK_2] = false;
            }
            if (button_click[SDLK_3]) {
                gamma_correction_mark = !gamma_correction_mark;
                button_click[SDLK_3] = false;
            }
            if (button_click[SDLK_4]) {
                aces_correction_mark = !aces_correction_mark;
                button_click[SDLK_4] = false;
            }
        }

        SunLight sun = { UP + RGH * std::sin(time * 0.3f) + FWD * 0.5f, SUN_COLOR };
        PointLight light = { LIGHT_POSITION + LIGHT_DELTA * 0.4f, LIGHT_COLOR, LIGHT_ATTENUATION };

        glm::mat4 shadowmap_projection = make_sun_shadowmap_projection(scene, sun.direction);

        { // Draw shadowmap
            shadowmapShader.projection = shadowmap_projection;
            shadowmapShader.Draw(sourceShader, scene);
        }

        { // Draw reflections
            {
                Camera camera = Camera();
                camera.position = glm::vec3(0.f);

                glm::mat4 model(1.f);

                glm::mat4 view(1.f);
                view = glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    
                glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f, 1.f, settings.near, settings.far);
    
                sourceShader.model = model;
                sourceShader.view = view;
                sourceShader.projection = projection;
                sourceShader.camera = camera;
                sourceShader.ambient_light = AMBIENT_COLOR;
                sourceShader.sun = sun;
                sourceShader.light = light;
                sourceShader.shadowmapTexture = shadowmapShader.shadowmapTexture;
                sourceShader.shadowmap_projection = shadowmap_projection;
                sourceShader.bump_mark = bump_mark;
                sourceShader.specular_mark = specular_mark;
                sourceShader.gamma_correction_mark = false;
                sourceShader.aces_correction_mark = false;

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sourceShader.bunny_reflection_fbo[0]);
                glClearColor(settings.clear_r, settings.clear_g, settings.clear_b, 1.0f);
                glViewport(0, 0, REFLECTION_CUBEMAP_RESOLUTION, REFLECTION_CUBEMAP_RESOLUTION);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
                glEnable(GL_FRAMEBUFFER_SRGB); 
                glCullFace(GL_BACK);

                sourceShader.DrawScene(settings, camera, scene);
            }
        }

        { // Draw source
            sourceShader.PrepareDraw(settings);

            {
                glm::mat4 model(1.f);

                glm::mat4 view(1.f);
                view = glm::rotate(view, camera.angle, UP);
                view = glm::translate(view, -camera.position);
    
                glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * settings.width) / settings.height, settings.near, settings.far);
    
                sourceShader.model = model;
                sourceShader.view = view;
                sourceShader.projection = projection;
                sourceShader.camera = camera;
                sourceShader.ambient_light = AMBIENT_COLOR;
                sourceShader.sun = sun;
                sourceShader.light = light;
                sourceShader.shadowmapTexture = shadowmapShader.shadowmapTexture;
                sourceShader.shadowmap_projection = shadowmap_projection;
                sourceShader.bump_mark = bump_mark;
                sourceShader.specular_mark = specular_mark;
                sourceShader.gamma_correction_mark = gamma_correction_mark;
                sourceShader.aces_correction_mark = aces_correction_mark;
    
                sourceShader.DrawScene(settings, camera, scene);
            }

            {
                glm::mat4 model(1.f);
                model = glm::translate(model, glm::vec3(0.f, 25.f, 0.f));
                model = glm::scale(model, glm::vec3(100.f));

                glm::mat4 view(1.f);
                view = glm::rotate(view, camera.angle, UP);
                view = glm::translate(view, -camera.position);
    
                glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * settings.width) / settings.height, settings.near, settings.far);
    
                sourceShader.model = model;
                sourceShader.view = view;
                sourceShader.projection = projection;
                sourceShader.camera = camera;
                sourceShader.ambient_light = AMBIENT_COLOR;
                sourceShader.sun = sun;
                sourceShader.light = light;
                sourceShader.shadowmapTexture = shadowmapShader.shadowmapTexture;
                sourceShader.shadowmap_projection = shadowmap_projection;
                sourceShader.bump_mark = bump_mark;
                sourceShader.specular_mark = specular_mark;
                sourceShader.gamma_correction_mark = gamma_correction_mark;
                sourceShader.aces_correction_mark = aces_correction_mark;
    
                sourceShader.DrawBunny(settings, camera, scene, bunny);
            }
        }

        { // Draw debug
            debugShader.shadowmapTexture = sourceShader.bunny_reflection_texture;
            debugShader.Draw();
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
