#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// https://github.com/eliasdaler/edbr/blob/master/edbr/include/edbr/Graphics/CPUMesh.h
struct Vertex {
    glm::vec3 position;
    float uv_x{};
    glm::vec3 normal;
    float uv_y{};
    glm::vec4 tangent;
    glm::vec4 color;
};
