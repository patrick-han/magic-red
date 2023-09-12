#pragma once
#include "vulkan/vulkan.hpp"
#include <unordered_map>

struct Material {
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
};

/* Add a material to a scene material map */
Material* create_material(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, const std::string& materialName, std::unordered_map<std::string, Material>& materialMap);

/* Get a material from a scene material map */
Material* get_material(const std::string& materialName, std::unordered_map<std::string, Material>& materialMap);
