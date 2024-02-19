#pragma once

// #include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>
#include <string>

/* Given a string path to a precompiled spirv shader file, create the VkShaderModule */
bool load_shader_spirv_source_to_module(const std::string& shaderSpirvPath, VkDevice device, VkShaderModule& shaderModule);

// /* Given a string path to a shader file, return a string containing its source ready to be compiled */
// std::string load_shader_source_to_string(std::string const& shaderPath);

// /* Given a raw shader source, kind, and file name (use only as an identifier), compile the shader and return its SPIR-V representation */
// void compile_shader(VkDevice device, VkShaderModule& shaderModule, std::string shaderSource, shaderc_shader_kind shaderKind, const char *inputFileName);