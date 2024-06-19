#pragma once
#include <Vertex/Vertex.h>
#include <Wrappers/Buffer.h>
#include <vector>
#include <unordered_map>


enum class MeshColor {
    Red,
    Green,
    Blue
};


struct CPUMesh {
    CPUMesh(const char* fileName, bool isBinary);
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct GPUMesh {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer  indexBuffer;
    uint32_t indexCount;

    void cleanup(VmaAllocator allocator);
};