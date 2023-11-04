#pragma once
#include "vulkan/vulkan.h"
#include <string>
#include <unordered_map>
#include "Pipeline/GraphicsPipeline.h" // I don't understand why ths needs to be included

class GraphicsPipeline;

struct Material {
    GraphicsPipeline *pipeline;
    // VkPipeline pipeline;
    // VkPipelineLayout pipelineLayout;
};

/* Add a material to a scene material map */
void create_material(GraphicsPipeline *graphicsPipeline, const std::string& materialName);

/* Get a material from a scene material map */
[[nodiscard]] Material* get_material(const std::string& materialName);
