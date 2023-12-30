#include "Pipeline/MaterialFunctions.h"
#include "Common/Log.h"
#include "Scene/Scene.h"

void create_material(GraphicsPipeline graphicsPipeline, const std::string& materialName) {
    Scene::GetInstance().sceneMaterialMap[materialName] = graphicsPipeline;
}

[[nodiscard]] GraphicsPipeline* get_material(const std::string& materialName) {
    // Search for the material, and return nullptr if not found
    auto it = Scene::GetInstance().sceneMaterialMap.find(materialName);
    if (it == Scene::GetInstance().sceneMaterialMap.end()) {
        MRCERR("Could not find material: " << materialName);
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}
