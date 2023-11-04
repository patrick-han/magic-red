#pragma once

#include <glm/glm.hpp>

class Mesh;
class Material;

struct RenderMesh {
    Mesh* mesh;
    Material* material;
    glm::mat4 transformMatrix;
};