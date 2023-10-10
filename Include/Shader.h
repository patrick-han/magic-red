#pragma once

#include <shaderc/shaderc.hpp>
#include "vulkan/vulkan.hpp"

/* Given a string path to a shader file, return a string containing its source ready to be compiled */
std::string load_shader_source_to_string(std::string const& shaderPath);

/* Given a raw shader source, kind, and file name (use only as an identifier), compile the shader and return its SPIR-V representation */
vk::UniqueShaderModule compile_shader(vk::Device device, std::string shaderSource, shaderc_shader_kind shaderKind, const char *inputFileName);