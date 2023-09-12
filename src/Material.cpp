#include "Material.h"


Material* create_material(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::string& materialName, std::unordered_map<std::string, Material>& materialMap) {
    Material mat = { pipeline, pipelineLayout };
    materialMap[materialName] = mat;
    return &materialMap[materialName];
}

Material* get_material(const std::string& materialName, std::unordered_map<std::string, Material>& materialMap) {
    // Search for the material, and return nullptr if not found
    auto it = materialMap.find(materialName);
    if (it == materialMap.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}
