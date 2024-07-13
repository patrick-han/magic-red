#include <Common/Compiler/DisableWarnings.h>
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4267) // conversion from 'size_t' to 'uint32_t', possible loss of data
DISABLE_MSVC_WARNING(4201) // nonstandard extension used : nameless struct / union (glm library)
PUSH_CLANG_WARNINGS
DISABLE_CLANG_WARNING("-Wmissing-field-initializers")
DISABLE_CLANG_WARNING("-Wshorten-64-to-32")
#include <Common/Compiler/Unused.h>

#include "Model.h"
#include <Texture/TextureCache.h>
#include <Material/MaterialCache.h>
#include <Material/Material.h>
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
            MRLOG("Mesh has " << nodeMesh.primitives.size() << " primtives");

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

            // Vertex UVs
            const tinygltf::Accessor &texcoordAccessor = model.accessors[nodeMeshPrimitive.attributes.find("TEXCOORD_0")->second];
            if (texcoordAccessor.type != TINYGLTF_TYPE_VEC2)
            {
                MRCERR("Texcoord accessor isn't VEC2 type!");
                exit(1);
            }
            const tinygltf::BufferView& texcoordBufferView = model.bufferViews[texcoordAccessor.bufferView];
            const tinygltf::Buffer& texcoordBuffer = model.buffers[texcoordBufferView.buffer];
            const float* texcoordBufferData = reinterpret_cast<const float*>(&(texcoordBuffer.data[texcoordAccessor.byteOffset + texcoordBufferView.byteOffset]));
            size_t texcoordByteStride = texcoordAccessor.ByteStride(texcoordBufferView) ? (texcoordAccessor.ByteStride(texcoordBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);

            // Create vertices
            for (size_t vertex_i = 0; vertex_i < vertexCount; vertex_i++)
            {
                glm::vec3 position = glm::make_vec3(&positionBufferData[vertex_i * positionByteStride]);
                glm::vec3 normal = glm::normalize(glm::make_vec3(&normalBufferData[vertex_i * normalByteStride]));
                glm::vec2 texcoord = glm::make_vec2(&texcoordBufferData[vertex_i * texcoordByteStride]);
                Vertex new_vertex = { position, texcoord.x, normal, texcoord.y, glm::vec4(0.0)}; // TODO
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

            // Material
            if (nodeMeshPrimitive.material != -1)
            {
                const tinygltf::Material &primMaterial = model.materials[nodeMeshPrimitive.material];

                // Diffuse
                const tinygltf::Texture &diffuseTexture = model.textures[primMaterial.pbrMetallicRoughness.baseColorTexture.index];
                const tinygltf::Image &diffuseImage = model.images[diffuseTexture.source];
                const tinygltf::Sampler &diffuseSampler = model.samplers[diffuseTexture.sampler];
                UNUSED(diffuseSampler);
                nodeLoadingData.diffuseTex.data = &diffuseImage.image.at(0);
                nodeLoadingData.diffuseTex.texSize.x = diffuseImage.width;
                nodeLoadingData.diffuseTex.texSize.y = diffuseImage.height;
                nodeLoadingData.diffuseTex.texSize.ch = diffuseImage.component;

                // Normal
                const tinygltf::Texture &normalTexture = model.textures[primMaterial.normalTexture.index];
                const tinygltf::Image &normalImage= model.images[normalTexture.source];
                const tinygltf::Sampler &normalSampler = model.samplers[normalTexture.sampler];
                UNUSED(normalSampler);
                nodeLoadingData.normalTex.data = &normalImage.image.at(0);
                nodeLoadingData.normalTex.texSize.x = normalImage.width;
                nodeLoadingData.normalTex.texSize.y = normalImage.height;
                nodeLoadingData.normalTex.texSize.ch = normalImage.component;

                // Metallic-Roughness
                const tinygltf::Texture &metRoughTexture = model.textures[primMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
                const tinygltf::Image &metRoughImage= model.images[metRoughTexture.source];
                const tinygltf::Sampler &metRoughSampler = model.samplers[metRoughTexture.sampler];
                UNUSED(metRoughSampler);
                nodeLoadingData.metRoughTex.data = &metRoughImage.image.at(0);
                nodeLoadingData.metRoughTex.texSize.x = metRoughImage.width;
                nodeLoadingData.metRoughTex.texSize.y = metRoughImage.height;
                nodeLoadingData.metRoughTex.texSize.ch = metRoughImage.component;

                // Emissive
                const tinygltf::Texture &emissiveTexture = model.textures[primMaterial.emissiveTexture.index];
                const tinygltf::Image &emissiveImage= model.images[emissiveTexture.source];
                const tinygltf::Sampler &emissiveSampler = model.samplers[emissiveTexture.sampler];
                UNUSED(emissiveSampler);
                nodeLoadingData.emissiveTex.data = &emissiveImage.image.at(0);
                nodeLoadingData.emissiveTex.texSize.x = emissiveImage.width;
                nodeLoadingData.emissiveTex.texSize.y = emissiveImage.height;
                nodeLoadingData.emissiveTex.texSize.ch = emissiveImage.component;

            }
            else
            {
                MRCERR("No material!");
            }
        }
    }
}

CPUModel::CPUModel(const char* fileName, bool isBinary, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice) : m_materialCache(_materialCache), m_textureCache(_textureCache), m_gfxDevice(_gfxDevice) {
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

    m_cpuMesh.m_vertices.resize(vertexCount);
    m_cpuMesh.m_indices.resize(indexCount);
    NodeLoadingData nodeLoadingData = {m_cpuMesh.m_vertices, m_cpuMesh.m_indices, 0, 0, {}, {}};

    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
        load_node(node, gltfModel, nodeLoadingData);
    }
    
    MRLOG(
        "Finished loading gltf: " 
        << fileName << " \n"
        << "Number of vertices: " << m_cpuMesh.m_vertices.size() << " \n"
        << "Number of indices: " << m_cpuMesh.m_indices.size() << " \n"
    );

    // Material/Textures
    Material modelMaterial;

    if (nodeLoadingData.diffuseTex.texSize.x != -1)
    {
        modelMaterial.diffuseImageId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.diffuseTex);
    }
    if (nodeLoadingData.normalTex.texSize.x != -1)
    {
        modelMaterial.normalImageId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.normalTex);
    }
    if (nodeLoadingData.metRoughTex.texSize.x != -1)
    {
        modelMaterial.metallicRoughnessImageId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.metRoughTex);
    }
    if (nodeLoadingData.emissiveTex.texSize.x != -1)
    {
        modelMaterial.emissiveImageId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.emissiveTex);
    }
    m_materialId = m_materialCache.add_material(modelMaterial);
}

POP_CLANG_WARNINGS
POP_MSVC_WARNINGS
