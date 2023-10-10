#pragma once

#include "Mesh/Mesh.h"
#include "Material.h"
#include <glm/glm.hpp>

struct RenderMesh {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};