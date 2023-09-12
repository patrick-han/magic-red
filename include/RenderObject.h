#pragma once

#include "Mesh.h"
#include "Material.h"
#include <glm/glm.hpp>

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};