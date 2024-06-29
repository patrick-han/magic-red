#include "PointLight.h"
#include <utility>

PointLight::PointLight(glm::vec3 _worldSpacePosition, uint32_t _type, glm::vec3 _color, float _intensity) 
    : worldSpacePosition(_worldSpacePosition)
    , type(_type)
    , color(_color)
    , intensity(_intensity)
{}

// PointLight::PointLight(glm::vec3 _worldSpacePosition, glm::vec3 _color) 
//     : worldSpacePosition(_worldSpacePosition)
//     , color(_color)
// {}
