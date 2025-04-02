#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT // Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust triangulation. Requires C++11
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <string>
#include <vector>
#include <iostream>
#include "data.hpp"
#include "scene_init.hpp"

std::vector<std::vector<obj_data::vertex>> group_verticies_by_material(
    const tinyobj::attrib_t & attrib,
    const std::vector<tinyobj::shape_t> & shapes,
    const std::vector<tinyobj::material_t> & materials
) {
    std::vector<std::vector<obj_data::vertex>> vertices_grouped_by_material(materials.size());

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            size_t material_idx = shapes[s].mesh.material_ids[f];

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                obj_data::vertex current_v;
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
                current_v.position = { vx, vy, vz };                   

                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    current_v.normal = { nx, ny, nz };
                }

                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    current_v.texcoord = { tx, ty };
                }

                vertices_grouped_by_material[material_idx].push_back(current_v);
            }
            index_offset += fv;
        }
    }

    
    return vertices_grouped_by_material;
}

obj_data make_scene(
    const std::string & scene_dir,
    const std::vector<std::vector<obj_data::vertex>> & vertices_grouped_by_material,
    const std::vector<tinyobj::material_t> & materials
) {
    obj_data scene;

    for (size_t material_idx = 0; material_idx < materials.size(); material_idx++) {
        obj_data::face_data current_face_data;
        current_face_data.firstVertex = scene.vertices.size();

        for (obj_data::vertex v : vertices_grouped_by_material[material_idx]) {
            scene.vertices.push_back(v);
        }

        tinyobj::material_t material = materials.at(material_idx);
        current_face_data.countVertex = scene.vertices.size() - current_face_data.firstVertex;
        current_face_data.material_name = material.name;
        current_face_data.albedo_texname = material.ambient_texname;
        current_face_data.alpha_texname = material.alpha_texname;
        current_face_data.bump_texname = material.bump_texname;
        current_face_data.specular_texname = material.specular_texname;
        current_face_data.glossiness = { material.specular[0], material.specular[1], material.specular[2] };
        current_face_data.power = material.shininess;

        if (current_face_data.albedo_texname != "") {
            std::string texture_path = current_face_data.albedo_texname;
            texture_path = texture_path.replace(texture_path.find("\\", 0), 1, "/");
            texture_path = scene_dir + texture_path;

            int texture_width, texture_height, texture_cpx;
            unsigned char * texture_pixels = stbi_load(
                texture_path.c_str(),
                &texture_width,
                &texture_height,
                &texture_cpx,
                4);

            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(texture_pixels);

            current_face_data.albedo_texture = textureID;
        } else {
            current_face_data.albedo_texture = 0;
        }

        if (current_face_data.alpha_texname != "") {
            std::string texture_path = current_face_data.alpha_texname;
            texture_path = texture_path.replace(texture_path.find("\\", 0), 1, "/");
            texture_path = scene_dir + texture_path;

            int texture_width, texture_height, texture_cpx;
            unsigned char * texture_pixels = stbi_load(
                texture_path.c_str(),
                &texture_width,
                &texture_height,
                &texture_cpx,
                4);


            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(texture_pixels);

            current_face_data.alpha_texture = textureID;
        } else {
            current_face_data.alpha_texture = 0;
        }

        if (current_face_data.bump_texname != "") {
            std::string texture_path = current_face_data.bump_texname;
            texture_path = texture_path.replace(texture_path.find("\\", 0), 1, "/");
            texture_path = scene_dir + texture_path;

            int texture_width, texture_height, texture_cpx;
            unsigned char * texture_pixels = stbi_load(
                texture_path.c_str(),
                &texture_width,
                &texture_height,
                &texture_cpx,
                4);


            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(texture_pixels);

            current_face_data.bump_texture = textureID;
        } else {
            current_face_data.bump_texture = 0;
        }

        if (current_face_data.specular_texname != "") {
            std::string texture_path = current_face_data.specular_texname;
            texture_path = texture_path.replace(texture_path.find("\\", 0), 1, "/");
            texture_path = scene_dir + texture_path;

            int texture_width, texture_height, texture_cpx;
            unsigned char * texture_pixels = stbi_load(
                texture_path.c_str(),
                &texture_width,
                &texture_height,
                &texture_cpx,
                4);


            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(texture_pixels);

            current_face_data.specular_texture = textureID;
        } else {
            current_face_data.specular_texture = 0;
        }

        scene.faces.push_back(current_face_data);
    }

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

    scene.bbox_min = glm::vec3(minx, miny, minz);
    scene.bbox_max = glm::vec3(maxx, maxy, maxz);

    return scene;
}

void log_scene(
    const obj_data & scene,
    const std::vector<tinyobj::shape_t> & shapes,
    const std::vector<tinyobj::material_t> & materials
) {
    std::cout << "Loaded:\n"
              << "- shapes:     " << shapes.size() << "\n"
              << "- materials:  " << materials.size() << "\n"
              << "- vertices:   " << scene.vertices.size() << "\n"
              << "- faces:      " << scene.faces.size() << "\n"
              << "- bbox min:   " << scene.bbox_min.x << ' ' << scene.bbox_min.y << ' ' << scene.bbox_min.z << '\n'
              << "- bbox max:   " << scene.bbox_max.x << ' ' << scene.bbox_max.y << ' ' << scene.bbox_max.z << '\n'
              << std::endl;
}

obj_data load_scene(const std::string & scene_path, const std::string & scene_dir) {
    bool triangulate = true;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, scene_path.c_str(), scene_dir.c_str(), triangulate);
    if (!err.empty()) 
        std::cerr << err << std::endl;
    if (!ret)
        exit(1);

    auto vertices_grouped_by_material = group_verticies_by_material(attrib, shapes, materials);
    auto scene = make_scene(scene_dir, vertices_grouped_by_material, materials);
    log_scene(scene, shapes, materials);

    return scene;
}

obj_data load_bunny(const std::string & scene_path) {
    bool triangulate = true;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, scene_path.c_str(), nullptr, triangulate);
    if (!err.empty()) 
        std::cerr << err << std::endl;
    if (!ret)
        exit(1);
        
    obj_data scene;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                obj_data::vertex current_v;
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
                current_v.position = { vx, vy, vz };                   

                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    current_v.normal = { nx, ny, nz };
                }

                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    current_v.texcoord = { tx, ty };
                }

                scene.vertices.push_back(current_v);
            }

            index_offset += fv;
        }
    }

    obj_data::face_data bunny_face_data;
    bunny_face_data.firstVertex = 0;
    bunny_face_data.countVertex = scene.vertices.size();
    bunny_face_data.glossiness = { 1.f, 1.f, 1.f };
    bunny_face_data.power = 32.f;
    scene.faces.push_back(bunny_face_data);

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

    scene.bbox_min = glm::vec3(minx, miny, minz);
    scene.bbox_max = glm::vec3(maxx, maxy, maxz);

    log_scene(scene, {}, {});

    return scene;
}
