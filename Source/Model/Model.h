#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>
#include <Texture/TextureData.h>
#include <Material/Material.h>
#include <filesystem>

class GfxDevice;
class TextureCache;
class MaterialCache;
struct aiMesh;
struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiString;
enum aiTextureType : int;

#include <glm/mat4x4.hpp>

struct CPUModel {
    CPUModel(const char* _filePath, bool _texturesEmbedded, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice);

    std::vector<CPUMesh> m_cpuMeshes;
private:
    MaterialCache& m_materialCache;
    TextureCache& m_textureCache; 
    const GfxDevice& m_gfxDevice;
    bool m_texturesEmbedded;
    const char* m_filePath;
    const std::filesystem::path m_path;

    inline static const std::string missingDiffuseTextureName{"missing_diffuse_texture.png"};
    inline static const std::string default1TextureName{"default_1_texture.png"};
    

    unsigned char* load_texture_from_filename(aiString& str, int* width, int* height, int* numberComponents);
    void load_embedded_texture_data(const aiMaterial* material, const aiScene* scene, aiTextureType textureType, Material& meshMaterial);
    CPUMesh process_mesh(aiMesh *mesh, const aiScene *scene, const glm::mat4x4& transformMatrix);
    void process_assimp_node(aiNode *node, const aiScene *scene, const glm::mat4x4& accumulateMatrix);
    
};