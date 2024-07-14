#pragma once
#include <limits>


#include <cstdint>
#include <limits>

using GPUMeshId = std::size_t;
static const auto NULL_GPU_MESH_ID = std::numeric_limits<std::size_t>::max();

// using JointId = std::uint16_t;
// static const JointId NULL_JOINT_ID = std::numeric_limits<JointId>::max();
// static const JointId ROOT_JOINT_ID = 0;

using GPUTextureId = std::uint32_t;
static const auto NULL_GPU_TEXTURE_ID = std::numeric_limits<std::uint32_t>::max();

using GraphicsPipelineId = std::uint32_t;
static const auto NULL_GRAPHICS_PIPELINE_ID = std::numeric_limits<std::uint32_t>::max();

using MaterialId = std::uint32_t;
static const auto NULL_MATERIAL_ID = std::numeric_limits<std::uint32_t>::max();