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

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT // Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust triangulation. Requires C++11
#include "tiny_obj_loader.h"

#include "obj_parser.hpp"
#include "source_shaders.hpp"
#include "debug_shaders.hpp"

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

GLuint create_shader(GLenum type, const char * source)
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

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
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

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto source_program = source_shader_program(create_program(vertex_shader, fragment_shader), vertex_shader, fragment_shader);

    auto debug_vs = create_shader(GL_VERTEX_SHADER, debug_vertex_shader);
    auto debug_fs = create_shader(GL_FRAGMENT_SHADER, debug_fragment_shader);
    auto debug_program = create_program(debug_vs, debug_fs);

    std::string project_root = PROJECT_ROOT;
    std::string scene_path = project_root + "/data/sponza/sponza.obj";
    std::string scene_dir = project_root + "/data/sponza/";
    // std::string scene_path = project_root + "/suzanne.obj";
    // std::string scene_dir = project_root;
    obj_data scene = parse_obj(scene_path);
    
    /*{ // Load scene
        std::string inputfile = scene_path;
        std::string mtl_basedir = scene_dir;
        bool triangulate = true;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str(), mtl_basedir.c_str(), triangulate);

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!ret) {
            exit(1);
        }

        std::cout << "Loaded " << shapes.size() << " shapes\n";
        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    obj_data::vertex current_v;

                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                    tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                    tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
                    current_v.position = { vx, vy, vz };                   

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0) {
                        tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                        tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                        tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                        current_v.normal = { nx, ny, nz };
                    } else {
                        current_v.normal = { 0.f, 0.f, 0.f };
                    }

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0) {
                        tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                        tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                        current_v.texcoord = { tx, };
                    } {
                        current_v.texcoord = { 0.f, 0.f };
                    }
                    // Optional: vertex colors
                    // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                    // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                    // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

                    scene.vertices.push_back(current_v);
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }

        for (int i = 0; i < scene.vertices.size(); i++) {
            scene.indices.push_back(i);
        }
    } */

    { // Log obj info
        float minx = 1e9;
        float miny = 1e9;
        float minz = 1e9;
        float maxx = -1e9;
        float maxy = -1e9;
        float maxz = -1e9;
    
        for (auto v: scene.vertices) {
            minx = std::min(minx, v.position[0]);
            miny = std::min(miny, v.position[1]);
            minz = std::min(minz, v.position[2]);
            maxx = std::max(maxx, v.position[0]);
            maxy = std::max(maxy, v.position[1]);
            maxz = std::max(maxz, v.position[2]);
        }
        
        glm::vec3 scene_bouding_box_min(minx, miny, minz);
        glm::vec3 scene_bouding_box_max(maxx, maxy, maxz);
    
        std::cout << "Loaded scene" << "\n"
            << "Vertexes: " << scene.vertices.size() << '\n'
            << "Indices: " << scene.indices.size() << '\n'
            << "Bouding box min: " << scene_bouding_box_min.x << ' ' << scene_bouding_box_min.y << ' ' << scene_bouding_box_min.z << '\n'
            << "Bouding box max: " << scene_bouding_box_max.x << ' ' << scene_bouding_box_max.y << ' ' << scene_bouding_box_max.z << '\n'
            << std::endl;
    }

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, scene.vertices.size() * sizeof(scene.vertices[0]), scene.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, scene.indices.size() * sizeof(scene.indices[0]), scene.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void*)(12));

    GLuint debug_vao;
    glGenVertexArrays(1, &debug_vao);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;
    bool paused = false;

    std::map<SDL_Keycode, bool> button_down;

    float camera_speed = 500.f;
    float camera_rotation_speed = glm::pi<float>() / 2;
    glm::vec3 UP(0.f, -1.f, 0.f);
    glm::vec3 RGH(1.f, 0.f, 0.f);
    glm::vec3 FWD(0.f, 0.f, 1.f);
    glm::vec3 camera_position(1000.f, -125.f, 30.f);
    float camera_angle = 3 * glm::pi<float>() / 2;

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
                camera_position += UP * dt * camera_speed;
                position_changed = true;
            }
            if (button_down[SDLK_s]) {
                camera_position -= UP * dt * camera_speed;
                position_changed;
            }
            if (button_down[SDLK_UP]) {
                camera_position += (FWD * std::cos(camera_angle) + RGH * std::sin(camera_angle)) * camera_speed * dt;
                position_changed = true;
            }
            if (button_down[SDLK_DOWN]) {
                camera_position -= (FWD * std::cos(camera_angle) + RGH * std::sin(camera_angle)) * camera_speed * dt;
                position_changed = true;
            }
            
            if (button_down[SDLK_d])
                camera_angle -= camera_rotation_speed * dt;
            if (button_down[SDLK_a])
                camera_angle += camera_rotation_speed * dt;

            if (position_changed) {
                std::cout << "Position changed: " 
                    << camera_position.x << ' ' 
                    << camera_position.y << ' ' 
                    << camera_position.z << ' '
                    << "( Angle: " << camera_angle << " )"
                    << std::endl;
            }
        }

        { // Draw scene
            glm::mat4 model(1.f);
    
            glm::mat4 view(1.f);
            view = glm::rotate(view, camera_angle, UP);
            view = glm::translate(view, camera_position);
    
            glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * width) / height, near, far);
    
            glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();
    
            glClearColor(0.5f, 0.f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            glUseProgram(source_program.program);
            glUniformMatrix4fv(source_program.model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
            glUniformMatrix4fv(source_program.view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
            glUniformMatrix4fv(source_program.projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
            glUniform3fv(source_program.camera_position_location, 1, (float *) (&camera_position));
            glUniform3f(source_program.albedo_location, 0.7f, 0.4f, 0.2f);
            glUniform3f(source_program.ambient_light_location, 0.2f, 0.2f, 0.2f);
            glUniform3f(source_program.sun_direction_location, (-UP - RGH).x, (-UP - RGH).y, (-UP - RGH).z);
            glUniform3f(source_program.sun_color_location, 1.0f, 0.9f, 0.8f);
    
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, scene.indices.size(), GL_UNSIGNED_INT, nullptr);
        }

        { // Draw debug
            glUseProgram(debug_program);
            glBindVertexArray(debug_vao);
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
