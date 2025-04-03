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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <vector>
#include <array>
#include "data.hpp"
#include "pool-shader.cpp"

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

    Settings settings = Settings();
    Camera camera = {
        glm::vec3(-10.f, 10.f, -10.f),
        glm::vec3(10.f, -10.f, 10.f),
        glm::vec3(5.f, 10.f, 5.f),
    };
    SDL_GetWindowSize(window, &settings.width, &settings.height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    std::string project_root = PROJECT_ROOT;

    PoolShader poolShader = PoolShader();
    SkyboxShader skyboxShader = SkyboxShader();
    { // Prepare pool texture
        int texture_width, texture_height, texture_cpx;
        std::string path = project_root + "/data/pool.png";
        unsigned char * texture_pixels = stbi_load(
            path.c_str(),
            &texture_width,
            &texture_height,
            &texture_cpx,
            4);

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(texture_pixels);

        poolShader.albedoTexture = textureID;
    }
    { // Prepare pool environment
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        std::vector<std::string> faces = {
            "/data/clouds/clouds1_north.bmp",
            "/data/clouds/clouds1_south.bmp",
            "/data/clouds/clouds1_up.bmp",
            "/data/clouds/clouds1_down.bmp",
            "/data/clouds/clouds1_west.bmp",
            "/data/clouds/clouds1_east.bmp"
        };
        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char *data = stbi_load((project_root + faces[i]).c_str(), &width, &height, &nrChannels, 0);

            if (data) {
                std::cout << "Loaded " << (project_root + faces[i]) << ": " << width << 'x' << height << std::endl;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            } else {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        poolShader.environmentTexture = textureID;
        skyboxShader.environmentTexture = textureID;
    }

    { // Put verticies
        std::vector<vertex> verticies;
        verticies.push_back({ { -10.f, 0.f, -10.f } });
        verticies.push_back({ { -10.f, 0.f,  10.f } });
        verticies.push_back({ {  10.f, 0.f, -10.f } });
        verticies.push_back({ { -10.f, 0.f,  10.f } });
        verticies.push_back({ {  10.f, 0.f, -10.f } });
        verticies.push_back({ {  10.f, 0.f,  10.f } });
        poolShader.UpdateBufferData(verticies);
    }

    float time = 0.f;

    auto last_frame_start = std::chrono::high_resolution_clock::now();
    
    std::map<SDL_Keycode, bool> button_down;
    std::map<SDL_Keycode, bool> button_click;

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
        }

        { // Clear
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glClearColor(settings.clear_r, settings.clear_g, settings.clear_b, 1.0f);
            glViewport(0, 0, settings.width, settings.height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        { // Draw skybox
            glDisable(GL_DEPTH_TEST);
            skyboxShader.view = glm::lookAt(camera.position, camera.position + camera.direction, glm::normalize(camera.up));
            skyboxShader.projection = glm::perspective(glm::pi<float>() / 2, (1.f * settings.width) / settings.height, settings.near, settings.far);
            skyboxShader.camera_position = camera.position;
            skyboxShader.Draw(0, 6);
        }

        { // Draw
            glEnable(GL_DEPTH_TEST);
            poolShader.model = glm::mat4(1.f);
            poolShader.view = glm::lookAt(camera.position, camera.position + camera.direction, glm::normalize(camera.up));
            poolShader.projection = glm::perspective(glm::pi<float>() / 2, (1.f * settings.width) / settings.height, settings.near, settings.far);
            poolShader.camera_position = camera.position;
            poolShader.ambient_light = glm::vec3(0.1f, 0.1f, 0.05f);
            poolShader.sun_color = glm::vec3(0.8f, 0.8f, 0.5f);
            poolShader.sun_direction = glm::vec3(0.5f, 1.f, 0.5f);
            poolShader.glossiness = glm::vec3(1.0f);
            poolShader.shininess = 32.f;

            poolShader.Draw(0, 6);
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
