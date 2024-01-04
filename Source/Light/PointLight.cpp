#include "PointLight.h"
#include <utility>

PointLight::PointLight(glm::vec3 _worldSpacePosition, glm::vec3 _color) 
    : worldSpacePosition(_worldSpacePosition)
    , color(_color) 
{}
