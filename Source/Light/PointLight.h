#pragma once

#include <glm/vec3.hpp>

#define TYPE_DIRECTIONAL_LIGHT 0
#define TYPE_POINT_LIGHT 1
#define TYPE_SPOT_LIGHT 2

struct PointLight {
    PointLight(glm::vec3 _worldSpacePosition, uint32_t _type, glm::vec3 _color, float _intensity);
    glm::vec3 worldSpacePosition;
    uint32_t type;
    glm::vec3 color;
    float intensity;
    // PointLight(glm::vec3 _worldSpacePosition, glm::vec3 _color);
    // glm::vec3 worldSpacePosition;
    // glm::vec3 color;
};