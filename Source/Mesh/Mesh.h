#pragma once
#include <Vertex/Vertex.h>
#include <Wrappers/Buffer.h>
#include <DeletionQueue.h>
#include <vector>
#include <unordered_map>


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

/* Load Mesh data from an .gltf or .glb file */
void load_mesh_from_gltf(Mesh& mesh, const char* fileName, bool isBinary);

/* Upload an instantiated Mesh to the GPU using a created VMA allocator */
[[nodiscard]] Mesh& upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue& deletionQueue);

/* Get a mesh from a scene mesh map */
[[nodiscard]] Mesh* get_mesh(const std::string& meshName, std::unordered_map<std::string, Mesh>& meshMap);