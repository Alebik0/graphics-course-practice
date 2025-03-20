#pragma once

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
    };

    std::vector<vertex> vertices;
    std::vector<face_data> faces;
};
