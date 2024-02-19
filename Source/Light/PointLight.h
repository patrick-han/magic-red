#pragma once

#include <glm/vec3.hpp>

struct PointLight {
    PointLight(glm::vec3 _worldSpacePosition, glm::vec3 _color);
    glm::vec3 worldSpacePosition;
    glm::vec3 color;
};