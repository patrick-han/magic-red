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
#include <assimp/matrix4x4.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

unsigned char* CPUModel::load_texture_from_filename(aiString& str, int* width, int* height, int* numberComponents)
{
    const char* textureFileName = str.C_Str();
    std::string texturePath = m_path.parent_path().append(textureFileName).string();
    const char* texturePath_cstr = texturePath.c_str();
    unsigned char *data = stbi_load(texturePath_cstr, width, height, numberComponents, STBI_rgb_alpha); // TODO: request 4 channels from all images
    return data;
}

void CPUModel::load_embedded_texture_data(const aiMaterial* material, const aiScene* scene, aiTextureType textureType, Material& meshMaterial)
{
    aiString embeddedTextureFile;
    material->GetTexture(textureType, 0, &embeddedTextureFile);
    const aiTexture* texture = scene->GetEmbeddedTexture(embeddedTextureFile.C_Str());
    int width, height, numberComponents;
    std::string textureName = m_path.filename().string();

    GPUTextureId* meshMaterialTextureIdToSet = nullptr;

    switch(textureType)
    {
        case aiTextureType_BASE_COLOR:
            textureName += std::string("_diffuse_tex");
            meshMaterialTextureIdToSet = &meshMaterial.diffuseTextureId;
            break;
        case aiTextureType_METALNESS:
            textureName += std::string("_metallic_roughness_tex");
            meshMaterialTextureIdToSet = &meshMaterial.metallicRoughnessTextureId;
            break;
        case aiTextureType_NORMALS:
            textureName += std::string("_normals_tex");
            meshMaterialTextureIdToSet = &meshMaterial.normalTextureId;
            break;
        case aiTextureType_EMISSIVE:
            textureName += std::string("_emissive_tex");
            meshMaterialTextureIdToSet = &meshMaterial.emissiveTextureId;
            break;
        default:
            MRCERR("Tried to load non-standard aiTextureType!");
            exit(1);
    }
    

    if (!m_textureCache.is_texture_loaded_already(textureName))
    {
        stbi_uc* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), texture->mWidth, &width, &height, &numberComponents, STBI_rgb_alpha);
        if (!data)
        {
            MRCERR("Failed to load embedded texture from memory!");
            exit(1);
        }

        TextureLoadingData textureLoadingData = {
            .data = data,
            .texSize = {width, height, 4} // TODO: force all images to have 4 channels...ignoring numberComponents for now
        };
        *meshMaterialTextureIdToSet = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, textureName);
        stbi_image_free(textureLoadingData.data);
    }
    else
    {
        *meshMaterialTextureIdToSet = m_textureCache.get_texture_id(textureName);
    }
}

CPUMesh CPUModel::process_mesh(aiMesh *mesh, const aiScene *scene, const glm::mat4x4& transformMatrix)
{
    CPUMesh cpuMesh;
    cpuMesh.m_transform = transformMatrix;
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

        unsigned int materialDiffuseCount = material->GetTextureCount(aiTextureType_BASE_COLOR);
        unsigned int materialMetallicRoughnessCount = material->GetTextureCount(aiTextureType_METALNESS);
        unsigned int materialNormalCount = material->GetTextureCount(aiTextureType_NORMALS);
        unsigned int materialEmissiveCount = material->GetTextureCount(aiTextureType_EMISSIVE);

        if (m_texturesEmbedded) // .glb for example
        {
            
            // const bool isCompressed = texture->mHeight == 0 ? true : false;
            // if (isCompressed)
            if (materialDiffuseCount > 0)
            {
                load_embedded_texture_data(material, scene, aiTextureType_BASE_COLOR, meshMaterial);   
            }
            else
            {
                meshMaterial.diffuseTextureId = m_textureCache.get_texture_id(missingDiffuseTextureName);
            }
            // else
            // {
            //     MRCERR("Uncompressed texture, what do?");
            //     exit(1);
            // }

            if (materialMetallicRoughnessCount > 0)
            {
                load_embedded_texture_data(material, scene, aiTextureType_METALNESS, meshMaterial);
            }
            else
            {
                meshMaterial.metallicRoughnessTextureId = m_textureCache.get_texture_id(default1TextureName);
            }

            if (materialNormalCount > 0)
            {
                load_embedded_texture_data(material, scene, aiTextureType_NORMALS, meshMaterial);
            }
            else
            {
                meshMaterial.normalTextureId = m_textureCache.get_texture_id(default1TextureName);
            }

            if (materialEmissiveCount > 0)
            {
                load_embedded_texture_data(material, scene, aiTextureType_EMISSIVE, meshMaterial);
            }
            else
            {
                meshMaterial.emissiveTextureId = m_textureCache.get_texture_id(default1TextureName);
            }


        }
        else
        {
            // TODO: When can a material have multiple textures of type diffuse?

            // TODO: Right now this CPUModel class is directly uploading the textures as it parses the assimp data structure, which doesn't
            // necessarily follow the spirit of the class name.
            // Better possibly would be to store the texture data and queue uploading jobs after the fact, along with uploading the mesh data.

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
                const std::string textureName(str.C_Str());
                
                if (!m_textureCache.is_texture_loaded_already(textureName))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        //.texSize = {width, height, numberComponents}
                        .texSize = {width, height, 4} // TODO: force all images to have 4 channels...ignoring numberComponents for now
                    };
                    meshMaterial.diffuseTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, textureName);
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.diffuseTextureId = m_textureCache.get_texture_id(textureName);
                }
            }
            else
            {
                meshMaterial.diffuseTextureId = m_textureCache.get_texture_id(missingDiffuseTextureName);
            }
            if (materialMetallicRoughnessCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &str);
                const std::string textureName(str.C_Str());
                //material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &str); // These seem to return the same thing for conformant gltf models

                if (!m_textureCache.is_texture_loaded_already(textureName))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.metallicRoughnessTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, textureName);
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.metallicRoughnessTextureId = m_textureCache.get_texture_id(textureName);
                }
            }
            else
            {
                meshMaterial.metallicRoughnessTextureId = m_textureCache.get_texture_id(default1TextureName);
            }
            if (materialNormalCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(aiTextureType_NORMALS, 0, &str);
                const std::string textureName(str.C_Str());
                if (!m_textureCache.is_texture_loaded_already(textureName))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.normalTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, textureName);
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.normalTextureId = m_textureCache.get_texture_id(textureName);
                }
            }
            else
            {
                meshMaterial.normalTextureId = m_textureCache.get_texture_id(default1TextureName);
            }
            if (materialEmissiveCount > 0)
            {
                aiString str;
                int width, height, numberComponents;
                material->GetTexture(aiTextureType_EMISSIVE, 0, &str);
                const std::string textureName(str.C_Str());
                if (!m_textureCache.is_texture_loaded_already(textureName))
                {
                    unsigned char* data = load_texture_from_filename(str, &width, &height, &numberComponents);
                    TextureLoadingData textureLoadingData = {
                        .data = data,
                        .texSize = {width, height, 4}
                    };
                    meshMaterial.emissiveTextureId = m_textureCache.add_texture(m_gfxDevice, textureLoadingData, textureName);
                    stbi_image_free(data);
                }
                else
                {
                    meshMaterial.emissiveTextureId = m_textureCache.get_texture_id(textureName);
                }
            }
            else
            {
                meshMaterial.emissiveTextureId = m_textureCache.get_texture_id(default1TextureName);
            }
        }
        cpuMesh.m_materialId = m_materialCache.add_material(meshMaterial);
    }
    return cpuMesh;
}

glm::mat4x4 convertAssimpMatrix(const aiMatrix4x4 &aiMat)
{
    return {
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    };
}

void CPUModel::process_assimp_node(aiNode *node, const aiScene *scene, const glm::mat4x4& accumulateMatrix)
{
    glm::mat4x4 transform = accumulateMatrix * convertAssimpMatrix(node->mTransformation);
    // Process this node's meshes
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_cpuMeshes.push_back(process_mesh(mesh, scene, transform));
    }
    // Process this node's child node(s)
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        process_assimp_node(node->mChildren[i], scene, transform);
    }
}

CPUModel::CPUModel(const char* _filePath, bool _texturesEmbedded, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice) : m_materialCache(_materialCache), m_textureCache(_textureCache), m_gfxDevice(_gfxDevice), m_texturesEmbedded(_texturesEmbedded), m_filePath(_filePath), m_path(std::string(m_filePath)){

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(m_filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        MRCERR("Problem loading model: " << m_filePath);
    }
    glm::mat4x4 rootTransform = convertAssimpMatrix(scene->mRootNode->mTransformation);
    process_assimp_node(scene->mRootNode, scene, rootTransform);
}

POP_CLANG_WARNINGS
POP_MSVC_WARNINGS
