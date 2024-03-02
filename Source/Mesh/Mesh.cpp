#include <Common/Compiler/DisableWarnings.h>
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4267) // conversion from 'size_t' to 'uint32_t', possible loss of data
DISABLE_MSVC_WARNING(4201) // nonstandard extension used : nameless struct / union (glm library)
#include <Common/Compiler/Unused.h>

#include <Mesh/Mesh.h>
#include <vulkan/vulkan.h>
#include <Common/Log.h>
#include <span>

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
    size_t indexPos = 0;
    size_t vertexPos = 0;
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
            uint32_t vertexStart = static_cast<uint32_t>(nodeLoadingData.vertexPos);
			uint32_t indexStart = static_cast<uint32_t>(nodeLoadingData.indexPos);
            UNUSED(indexStart);

            // Vertex Positions
            const tinygltf::Accessor& positionAccessor = model.accessors[nodeMeshPrimitive.attributes.find("POSITION")->second];
            size_t vertexCount = positionAccessor.count;
            if (positionAccessor.type != TINYGLTF_TYPE_VEC3)
            {
                MRCERR("Position accessor isn't VEC3 type!");
                exit(1);
            }
            const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
            const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
            const float* positionBufferData = reinterpret_cast<const float*>(&(positionBuffer.data[positionAccessor.byteOffset + positionBufferView.byteOffset]));
            size_t positionByteStride = positionAccessor.ByteStride(positionBufferView) ? (positionAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);


            // Vertex Normals
            const tinygltf::Accessor &normalAccessor = model.accessors[nodeMeshPrimitive.attributes.find("NORMAL")->second];
            if (normalAccessor.type != TINYGLTF_TYPE_VEC3)
            {
                MRCERR("Normal accessor isn't VEC3 type!");
                exit(1);
            }
            const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
            const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
            const float* normalBufferData = reinterpret_cast<const float*>(&(normalBuffer.data[normalAccessor.byteOffset + normalBufferView.byteOffset]));
            size_t normalByteStride = normalAccessor.ByteStride(normalBufferView) ? (normalAccessor.ByteStride(normalBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

            // Create vertices
            for (size_t vertex_i = 0; vertex_i < vertexCount; vertex_i++)
            {
                glm::vec3 position = glm::make_vec3(&positionBufferData[vertex_i * positionByteStride]);
                glm::vec3 normal = glm::normalize(glm::make_vec3(&normalBufferData[vertex_i * normalByteStride]));
                Vertex new_vertex = { position, normal, glm::vec3(0.0, 0.8, 1.0)}; // Green default
                nodeLoadingData.vertices[nodeLoadingData.vertexPos] = new_vertex;
                nodeLoadingData.vertexPos++;
            }

            // Indices
            const tinygltf::Accessor &indicesAccessor = model.accessors[nodeMeshPrimitive.indices];
            size_t indexCount = indicesAccessor.count;
            if (indicesAccessor.type != TINYGLTF_TYPE_SCALAR)
            {
                MRCERR("Index accessor isn't SCALAR type!");
                exit(1);
            }
            const tinygltf::BufferView &indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
            const tinygltf::Buffer &indicesBuffer = model.buffers[indicesBufferView.buffer];
            const void* indexBufferDataPtr = &(indicesBuffer.data[indicesAccessor.byteOffset + indicesBufferView.byteOffset]);

            switch (indicesAccessor.componentType)
            {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                const uint32_t* indicesBufferData = static_cast<const uint32_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
                    nodeLoadingData.indexPos++;
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                const uint16_t* indicesBufferData = static_cast<const uint16_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
                    nodeLoadingData.indexPos++;
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                const uint8_t* indicesBufferData = static_cast<const uint8_t*>(indexBufferDataPtr);
                for (size_t index_i = 0; index_i < indexCount; index_i++)
                {
                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
                    nodeLoadingData.indexPos++;
                }
                break;
            }
            default:
                MRCERR("Unexpected index accessor component type, perhaps the gltf is malformed!");
                exit(1);
                break;
            }
        }
    }
}

void load_mesh_from_gltf(Mesh& mesh, const char* fileName, bool isBinary) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfLoader;
    std::string err;
    std::string warn;

    bool ret;
    if (isBinary)
    {
        ret = gltfLoader.LoadBinaryFromFile(&gltfModel, &err, &warn, fileName);
    }
    else
    {
        ret = gltfLoader.LoadASCIIFromFile(&gltfModel, &err, &warn, fileName);
    }

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

    mesh.vertices.resize(vertexCount);
    mesh.indices.resize(indexCount);
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