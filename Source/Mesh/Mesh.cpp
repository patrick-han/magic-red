#include <Common/Compiler/DisableWarnings.h>
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4267) // conversion from 'size_t' to 'uint32_t', possible loss of data
//TODO: remove later but here to make development faster
DISABLE_MSVC_WARNING(4101)
DISABLE_MSVC_WARNING(4189)
DISABLE_MSVC_WARNING(4201) // nonstandard extension used : nameless struct / union

#include <Mesh/Mesh.h>
#include <vulkan/vulkan.h>
#include <Common/Log.h>
#include <span>


#define TINYOBJLOADER_IMPLEMENTATION
#include <External/tiny_obj_loader.h>
#include <Common/Compiler/Unused.h>


// Define these only in *one* .cpp file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <External/tinygltf/tiny_gltf.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


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
        exit(1);
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

//class Node {
//public:
//    Node() = default;
//
//    DEFAULT_METHODS(Node)
//
//        bool operator==(const Node&) const;
//
//    int camera{ -1 };  // the index of the camera referenced by this node
//
//    std::string name;
//    int skin{ -1 };
//    int mesh{ -1 };
//    int light{ -1 };    // light source index (KHR_lights_punctual)
//    int emitter{ -1 };  // audio emitter index (KHR_audio)
//    std::vector<int> lods; // level of detail nodes (MSFT_lod)
//    std::vector<int> children;
//    std::vector<double> rotation;     // length must be 0 or 4
//    std::vector<double> scale;        // length must be 0 or 3
//    std::vector<double> translation;  // length must be 0 or 3
//    std::vector<double> matrix;       // length must be 0 or 16
//    std::vector<double> weights;  // The weights of the instantiated Morph Target
//
//    ExtensionMap extensions;
//    Value extras;
//
//    // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
//    std::string extras_json_string;
//    std::string extensions_json_string;
//};

/* gltf */
size_t component_type_to_size_in_bytes(int componentType)
{
    switch (componentType)
    {
    case 5120:
        return 1; // signed byte, 2's complement, 8 bits
    case 5121:
        return 1; // unsigned byte, 8 bits
    case 5122:
        return 2; // signed short, 2's complement, 16 bits
    case 5123:
        return 2; // unsigned short, 16 bits
    case 5125:
        return 4; // unsigined int, 32 bits
    case 5126:
        return 4; // float, 32 bits
    default:
        MRCERR("Unexpected accessor component type, perhaps the gltf is malformed!");
        exit(1);
        break;
    }
}



void get_node_properties(const tinygltf::Node& node, const tinygltf::Model& model, uint32_t& vertexCount, uint32_t& indexCount) {
    if (node.children.size() > 0)
    {
        for (size_t i = 0; i < node.children.size(); i++)
        {
            tinygltf::Node childNode = model.nodes[node.children[i]];
            get_node_properties(childNode, model, vertexCount, indexCount);
        }
    }
    if (node.mesh != -1) // -1 used to indicate validity in the tingltf library
    {
        tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
        for (size_t i = 0; i < nodeMesh.primitives.size(); i++)
        {
            tinygltf::Primitive nodeMeshPrimitive = nodeMesh.primitives[i];
            int primitivePositionAccessorIndex = nodeMeshPrimitive.attributes.find("POSITION")->second;
            vertexCount += model.accessors[primitivePositionAccessorIndex].count;
            if (nodeMeshPrimitive.indices != -1) // Meshes may not be indexed drawn
            {
                indexCount += model.accessors[nodeMeshPrimitive.indices].count;
            }
            
        }
    }
}

struct NodeLoadingData {
    std::vector<Vertex>& vertices;
    std::vector<uint32_t>& indices;
};

void load_node(const tinygltf::Node& node, const tinygltf::Model& model, NodeLoadingData& nodeLoadingData) {


    if (node.children.size() >  0)
    {
        load_node(node, model, nodeLoadingData);
    }
    if (node.mesh != -1)
    {
        tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
        for (size_t j = 0; j < nodeMesh.primitives.size(); j++)
        {
            
            tinygltf::Primitive nodeMeshPrimitive = nodeMesh.primitives[j];


            // Vertex Positions
            int primitivePositionAccessorIndex = nodeMeshPrimitive.attributes.find("POSITION")->second;
            const tinygltf::Accessor &positionAccessor = model.accessors[primitivePositionAccessorIndex];
            size_t vertexCount = positionAccessor.count;
            if (positionAccessor.type != TINYGLTF_TYPE_VEC3)
            {
                MRCERR("Position accessor isn't VEC3 type!");
                exit(1);
            }
            const tinygltf::BufferView &positionBufferView = model.bufferViews[positionAccessor.bufferView];
            size_t positionComponentTypeByteSize = component_type_to_size_in_bytes(positionAccessor.componentType);
            tinygltf::Buffer positionBuffer = model.buffers[positionBufferView.buffer];
            //                                                                                   Multiple accessors may refer 
            //                                                                                   to the same buffer view
            //                                                                                                  V
            const float* positionBufferData = reinterpret_cast<const float*>(&(positionBuffer.data[positionAccessor.byteOffset + positionBufferView.byteOffset]));
//            size_t positionByteStride = positionAccessor.ByteStride(positionBufferView);
            size_t blah = positionAccessor.ByteStride(positionBufferView);
            size_t positionByteStride = positionAccessor.ByteStride(positionBufferView) ? (positionAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);


            // Vertex Normals
            int primitiveNormalAccessorIndex = nodeMeshPrimitive.attributes.find("NORMAL")->second;
            const tinygltf::Accessor &normalAccessor = model.accessors[primitiveNormalAccessorIndex];
            if (normalAccessor.type != TINYGLTF_TYPE_VEC3)
            {
                MRCERR("Normal accessor isn't VEC3 type!");
                exit(1);
            }
            const tinygltf::BufferView &normalBufferView = model.bufferViews[normalAccessor.bufferView];
            size_t normalComponentTypeByteSize = component_type_to_size_in_bytes(normalAccessor.componentType);
            tinygltf::Buffer normalBuffer = model.buffers[normalBufferView.buffer];
            const float* normalBufferData = reinterpret_cast<const float*>(&(normalBuffer.data[normalAccessor.byteOffset + normalBufferView.byteOffset]));
//            size_t normalByteStride = normalAccessor.ByteStride(normalBufferView);
            size_t normalByteStride = normalAccessor.ByteStride(normalBufferView) ? (normalAccessor.ByteStride(normalBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);


            for (size_t vertex_i = 0; vertex_i < vertexCount; vertex_i++)
            {
                glm::vec3 position = glm::make_vec3(&positionBufferData[vertex_i * positionByteStride]);
                glm::vec3 normal = glm::normalize(glm::make_vec3(&normalBufferData[vertex_i * normalByteStride]));
                Vertex new_vertex = { position, normal, glm::vec3(0.0, 0.8, 1.0)}; // Green default
                nodeLoadingData.vertices.push_back(new_vertex);
            }

            // Indices
            int primitiveIndicesAccessorIndex = nodeMeshPrimitive.indices;
            tinygltf::Accessor indicesAccessor = model.accessors[primitiveIndicesAccessorIndex];
            size_t indexCount = indicesAccessor.count;
            if (indicesAccessor.type != TINYGLTF_TYPE_SCALAR)
            {
                MRCERR("Index accessor isn't SCALAR type!");
                exit(1);
            }
            tinygltf::BufferView indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
//            size_t indicesComponentTypeByteSize = component_type_to_size_in_bytes(indicesAccessor.componentType);
            tinygltf::Buffer indicesBuffer = model.buffers[indicesBufferView.buffer];
            const void* indexBufferDataPtr = &(indicesBuffer.data[indicesAccessor.byteOffset + indicesBufferView.byteOffset]);

            switch (indicesAccessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            {
                const uint32_t* indicesBufferData = static_cast<const uint32_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices.push_back(indicesBufferData[index_i]);
                }
            }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                const uint16_t* indicesBufferData = static_cast<const uint16_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices.push_back(indicesBufferData[index_i]);
                }
            }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                const uint8_t* indicesBufferData = static_cast<const uint8_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices.push_back(indicesBufferData[index_i]);
                }
            }
                break;
            default:
                MRCERR("Unexpected index accessor component type, perhaps the gltf is malformed!");
                exit(1);
                break;
            }
//
//            const float* indicesBufferData = reinterpret_cast<const float*>(&(indicesBuffer.data[indicesAccessor.byteOffset + indicesBufferView.byteOffset]));
//            size_t indicesByteStride = 1 * indicesComponentTypeByteSize;




            // if (nodeMeshPrimitive.indices != -1) // Meshes may not be indexed drawn
            // {
            //     nodeLoadingData.indexCount += model.accessors[nodeMeshPrimitive.indices].count;
            // }
            
        }
        UNUSED(nodeMesh);
    }
}

void load_mesh_from_gltf(Mesh& mesh, const char* fileName) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfLoader;
    std::string err;
    std::string warn;
    UNUSED(mesh);
//    bool ret = gltfLoader.LoadBinaryFromFile(&gltfModel, &err, &warn, fileName);
    bool ret = gltfLoader.LoadASCIIFromFile(&gltfModel, &err, &warn, fileName);
    if (!warn.empty())
    {
        MRLOG("[tinygltf] Warning: " << warn);
    }
    if (!err.empty()) 
    {
        MRLOG("[tinygltf] Error: " << err);
    }
    if(!ret) 
    {
        MRLOG("[tinygltf] Failed to parse gltf");
        exit(1);
    }
    const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene != -1 ? gltfModel.defaultScene : 0];
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        get_node_properties(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount);
    }
    mesh.vertices.reserve(vertexCount);
    mesh.indices.reserve(indexCount);
    NodeLoadingData nodeLoadingData = {mesh.vertices, mesh.indices};

    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
        load_node(node, gltfModel, nodeLoadingData);
    }
    
    MRLOG(
        "Finished loading gltf: " 
        << fileName << " \n"
        << "Number of vertices: " << mesh.vertices.size() << " \n"
        << "Number of indices: " << mesh.indices.size() << " \n"
    );
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
POP_MSVC_WARNINGS