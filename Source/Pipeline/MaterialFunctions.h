#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <Pipeline/GraphicsPipeline.h>

/* Add a material to a scene material map */
void create_material(GraphicsPipeline graphicsPipeline, const std::string& materialName);

/* Get a material from a scene material map */
[[nodiscard]] GraphicsPipeline* get_material(const std::string& materialName, std::unordered_map<std::string, GraphicsPipeline>& materialMap);
