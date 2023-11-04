#pragma once
#include "vulkan/vulkan.h"
#include <string>
#include <unordered_map>

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

/* Add a material to a scene material map */
void create_material(VkPipeline pipeline, VkPipelineLayout pipelineLayout, const std::string& materialName, std::unordered_map<std::string, Material>& materialMap);

/* Get a material from a scene material map */
[[nodiscard]] Material* get_material(const std::string& materialName, std::unordered_map<std::string, Material>& materialMap);
