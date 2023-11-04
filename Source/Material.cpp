#include "Material.h"
#include "Common/Log.h"

void create_material(GraphicsPipeline *graphicsPipeline, const std::string& materialName, std::unordered_map<std::string, Material>& materialMap) {
    Material mat = { graphicsPipeline };
    materialMap[materialName] = mat;
}

[[nodiscard]] Material* get_material(const std::string& materialName, std::unordered_map<std::string, Material>& materialMap) {
    // Search for the material, and return nullptr if not found
    auto it = materialMap.find(materialName);
    if (it == materialMap.end()) {
        MRCERR("Could not find material!");
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}
