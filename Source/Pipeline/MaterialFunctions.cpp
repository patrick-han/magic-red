#include <Pipeline/MaterialFunctions.h>
#include <Common/Log.h>
#include <Scene/Scene.h>

void create_material(std::unordered_map<std::string, GraphicsPipeline> &sceneMaterialMap, GraphicsPipeline graphicsPipeline, const std::string& materialName) {
    sceneMaterialMap[materialName] = graphicsPipeline;
}

[[nodiscard]] GraphicsPipeline* get_material(const std::string& materialName, std::unordered_map<std::string, GraphicsPipeline>& materialMap) {
    // Search for the material, and return nullptr if not found
    auto it = materialMap.find(materialName);
    if (it == materialMap.end()) {
        MRCERR("Could not find material: " << materialName);
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}
