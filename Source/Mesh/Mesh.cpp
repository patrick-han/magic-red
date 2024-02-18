#include "Mesh/Mesh.h"
#include "vulkan/vulkan.h"
#include "Common/Log.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "External/tiny_obj_loader.h"
#include <Common/Compiler/Unused.h>

void load_mesh_from_obj(Mesh& mesh, const char* fileName, MeshColor overrideColor) {
    // Attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib;
    // Shapes contains the info for each separate object in the file
    std::vector<tinyobj::shape_t> shapes;
    // Materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

    // Error and warning output from the load function
    std::string warn;
    std::string err;

    // Load the OBJ file
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName, nullptr);
    // Make sure to output the warnings to the console, in case there are issues with the file
    if (!warn.empty()) {
        MRWARN(warn);
    }
    // If we have any error, print it to the console, and break the mesh loading.
    // This happens if the file can't be found or is malformed
    if (!err.empty()) {
        MRCERR("Failed to load mesh: " <<  fileName);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            //hardcode loading to triangles
            uint32_t fv = 3;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                //vertex position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                //vertex normal
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                //copy it into our vertex
                Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                // Override vertex colors for debug
                switch(overrideColor) {
                    case MeshColor::Red:
                        new_vert.color.x = 1.0f;
                        new_vert.color.y = 0.2f;
                        new_vert.color.z = 0.2f;
                    break;
                    case MeshColor::Green:
                        new_vert.color.x = 0.2f;
                        new_vert.color.y = 1.0f;
                        new_vert.color.z = 0.2f;
                    break;
                    case MeshColor::Blue:
                        new_vert.color.x = 0.2f;
                        new_vert.color.y = 0.2f;
                        new_vert.color.z = 1.0f;
                    break;
                }


                mesh.vertices.push_back(new_vert);
            }
            index_offset += fv;
        }
    }
}

void load_mesh(Mesh& mesh, const char* meshFileName) {
    UNUSED(mesh);
    UNUSED(meshFileName);
}

[[nodiscard]] Mesh& upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue& deletionQueue) {
    upload_buffer(mesh.vertexBuffer, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, allocator, deletionQueue);

    if (mesh.indices.size() > 0) {
        upload_buffer(mesh.indexBuffer, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, allocator, deletionQueue);
    }
    return mesh;
}

[[nodiscard]] Mesh* get_mesh(const std::string& meshName, std::unordered_map<std::string, Mesh>& meshMap) {
    // Search for the mesh, and return nullptr if not found
    auto it = meshMap.find(meshName);
    if (it == meshMap.end()) {
        MRCERR("Could not find mesh: " << meshName);
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}