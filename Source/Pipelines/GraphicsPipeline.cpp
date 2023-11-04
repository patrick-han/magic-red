#include "Pipeline/GraphicsPipeline.h"
#include "Shader.h"
#include "VertexDescriptors.h" // Temp
#include "Common/Config.h" // Temp
#include "RootDir.h"

GraphicsPipeline::GraphicsPipeline() {

}

GraphicsPipeline::GraphicsPipeline(const VkDevice logicalDevice, const VkRenderPass renderPass, const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::vector<VkPushConstantRange>& pushConstantRanges) : Pipeline(logicalDevice) {
    // std::string vertexShaderSource = load_shader_source_to_string(ROOT_DIR "Shaders/triangle_mesh.vert");
    // std::string fragmentShaderSource = load_shader_source_to_string(ROOT_DIR "Shaders/triangle_mesh.frag");

    std::string vertexShaderSource = load_shader_source_to_string(std::string(ROOT_DIR) + vertexShaderPath);
    std::string fragmentShaderSource = load_shader_source_to_string(std::string(ROOT_DIR) + fragmentShaderPath);

    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;

    compile_shader(m_logicalDevice, vertexShaderModule, vertexShaderSource, shaderc_glsl_vertex_shader, "vertex shader");
    compile_shader(m_logicalDevice, fragmentShaderModule, fragmentShaderSource, shaderc_glsl_fragment_shader, "fragment shader");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, VkPipelineShaderStageCreateFlags(), VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main", nullptr};
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, VkPipelineShaderStageCreateFlags(), VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main", nullptr};

    std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStages = { vertShaderStageInfo, fragShaderStageInfo };
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, VkPipelineVertexInputStateCreateFlags(), 0u, nullptr, 0u, nullptr };
    VertexInputDescription vertexDescription = VertexInputDescription::get_default_vertex_description();
    vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, VkPipelineInputAssemblyStateCreateFlags(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE };
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), 0.0f, 1.0f };
    VkExtent2D swapChainExtent = VkExtent2D{ WINDOW_WIDTH, WINDOW_HEIGHT };
    VkRect2D scissor = { { 0, 0 }, swapChainExtent };
    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, VkPipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor };
    VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, VkPipelineRasterizationStateCreateFlags(), /*depthClamp*/ VK_FALSE,
    /*rasterizeDiscard*/ VK_FALSE, VK_POLYGON_MODE_FILL, VkCullModeFlags(),
    /*frontFace*/ VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, {}, {}, {}, 1.0f };

    VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, VkPipelineDepthStencilStateCreateFlags(),
        bDepthTest ? VK_TRUE : VK_FALSE,
        bDepthWrite ? VK_TRUE : VK_FALSE,
        VK_COMPARE_OP_LESS_OR_EQUAL,
        VK_FALSE, // depth bounds test
        VK_FALSE, // stencil
        {}, {}, {}, {}
    };

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, VkPipelineMultisampleStateCreateFlags(), VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0 , {}, {}, {}};

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { VK_FALSE, /*srcCol*/ VK_BLEND_FACTOR_ONE,
    /*dstCol*/ VK_BLEND_FACTOR_ZERO, /*colBlend*/ VK_BLEND_OP_ADD,
    /*srcAlpha*/ VK_BLEND_FACTOR_ONE, /*dstAlpha*/ VK_BLEND_FACTOR_ZERO,
    /*alphaBlend*/ VK_BLEND_OP_ADD,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
    VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, VkPipelineColorBlendStateCreateFlags(), /*logicOpEnable=*/false, VK_LOGIC_OP_COPY, /*attachmentCount=*/1, /*colourAttachments=*/&colorBlendAttachment, {}};

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = VkPipelineDynamicStateCreateFlags();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Push constants
    // VkPushConstantRange meshPushConstant = {};
    // meshPushConstant.offset = 0;
    // meshPushConstant.size = sizeof(MeshPushConstants);
    // meshPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


    CreatePipelineLayout(pushConstantRanges);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        VkPipelineCreateFlags(),
        2, 
        pipelineShaderStages.data(), 
        &vertexInputInfo, 
        &inputAssembly, 
        nullptr, 
        &viewportState, 
        &rasterizer, 
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicState, // Dynamic State
        m_pipelineLayout,
        renderPass,
        0,
        {},
        0
    };

    vkCreateGraphicsPipelines(m_logicalDevice, {}, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    vkDestroyShaderModule(m_logicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_logicalDevice, fragmentShaderModule, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {

}