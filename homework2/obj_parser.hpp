#pragma once

#include <array>
#include <vector>

struct obj_data
{
    struct vertex
    {
        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 2> texcoord;
    };

    std::vector<vertex> vertices;
};
