#include "DirectionalLight.h"

DirectionalLight::DirectionalLight() : direction(0.0f, 0.0f, 0.0f), power(1.0f) {}

// When defining a direction vector as a vec4 we don't want translations to have an effect 
// (since they just represent directions, nothing more) so we define the w component to be 0.0. 
DirectionalLight::DirectionalLight(glm::vec3 _direction, float _power) : direction(_direction.x, _direction.y, _direction.z), power(_power) {}