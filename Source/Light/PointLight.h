#pragma once

#include <glm/vec4.hpp>

struct PointLight {
    PointLight(glm::vec4 _worldSpacePosition, glm::vec4 _color);
    glm::vec4 worldSpacePosition;
    glm::vec4 color;
};