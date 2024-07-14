#pragma once
#include <Vertex/Vertex.h>
#include <Wrappers/Buffer.h>
#include <Common/IdTypes.h>
#include <vector>
#include <unordered_map>
#include <glm/mat4x4.hpp>

struct CPUMesh {
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    MaterialId m_materialId{NULL_MATERIAL_ID};
    glm::mat4x4 m_transform{0.0};
};

struct GPUMesh {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer  indexBuffer;
    uint32_t indexCount;
    MaterialId m_materialId{NULL_MATERIAL_ID};

    void cleanup(VmaAllocator allocator);
};