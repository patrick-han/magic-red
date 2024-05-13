#pragma once
#include <Rendering/Vertex/Vertex.h>
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
};

/* Load Mesh data from an .gltf or .glb file */
void load_mesh_from_gltf(Mesh& mesh, const char* fileName, bool isBinary);

/* Get a mesh from a scene mesh map */
[[nodiscard]] Mesh* get_mesh(const std::string& meshName, std::unordered_map<std::string, Mesh>& meshMap);
