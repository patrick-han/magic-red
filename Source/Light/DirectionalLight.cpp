#include "DirectionalLight.h"

// When defining a direction vector as a vec4 we don't want translations to have an effect 
// (since they just represent directions, nothing more) so we define the w component to be 0.0. 
DirectionalLight::DirectionalLight(glm::vec3 _direction) : direction(glm::vec4(_direction.x, _direction.y, _direction.z, 0.0f)) {}