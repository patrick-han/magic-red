#include "BlinnPhongLightingStage.h"
#include <Rendering/GfxDevice.h>
#include <Common/Defaults.h>
#include <Texture/TextureCache.h>


inline static constexpr std::array<DescriptorSetLayoutBinding, 4> lightingDescriptorBindings {{
    // GBuffer
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1},
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1},
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1},
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1}
}};

BlinnPhongLightingStage::BlinnPhongLightingStage(
    const GfxDevice& _gfxDevice,
    const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
    const TextureCache& _textureCache,
    const VkDescriptorPool _globalDescriptorPool,
    const VkDescriptorSetLayout _bindlessDescriptorSetLayout,
    const VkDescriptorSet _bindlessDescriptorSet,
    GPUTextureId _albedoRTId,
    GPUTextureId _worldNormalsRTId,
    GPUTextureId _metallicRoughnessRTId
    )
    : StageBase(_gfxDevice)
    , m_textureCache(_textureCache)
    , m_globalDescriptorPool(_globalDescriptorPool)
    , m_bindlessDescriptorSet(_bindlessDescriptorSet)
    , m_pipeline(m_gfxDevice)
    {
        {
            // Build a descriptor set layout
            std::vector<VkDescriptorSetLayoutBinding> lightingDescriptorSetLayoutBindings;
            uint32_t bindingIndex = 0;
            for(DescriptorSetLayoutBinding descriptorBinding : lightingDescriptorBindings)
            {
                VkDescriptorSetLayoutBinding newBinding = {
                    .binding = bindingIndex,
                    .descriptorType = descriptorBinding.descriptorType,
                    .descriptorCount = static_cast<uint32_t>(descriptorBinding.descriptorCount),
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                };
                lightingDescriptorSetLayoutBindings.push_back(newBinding);
                bindingIndex++;
            }
            VkDescriptorSetLayoutCreateInfo gbufferSetLayoutCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(lightingDescriptorSetLayoutBindings.size()),
                .pBindings = lightingDescriptorSetLayoutBindings.data()
            };
            vkCreateDescriptorSetLayout(m_gfxDevice, &gbufferSetLayoutCreateInfo, nullptr, &m_lightingDescriptorSetLayout);

            // Allocate the descriptor set
            VkDescriptorSetAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_globalDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_lightingDescriptorSetLayout
            };

            vkAllocateDescriptorSets(m_gfxDevice, &allocateInfo, &m_lightingDescriptorSet);
        }


        // Done like this instead of constructing temps in a for loop because of pImageInfo
        std::vector<VkWriteDescriptorSet> gbufferDescriptorWrites;

        {
                VkDescriptorImageInfo albedoImageInfo = {
                    .imageView = m_textureCache.get_render_texture_texture(_albedoRTId).allocatedImage.imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                VkWriteDescriptorSet albedoWriteDescriptor = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_lightingDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &albedoImageInfo
                };
                gbufferDescriptorWrites.push_back(albedoWriteDescriptor);
        }
        {
                VkDescriptorImageInfo normalsImageInfo = {
                    .imageView = m_textureCache.get_render_texture_texture(_worldNormalsRTId).allocatedImage.imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                VkWriteDescriptorSet normalsWriteDescriptor = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_lightingDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &normalsImageInfo
                };
                gbufferDescriptorWrites.push_back(normalsWriteDescriptor);
        }
        {
                VkDescriptorImageInfo metallicRoughnessImageInfo = {
                    .imageView = m_textureCache.get_render_texture_texture(_metallicRoughnessRTId).allocatedImage.imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                VkWriteDescriptorSet metallicRoughnessWriteDescriptor = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_lightingDescriptorSet,
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &metallicRoughnessImageInfo
                };
                gbufferDescriptorWrites.push_back(metallicRoughnessWriteDescriptor);
        }
        {
                VkDescriptorImageInfo depthImageInfo = {
                    .imageView = m_gfxDevice.m_depthImage.imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                VkWriteDescriptorSet depthWriteDescriptor = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_lightingDescriptorSet,
                    .dstBinding = 3,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &depthImageInfo
                };
                gbufferDescriptorWrites.push_back(depthWriteDescriptor);
        }


        vkUpdateDescriptorSets(m_gfxDevice, static_cast<uint32_t>(gbufferDescriptorWrites.size()), gbufferDescriptorWrites.data(), 0, nullptr);


        std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {{_bindlessDescriptorSetLayout, m_lightingDescriptorSetLayout}};

        VertexInputDescription vertexDescription;
        m_pipeline.BuildPipeline(
            _pipelineRenderingCreateInfo
            , m_vertexShaderPath, m_fragmentShaderPath
            , vertexDescription
            , m_pushConstantRanges
            , descriptorSetLayouts
            , m_extent
            );
    }

BlinnPhongLightingStage::~BlinnPhongLightingStage() {}

void BlinnPhongLightingStage::Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress) {

    vkCmdSetViewport(cmdBuffer, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
    vkCmdSetScissor(cmdBuffer, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get_pipeline_handle());

    // Bindless descriptor set shared for color pass
    std::array<VkDescriptorSet, 2> descriptorSets = {{m_bindlessDescriptorSet, m_lightingDescriptorSet}}; // TODO:: smelly?
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_pipeline.get_pipeline_layout(),
      0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);


    // TODO: different push constants template, just using this for the sceneBuffer for now
    DefaultPushConstants pushConstants;
    pushConstants.model = glm::mat4(0.0f);
    pushConstants.sceneDataBufferAddress = sceneDataBufferAddress;
    pushConstants.materialId = 0;
    vkCmdPushConstants(cmdBuffer, m_pipeline.get_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}

void BlinnPhongLightingStage::Cleanup() {
    vkDestroyDescriptorSetLayout(m_gfxDevice, m_lightingDescriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(m_gfxDevice, m_pipeline.get_pipeline_layout(), nullptr);
    vkDestroyPipeline(m_gfxDevice, m_pipeline.get_pipeline_handle(), nullptr);
}
