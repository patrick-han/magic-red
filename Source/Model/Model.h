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
struct aiString;

// struct NodeLoadingData {
//     std::vector<Vertex>& vertices;
//     std::vector<uint32_t>& indices;
//     size_t indexPos{};
//     size_t vertexPos{};
//     // Each node has a single mesh?
//     // TODO: Technically a mesh could have multiple materials (each "primitive" could have a different material)
//     // but lets keep it simple for now
//     std::vector<CPUMaterial> perPrimitiveMaterialData;
// };

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
    

    unsigned char* load_texture_from_filename(aiString& str, int* width, int* height, int* numberComponents);
    CPUMesh process_mesh(aiMesh *mesh, const aiScene *scene);
    void process_assimp_node(aiNode *node, const aiScene *scene);
    
};