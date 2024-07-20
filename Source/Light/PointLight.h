#pragma once

#include <glm/vec3.hpp>

struct PointLight {
    PointLight(glm::vec3 _worldSpacePosition, float _intensity, glm::vec3 _color, float constantAtten, float linearAtten, float quadraticAtten);
    glm::vec3 worldSpacePosition;
    float intensity;
    glm::vec3 color;

    struct AttenuationTerms
    {
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
    };

    AttenuationTerms attenuationTerms;
    // PointLight(glm::vec3 _worldSpacePosition, glm::vec3 _color);
    // glm::vec3 worldSpacePosition;
    // glm::vec3 color;
};