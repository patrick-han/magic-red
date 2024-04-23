#include "PointLight.h"
#include <utility>

PointLight::PointLight(glm::vec4 _worldSpacePosition, glm::vec4 _color) 
    : worldSpacePosition(_worldSpacePosition)
    , color(_color) 
{}
