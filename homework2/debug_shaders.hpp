#pragma once

const char * debug_vertex_shader = R"(#version 330 core

vec2 vertices[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

out vec2 texcoord;

void main()
{
    vec2 position = vertices[gl_VertexID];
    gl_Position = vec4(position * 0.25 + vec2(-0.75, -0.75), 0.0, 1.0);
    texcoord = position * 0.5 + vec2(0.5);
}
)";

const char * debug_fragment_shader = 
    R"(#version 330 core

uniform sampler2D shadowmapTexture;

in vec2 texcoord;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = texture(shadowmapTexture, texcoord);
}
)";
