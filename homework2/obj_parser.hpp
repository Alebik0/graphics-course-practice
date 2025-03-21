#pragma once

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <array>
#include <vector>
#include <string>

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
        std::array<float, 3> glossiness;        // TinyObj -> specular
        float power;                            // TinyObj -> shininess
        GLuint albedo_texture;
        GLuint alpha_texture;
    };

    std::vector<vertex> vertices;
    std::vector<face_data> faces;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
};
