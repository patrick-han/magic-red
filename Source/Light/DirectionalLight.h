#pragma once

#include <glm/vec3.hpp>


struct DirectionalLight {
    DirectionalLight();
    DirectionalLight(glm::vec3 _direction, float _power);
    glm::vec3 direction; // Defined as pointing _away_ from the light
    float power;
};