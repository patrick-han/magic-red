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
//#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
//#include <External/tinygltf/tiny_gltf.h>
#include <External/tinygltf/stb_image.h>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//void get_node_properties(const tinygltf::Node& node, const tinygltf::Model& model, uint32_t& vertexCount, uint32_t& indexCount) {
//    if (node.children.size() > 0)
//    {
//        for (size_t i = 0; i < node.children.size(); i++)
//        {
//            tinygltf::Node childNode = model.nodes[node.children[i]];
//            get_node_properties(childNode, model, vertexCount, indexCount);
//        }
//    }
//    if (node.mesh != -1) // -1 used to indicate validity in the tingltf library
//    {
//        tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
//        for (size_t i = 0; i < nodeMesh.primitives.size(); i++)
//        {
//            tinygltf::Primitive nodeMeshPrimitive = nodeMesh.primitives[i];
//            int primitivePositionAccessorIndex = nodeMeshPrimitive.attributes.find("POSITION")->second;
//            vertexCount += model.accessors[primitivePositionAccessorIndex].count;
//            if (nodeMeshPrimitive.indices != -1) // Meshes may not be indexed drawn
//            {
//                indexCount += model.accessors[nodeMeshPrimitive.indices].count;
//            }
//            
//        }
//    }
//}

//void load_node(const tinygltf::Node& node, const tinygltf::Model& model, NodeLoadingData& nodeLoadingData) {
//
//
//    if (node.children.size() >  0)
//    {
//        load_node(node, model, nodeLoadingData);
//    }
//    if (node.mesh != -1)
//    {
//        tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
//         for (size_t j = 0; j < nodeMesh.primitives.size(); j++)
//        {
//            MRLOG("Mesh has " << nodeMesh.primitives.size() << " primtives");
//
//            tinygltf::Primitive nodeMeshPrimitive = nodeMesh.primitives[j];
//            uint32_t vertexStart = static_cast<uint32_t>(nodeLoadingData.vertexPos);
//			uint32_t indexStart = static_cast<uint32_t>(nodeLoadingData.indexPos);
//            UNUSED(indexStart);
//
//            // Vertex Positions
//            const tinygltf::Accessor& positionAccessor = model.accessors[nodeMeshPrimitive.attributes.find("POSITION")->second];
//            size_t vertexCount = positionAccessor.count;
//            if (positionAccessor.type != TINYGLTF_TYPE_VEC3)
//            {
//                MRCERR("Position accessor isn't VEC3 type!");
//                exit(1);
//            }
//            const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
//            const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
//            const float* positionBufferData = reinterpret_cast<const float*>(&(positionBuffer.data[positionAccessor.byteOffset + positionBufferView.byteOffset]));
//            size_t positionByteStride = positionAccessor.ByteStride(positionBufferView) ? (positionAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
//
//
//            // Vertex Normals
//            const tinygltf::Accessor &normalAccessor = model.accessors[nodeMeshPrimitive.attributes.find("NORMAL")->second];
//            if (normalAccessor.type != TINYGLTF_TYPE_VEC3)
//            {
//                MRCERR("Normal accessor isn't VEC3 type!");
//                exit(1);
//            }
//            const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
//            const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
//            const float* normalBufferData = reinterpret_cast<const float*>(&(normalBuffer.data[normalAccessor.byteOffset + normalBufferView.byteOffset]));
//            size_t normalByteStride = normalAccessor.ByteStride(normalBufferView) ? (normalAccessor.ByteStride(normalBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
//
//            // Vertex UVs
//            const tinygltf::Accessor &texcoordAccessor = model.accessors[nodeMeshPrimitive.attributes.find("TEXCOORD_0")->second];
//            if (texcoordAccessor.type != TINYGLTF_TYPE_VEC2)
//            {
//                MRCERR("Texcoord accessor isn't VEC2 type!");
//                exit(1);
//            }
//            const tinygltf::BufferView& texcoordBufferView = model.bufferViews[texcoordAccessor.bufferView];
//            const tinygltf::Buffer& texcoordBuffer = model.buffers[texcoordBufferView.buffer];
//            const float* texcoordBufferData = reinterpret_cast<const float*>(&(texcoordBuffer.data[texcoordAccessor.byteOffset + texcoordBufferView.byteOffset]));
//            size_t texcoordByteStride = texcoordAccessor.ByteStride(texcoordBufferView) ? (texcoordAccessor.ByteStride(texcoordBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
//
//            // Create vertices
//            for (size_t vertex_i = 0; vertex_i < vertexCount; vertex_i++)
//            {
//                glm::vec3 position = glm::make_vec3(&positionBufferData[vertex_i * positionByteStride]);
//                glm::vec3 normal = glm::normalize(glm::make_vec3(&normalBufferData[vertex_i * normalByteStride]));
//                glm::vec2 texcoord = glm::make_vec2(&texcoordBufferData[vertex_i * texcoordByteStride]);
//                Vertex new_vertex = { position, texcoord.x, normal, texcoord.y, glm::vec4(0.0)}; // TODO
//                nodeLoadingData.vertices[nodeLoadingData.vertexPos] = new_vertex;
//                nodeLoadingData.vertexPos++;
//            }
//
//            // Indices
//            const tinygltf::Accessor &indicesAccessor = model.accessors[nodeMeshPrimitive.indices];
//            size_t indexCount = indicesAccessor.count;
//            if (indicesAccessor.type != TINYGLTF_TYPE_SCALAR)
//            {
//                MRCERR("Index accessor isn't SCALAR type!");
//                exit(1);
//            }
//            const tinygltf::BufferView &indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
//            const tinygltf::Buffer &indicesBuffer = model.buffers[indicesBufferView.buffer];
//            const void* indexBufferDataPtr = &(indicesBuffer.data[indicesAccessor.byteOffset + indicesBufferView.byteOffset]);
//
//            switch (indicesAccessor.componentType)
//            {
//            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
//            {
//                const uint32_t* indicesBufferData = static_cast<const uint32_t*>(indexBufferDataPtr);
//                for (size_t index_i = 0; index_i < indexCount; index_i++)
//                {
//                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
//                    nodeLoadingData.indexPos++;
//                }
//                break;
//            }
//            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
//            {
//                const uint16_t* indicesBufferData = static_cast<const uint16_t*>(indexBufferDataPtr);
//                for (size_t index_i = 0; index_i < indexCount; index_i++)
//                {
//                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
//                    nodeLoadingData.indexPos++;
//                }
//                break;
//            }
//            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
//            {
//                const uint8_t* indicesBufferData = static_cast<const uint8_t*>(indexBufferDataPtr);
//                for (size_t index_i = 0; index_i < indexCount; index_i++)
//                {
//                    nodeLoadingData.indices[nodeLoadingData.indexPos] = indicesBufferData[index_i] + vertexStart;
//                    nodeLoadingData.indexPos++;
//                }
//                break;
//            }
//            default:
//                MRCERR("Unexpected index accessor component type, perhaps the gltf is malformed!");
//                exit(1);
//                break;
//            }
//
//            // Material
//            if (nodeMeshPrimitive.material != -1)
//            {
//                const tinygltf::Material &primMaterial = model.materials[nodeMeshPrimitive.material];
//
//                CPUMaterial materialData;
//
//                if (primMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
//                {
//                    // Diffuse
//                    const tinygltf::Texture& diffuseTexture = model.textures[primMaterial.pbrMetallicRoughness.baseColorTexture.index];
//                    const tinygltf::Image& diffuseImage = model.images[diffuseTexture.source];
//                    const tinygltf::Sampler& diffuseSampler = model.samplers[diffuseTexture.sampler];
//                    UNUSED(diffuseSampler);
//                    materialData.diffuseTex.data = &diffuseImage.image.at(0);
//                    materialData.diffuseTex.texSize.x = diffuseImage.width;
//                    materialData.diffuseTex.texSize.y = diffuseImage.height;
//                    materialData.diffuseTex.texSize.ch = diffuseImage.component;
//                }
//
//                if (primMaterial.normalTexture.index != -1)
//                {
//                    // Normal
//                    const tinygltf::Texture& normalTexture = model.textures[primMaterial.normalTexture.index];
//                    const tinygltf::Image& normalImage = model.images[normalTexture.source];
//                    const tinygltf::Sampler& normalSampler = model.samplers[normalTexture.sampler];
//                    UNUSED(normalSampler);
//                    materialData.normalTex.data = &normalImage.image.at(0);
//                    materialData.normalTex.texSize.x = normalImage.width;
//                    materialData.normalTex.texSize.y = normalImage.height;
//                    materialData.normalTex.texSize.ch = normalImage.component;
//                }
//
//                if (primMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
//                {
//                    // Metallic-Roughness
//                    const tinygltf::Texture& metRoughTexture = model.textures[primMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
//                    const tinygltf::Image& metRoughImage = model.images[metRoughTexture.source];
//                    const tinygltf::Sampler& metRoughSampler = model.samplers[metRoughTexture.sampler];
//                    UNUSED(metRoughSampler);
//                    materialData.metRoughTex.data = &metRoughImage.image.at(0);
//                    materialData.metRoughTex.texSize.x = metRoughImage.width;
//                    materialData.metRoughTex.texSize.y = metRoughImage.height;
//                    materialData.metRoughTex.texSize.ch = metRoughImage.component;
//                }
//
//
//                if (primMaterial.emissiveTexture.index != -1)
//                {
//                    // Emissive
//                    const tinygltf::Texture& emissiveTexture = model.textures[primMaterial.emissiveTexture.index];
//                    const tinygltf::Image& emissiveImage = model.images[emissiveTexture.source];
//                    const tinygltf::Sampler& emissiveSampler = model.samplers[emissiveTexture.sampler];
//                    UNUSED(emissiveSampler);
//                    materialData.emissiveTex.data = &emissiveImage.image.at(0);
//                    materialData.emissiveTex.texSize.x = emissiveImage.width;
//                    materialData.emissiveTex.texSize.y = emissiveImage.height;
//                    materialData.emissiveTex.texSize.ch = emissiveImage.component;
//                }
//
//            }
//            else
//            {
//                MRCERR("No material!");
//            }
//        }
//    }
//}

// CPUModel::CPUModel(const char* fileName, bool isBinary, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice) : m_materialCache(_materialCache), m_textureCache(_textureCache), m_gfxDevice(_gfxDevice) {
//     tinygltf::Model gltfModel;
//     tinygltf::TinyGLTF gltfLoader;
//     std::string err;
//     std::string warn;

//     bool ret;
//     if (isBinary)
//     {
//         ret = gltfLoader.LoadBinaryFromFile(&gltfModel, &err, &warn, fileName);
//     }
//     else
//     {
//         ret = gltfLoader.LoadASCIIFromFile(&gltfModel, &err, &warn, fileName);
//     }

//     if (!warn.empty())
//     {
//         MRLOG("[tinygltf] Warning: " << warn);
//     }
//     if (!err.empty()) 
//     {
//         MRLOG("[tinygltf] Error: " << err);
//     }
//     if(!ret) 
//     {
//         MRLOG("[tinygltf] Failed to parse gltf");
//         exit(1);
//     }
//     const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene != -1 ? gltfModel.defaultScene : 0];
//     uint32_t vertexCount = 0;
//     uint32_t indexCount = 0;
//     for (size_t i = 0; i < scene.nodes.size(); i++)
//     {
//         get_node_properties(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount);
//     }

//     m_cpuMesh.m_vertices.resize(vertexCount);
//     m_cpuMesh.m_indices.resize(indexCount);
//     NodeLoadingData nodeLoadingData = {m_cpuMesh.m_vertices, m_cpuMesh.m_indices, 0, 0, {}};

//     for (size_t i = 0; i < scene.nodes.size(); i++)
//     {
//         const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
//         load_node(node, gltfModel, nodeLoadingData);
//     }
    
//     MRLOG(
//         "Finished loading gltf: " 
//         << fileName << " \n"
//         << "Number of vertices: " << m_cpuMesh.m_vertices.size() << " \n"
//         << "Number of indices: " << m_cpuMesh.m_indices.size() << " \n"
//     );

//     // Material/Textures
//     Material modelMaterial;

//     //if (nodeLoadingData.materialData.diffuseTex.texSize.x != -1)
//     //{
//     //    modelMaterial.diffuseTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.diffuseTex);
//     //}
//     //if (nodeLoadingData.materialData.normalTex.texSize.x != -1)
//     //{
//     //    modelMaterial.normalTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.normalTex);
//     //}
//     //if (nodeLoadingData.materialData.metRoughTex.texSize.x != -1)
//     //{
//     //    modelMaterial.metallicRoughnessTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.metRoughTex);
//     //}
//     //if (nodeLoadingData.materialData.emissiveTex.texSize.x != -1)
//     //{
//     //    modelMaterial.emissiveTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.emissiveTex);
//     //}
//     //m_materialId = m_materialCache.add_material(modelMaterial);
// }

unsigned char* CPUModel::load_texture_from_filename(aiString& str, int* width, int* height, int* numberComponents)
{
    const char* textureFileName = str.C_Str();
    std::string texturePath = m_path.parent_path().append(textureFileName).string();
    const char* texturePath_cstr = texturePath.c_str();
    unsigned char *data = stbi_load(texturePath_cstr, width, height, numberComponents, 4); // TODO: request 4 channels from all images
    return data;
}

CPUMesh CPUModel::process_mesh(aiMesh *mesh, const aiScene *scene)
{
    CPUMesh cpuMesh;
    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.uv_x = mesh->mTextureCoords[0][i].x;
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.uv_y = mesh->mTextureCoords[0][i].y;

        // TODO:
        // vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        cpuMesh.m_vertices.emplace_back(vertex);
    }
    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
        {
            cpuMesh.m_indices.push_back(face.mIndices[j]);
        } 
    }
    if(mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        Material meshMaterial;

        if (m_texturesEmbedded) // .glb for example
        {

        }
        else
        {
            // TODO: When can a material have multiple textures of type diffuse?

            // TODO: Right now this CPUModel class is directly uploading the textures as it parses the assimp data structure, which doesn't
            // necessarily follow the spirit of the class name.
            // Better possibly would be to store the texture data and queue uploading jobs after the fact, along with uploading the mesh data.
            unsigned int materialDiffuseCount = material->GetTextureCount(aiTextureType_BASE_COLOR);
            unsigned int materialMetallicRoughnessCount = material->GetTextureCount(aiTextureType_METALNESS);
            unsigned int materialNormalCount = material->GetTextureCount(aiTextureType_NORMALS);
            unsigned int materialEmissiveCount = material->GetTextureCount(aiTextureType_EMISSIVE);
            // MRLOG("Diffuse count: " << materialDiffuseCount);
            // MRLOG("Metalness count: " << materialMetallicRoughnessCount);
            // MRLOG("Normal count: " << materialNormalCount);
            // MRLOG("Emissive count: " << materialEmissiveCount);

            

            // From the glTF 2.0 spec: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material

            // "The value for each property MAY be defined using factors and/or textures (e.g., baseColorTexture and baseColorFactor). If a texture is not given, all respective texture components within this material model MUST be assumed to have a value of 1.0. If both factors and textures are present, the factor value acts as a linear multiplier for the corresponding texture values."

            // {
            // "materials": [
            //     {
            //         "name": "Material0",
            //         "pbrMetallicRoughness": {
            //             "baseColorFactor": [ 0.5, 0.5, 0.5, 1.0 ],
            //             "baseColorTexture": {
            //                 "index": 1,
            //                 "texCoord": 1
            //             },
            //             "metallicFactor": 1,
            //             "roughnessFactor": 1,
            //             "metallicRoughnessTexture": {
            //                 "index": 2,
            //                 "texCoord": 1
            //             }
            //         },
            //         "normalTexture": {
            //             "scale": 2,
            //             "index": 3,
            //             "texCoord": 1
            //         },
            //         "emissiveFactor": [ 0.2, 0.1, 0.0 ]
            //     }
            // ]
            // }

            // glm::vec4 baseColorFactor;
            // float metallicFactor;
            // float roughnessFactor;
            // float normalScale;
            // glm::vec3 emissiveFactor;

            if (materialDiffuseCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &str);
                
                if (!m_textureCache.is_texture_loaded_already(str.C_Str()))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        //.texSize = {width, height, numberComponents}
                        .texSize = {width, height, 4} // TODO: force all images to have 4 channels...ignoring numberComponents for now
                    };
                    meshMaterial.diffuseTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, str.C_Str());
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.diffuseTextureId = m_textureCache.get_texture_id(str.C_Str());
                }
            }
            else
            {
                meshMaterial.diffuseTextureId = m_textureCache.get_texture_id("default_1_texture.png");
            }
            if (materialMetallicRoughnessCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &str);
                //material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &str); // These seem to return the same thing for conformant gltf models

                if (!m_textureCache.is_texture_loaded_already(str.C_Str()))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.metallicRoughnessTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, str.C_Str());
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.metallicRoughnessTextureId = m_textureCache.get_texture_id(str.C_Str());
                }
            }
            else
            {
                meshMaterial.metallicRoughnessTextureId = m_textureCache.get_texture_id("default_1_texture.png");
            }
            if (materialNormalCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(aiTextureType_NORMALS, 0, &str);

                if (!m_textureCache.is_texture_loaded_already(str.C_Str()))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.normalTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, str.C_Str());
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.normalTextureId = m_textureCache.get_texture_id(str.C_Str());
                }
            }
            else
            {
                meshMaterial.normalTextureId = m_textureCache.get_texture_id("default_1_texture.png");
            }
            if (materialEmissiveCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(aiTextureType_EMISSIVE, 0, &str);

                if (!m_textureCache.is_texture_loaded_already(str.C_Str()))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.emissiveTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, str.C_Str());
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.emissiveTextureId = m_textureCache.get_texture_id(str.C_Str());
                }
            }
            else
            {
                meshMaterial.emissiveTextureId = m_textureCache.get_texture_id("default_1_texture.png");
            }
        }
        cpuMesh.m_materialId = m_materialCache.add_material(meshMaterial);
    }
    return cpuMesh;
}

void CPUModel::process_assimp_node(aiNode *node, const aiScene *scene)
{
    // Process this node's meshes
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_cpuMeshes.push_back(process_mesh(mesh, scene));
    }
    // Process this node's child node(s)
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        process_assimp_node(node->mChildren[i], scene);
    }
}

CPUModel::CPUModel(const char* _filePath, bool _texturesEmbedded, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice) : m_materialCache(_materialCache), m_textureCache(_textureCache), m_gfxDevice(_gfxDevice), m_texturesEmbedded(_texturesEmbedded), m_filePath(_filePath), m_path(std::string(m_filePath)){

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(m_filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        MRCERR("Problem loading model: " << m_filePath);
    }

    process_assimp_node(scene->mRootNode, scene);


    // NodeLoadingData nodeLoadingData = {m_cpuMesh.m_vertices, m_cpuMesh.m_indices, 0, 0, {}};

    // Material/Textures
    // Material modelMaterial;

    //if (nodeLoadingData.materialData.diffuseTex.texSize.x != -1)
    //{
    //    modelMaterial.diffuseTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.diffuseTex);
    //}
    //if (nodeLoadingData.materialData.normalTex.texSize.x != -1)
    //{
    //    modelMaterial.normalTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.normalTex);
    //}
    //if (nodeLoadingData.materialData.metRoughTex.texSize.x != -1)
    //{
    //    modelMaterial.metallicRoughnessTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.metRoughTex);
    //}
    //if (nodeLoadingData.materialData.emissiveTex.texSize.x != -1)
    //{
    //    modelMaterial.emissiveTextureId = m_textureCache.add_texture(m_gfxDevice, nodeLoadingData.materialData.emissiveTex);
    //}
    //m_materialId = m_materialCache.add_material(modelMaterial);
}

POP_CLANG_WARNINGS
POP_MSVC_WARNINGS
