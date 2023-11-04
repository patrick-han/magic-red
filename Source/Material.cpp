#include "Material.h"
#include "Common/Log.h"
#include "Scene/Scene.h"

void create_material(GraphicsPipeline *graphicsPipeline, const std::string& materialName) {
    Material mat = { graphicsPipeline };
    Scene::GetInstance().sceneMaterialMap[materialName] = mat;
}

[[nodiscard]] Material* get_material(const std::string& materialName) {
    // Search for the material, and return nullptr if not found
    auto it = Scene::GetInstance().sceneMaterialMap.find(materialName);
    if (it == Scene::GetInstance().sceneMaterialMap.end()) {
        MRCERR("Could not find material!");
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}
