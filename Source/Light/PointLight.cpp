#include "PointLight.h"
#include <utility>

PointLight::PointLight(glm::vec4 _worldSpacePosition, glm::vec4 _color = glm::vec4(1.0, 0.0, 0.0, 1.0)) 
    : worldSpacePosition(_worldSpacePosition)
    , color(_color) 
{}
