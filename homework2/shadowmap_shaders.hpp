#pragma once

const char shadowmap_vertex_shader[] =
R"(#version 330 core

uniform mat4 model;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;

void main()
{
    gl_Position = projection * model * vec4(in_position, 1.0);
}
)";

const char shadowmap_fragment_shader[] =
R"(#version 330 core

out vec4 shadow_output;

void main()
{
    float z = gl_FragCoord.z;
    shadow_output = vec4(z, z * z + 1.0 / 4 * (dFdx(z) * dFdx(z) + dFdy(z) * dFdy(z)), 0, 0);
}
)";
