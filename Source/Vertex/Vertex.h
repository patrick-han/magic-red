#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;

    static size_t sizeInBytes()
    {
        return sizeof(Vertex);
    }
};
