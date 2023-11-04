#pragma once

#include <glm/glm.hpp>

class Mesh;
class GraphicsPipeline;

struct RenderMesh {
    Mesh* mesh;
    GraphicsPipeline* material;
    glm::mat4 transformMatrix;
};