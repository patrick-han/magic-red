#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>
#include <Texture/TextureData.h>

class GfxDevice;
class TextureCache;
class MaterialCache;

struct NodeLoadingData {
    std::vector<Vertex>& vertices;
    std::vector<uint32_t>& indices;
    size_t indexPos{};
    size_t vertexPos{};
    // Each node has a single mesh?
    // TODO: Technically a mesh could have multiple materials (each "primitive" could have a different material)
    // but lets keep it simple for now
    TextureLoadingData diffuseTex;
    TextureLoadingData normalTex;
    TextureLoadingData metRoughTex;
    TextureLoadingData emissiveTex;
};

struct CPUModel {
    CPUModel(const char* fileName, bool isBinary, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice);
    CPUMesh m_cpuMesh;
    MaterialId m_materialId;
private:
    MaterialCache& m_materialCache;
    TextureCache& m_textureCache;
    const GfxDevice& m_gfxDevice;
    
};