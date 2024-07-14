#pragma once
#include <Vertex/Vertex.h>
#include <Wrappers/Buffer.h>
#include <Common/IdTypes.h>
#include <vector>
#include <unordered_map>

struct CPUMesh {
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    MaterialId m_materialId{NULL_MATERIAL_ID};
};

struct GPUMesh {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer  indexBuffer;
    uint32_t indexCount;
    MaterialId m_materialId{NULL_MATERIAL_ID};

    void cleanup(VmaAllocator allocator);
};