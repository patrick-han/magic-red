#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>


struct DirectionalLight {
    DirectionalLight(glm::vec3 _direction);
    glm::vec4 direction{0.0f}; // Defined as pointing _away_ from the light
};