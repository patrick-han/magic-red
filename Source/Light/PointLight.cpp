#include "PointLight.h"
#include <utility>

PointLight::PointLight(glm::vec3 _worldSpacePosition, float _intensity, glm::vec3 _color, float constantAtten, float linearAtten, float quadraticAtten) 
    : worldSpacePosition(_worldSpacePosition)
    , color(_color)
    , intensity(_intensity)
    , attenuationTerms{constantAtten, linearAtten, quadraticAtten}
{}