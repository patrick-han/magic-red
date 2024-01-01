#pragma once
#include "Vertex/Vertex.h"
#include "Wrappers/Buffer.h"
#include "DeletionQueue.h"
#include <vector>



enum class MeshColor {
    Red,
    Green,
    Blue
};


struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer  indexBuffer;
};

/* Load Mesh data from an .obj file */
void load_mesh_from_obj(Mesh& mesh, const char* objFileName, MeshColor overrideColor);

/* Load Mesh data using ASSIMP */
void load_mesh(Mesh& mesh, const char* meshFileName);

/* Upload an instantiated Mesh to the GPU using a created VMA allocator */
[[nodiscard]] Mesh& upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue& deletionQueue);

/* Get a mesh from a scene mesh map */
[[nodiscard]] Mesh* get_mesh(const std::string& meshName, std::unordered_map<std::string, Mesh>& meshMap);