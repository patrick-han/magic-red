#include "Renderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?

#include <stdexcept>
#include <cstdlib>
#include <span>
#include <Common/RootDir.h>
#include <Common/Platform.h>
#include <Common/Compiler/Unused.h>

#include <Camera/Camera.h>
#include <Common/Log.h>
#include <DeletionQueue.h>
#include <Mesh/Mesh.h>
#include <Model/Model.h>

#include <Wrappers/Image.h>
#include <Wrappers/ImageMemoryBarrier.h>
#include <Wrappers/DynamicRendering.h>

#include <Vertex/VertexDescriptors.h>
#include <Common/Defaults.h>
#include <Descriptor/Descriptor.h>

#include <IncludeHelpers/ImguiIncludes.h>

// Frame data
int frameNumber = 0;
float deltaTime = 0.0f; // Time between current and last frame
uint64_t lastFrameTick = 0;
uint64_t currentFrameTick = 0;
bool interactableUI = false;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -2.0f);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
Camera camera(cameraPos, worldUp, cameraFront, -90.0f, 0.0f, 45.0f, true);
float cameraSpeed = 0.0f;
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2; // Initial mouse positions


Renderer::Renderer()
{}

void Renderer::run() {
    initWindow();
    init_graphics();
    mainLoop();
    cleanup();
}

void Renderer::initWindow() {
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    m_window = SDL_CreateWindow("Magic Red", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void Renderer::init_graphics() {
    m_GfxDevice.init(m_window);
    init_lights();
    create_samplers();
    init_texture_descriptors();
    init_assets();
    init_material_data();
    init_scene_data();

    update_texture_descriptors();
    
    // build_pipelines();
    // build_render_objects();
    init_imgui();
}

void Renderer::init_lights() {
        m_CPUPointLights.push_back(PointLight(glm::vec3(0.0f, 3.5f, -4.0f), 0, glm::vec3(1.0f, 223.0f/255.0f, 188.0f/255.0f), 1.0));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)  
    {
        AllocatedBuffer pointLightBuffer;
        m_GPUPointLightsBuffers_F.push_back(pointLightBuffer);
        upload_buffer(
            m_GPUPointLightsBuffers_F[i],
            m_CPUPointLights.size() * sizeof(PointLight),
            m_CPUPointLights.data(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
            m_GfxDevice.m_vmaAllocator
        );
    }

    for (auto &lightBuffer : m_GPUPointLightsBuffers_F)
    {
        VkBufferDeviceAddressInfoKHR addressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
            .buffer = lightBuffer.buffer
        };
        lightBuffer.gpuAddress  = vkGetBufferDeviceAddress(m_GfxDevice, &addressInfo);
    }
}

void Renderer::create_samplers() {
    VkSamplerCreateInfo linearCI = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .anisotropyEnable = VK_FALSE,
            // .maxAnisotropy = maxAnisotropy,
        };
    vkCreateSampler(m_GfxDevice, &linearCI, nullptr, &m_linearSampler);
    VkSamplerCreateInfo nearestCI = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        };
    vkCreateSampler(m_GfxDevice, &nearestCI, nullptr, &m_nearestSampler);
}

void Renderer::init_texture_descriptors() {
    // Describe what and how many descriptors we want and create our pool
    // These may be distributed in any combination among our sets
    
    m_bindlessDescriptorAllocator.init_pool(m_GfxDevice, 1, m_descriptorTypeCounts);

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_SAMPLER);

    // We only need a single layout since they are all the same for each frame in flight
    // m_sceneDataDescriptorSetLayouts.push_back(layoutBuilder.buildLayout(m_GfxDevice, VK_SHADER_STAGE_FRAGMENT_BIT));
    m_bindlessDescriptorSetLayout = layoutBuilder.buildLayout(m_GfxDevice, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_bindlessDescriptorSet = m_bindlessDescriptorAllocator.allocate(m_GfxDevice, m_bindlessDescriptorSetLayout);

    // // Scene Data buffer init
    // for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    // {
    //     m_sceneDataDescriptorSets_F.push_back(
    //         m_globalDescriptorAllocator.allocate(m_GfxDevice, m_sceneDataDescriptorSetLayout)
    //     );

    //     // Update descriptor set(s)
    //     VkDescriptorBufferInfo sceneDataBufferInfo = {
    //         .buffer = m_GPUSceneDataBuffers_F[i].buffer,
    //         .offset = 0,
    //         .range = sizeof(CPUSceneData)
    //     };
    //     VkWriteDescriptorSet sceneDataDescriptorWrite = {
    //         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //         .pNext = nullptr,
    //         .dstSet = m_sceneDataDescriptorSets_F[i],
    //         .dstBinding = 0,
    //         .dstArrayElement = {},
    //         .descriptorCount = 1,
    //         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //         .pImageInfo = nullptr,
    //         .pBufferInfo = &sceneDataBufferInfo,
    //         .pTexelBufferView = nullptr // ???
    //     };
    //     vkUpdateDescriptorSets(m_GfxDevice, 1, &sceneDataDescriptorWrite, 0, nullptr);
    // }
}

// temp
std::vector<VkPushConstantRange> defaultPushConstantRanges = {DefaultPushConstants::range()};

void Renderer::init_assets() {
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_GfxDevice.m_swapChainFormat,
        .depthAttachmentFormat = m_GfxDevice.m_depthImage.imageFormat,
        .stencilAttachmentFormat = {}
    };

    GraphicsPipeline defaultPipeline(
        m_GfxDevice,
        &pipelineRenderingCI, 
        std::string("Shaders/triangle_mesh.vert.spv"), 
        std::string("Shaders/triangle_mesh.frag.spv"), 
        defaultPushConstantRanges,
        // std::span<const VkDescriptorSetLayout>(m_sceneDataDescriptorSetLayouts.data(), 1), // TODO: They're all the same and this only has 1 layout for now
        std::span<const VkDescriptorSetLayout>(&m_bindlessDescriptorSetLayout, 1),
        // std::span<const VkDescriptorSetLayout>(),
        {WINDOW_WIDTH, WINDOW_HEIGHT}
    );

    GraphicsPipelineId defaultPipelineId = m_PipelineCache.add_pipeline(m_GfxDevice, defaultPipeline);

    // {
    //     // Sponza mesh
    //     CPUModel sponzaModel(ROOT_DIR "/Assets/Meshes/sponza-gltf/Sponza.gltf", false, m_TextureCache);
    //     GPUMeshId sponzaMeshId = m_MeshCache.add_mesh(m_GfxDevice, sponzaModel.m_cpuMesh);
    //     RenderObject sponzaObject(defaultPipelineId, sponzaMeshId, m_PipelineCache, m_MeshCache);
    //     glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, -5.0f, 0.0f));
    //     glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.05f, 0.05f, 0.05f));
    //     sponzaObject.set_transform(translate * scale);
    //     m_sceneRenderObjects.push_back(sponzaObject);
    // }
    
    // {
    //     // Suzanne mesh
    //     CPUModel suzanneModel(ROOT_DIR "/Assets/Meshes/suzanne.glb", true, m_TextureCache);
    //     GPUMeshId suzanneMeshId = m_MeshCache.add_mesh(m_GfxDevice, suzanneModel.m_cpuMesh);
    //     RenderObject suzanneObject(defaultPipelineId, suzanneMeshId, m_PipelineCache, m_MeshCache);
    //     glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
    //     suzanneObject.set_transform(monkeyTranslate);
    //     m_sceneRenderObjects.push_back(suzanneObject);
    // }

    {
        // Helmet mesh
        UNUSED(m_materialDataBuffer);
        CPUModel helmetModel(ROOT_DIR "/Assets/Meshes/DamagedHelmet.glb", true, m_MaterialCache, m_TextureCache, m_GfxDevice);
        GPUMeshId helmetMeshId = m_MeshCache.add_mesh(m_GfxDevice, helmetModel.m_cpuMesh);
        RenderObject helmetObject(defaultPipelineId, helmetMeshId, helmetModel.m_materialId, m_PipelineCache, m_MeshCache);
        glm::mat4 helmetTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 3.0f, 0.0f));
        helmetTransform = glm::rotate(helmetTransform, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
        helmetObject.set_transform(helmetTransform);
        m_sceneRenderObjects.push_back(helmetObject);
    }
}

void Renderer::init_material_data() {
    upload_buffer(
        m_materialDataBuffer,
        m_MaterialCache.get_material_count() * sizeof(Material),
        m_MaterialCache.get_material_data(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
        m_GfxDevice.m_vmaAllocator
    );
}

void Renderer::init_scene_data() {
    m_CPUSceneData.view = camera.get_view_matrix();
    glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
    projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
    m_CPUSceneData.projection = projection;
    m_CPUSceneData.cameraWorldPosition = camera.get_world_position();
    // m_CPUSceneData.numPointLights = static_cast<uint32_t>(m_CPUPointLights.size());
    m_CPUSceneData.lightBufferAddress = 0; // TODO during update_scene_data()? or now

    // Material data only set once at the beginning, since for now we are loading all assets in ahead of time
    VkBufferDeviceAddressInfoKHR addressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
        .buffer = m_materialDataBuffer.buffer
    };
    m_CPUSceneData.materialBufferAddress = vkGetBufferDeviceAddress(m_GfxDevice, &addressInfo);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)  
    {
        AllocatedBuffer sceneDataBuffer;
        m_GPUSceneDataBuffers_F.push_back(sceneDataBuffer);
        upload_buffer(
            m_GPUSceneDataBuffers_F[i],
            sizeof(CPUSceneData),
            &m_CPUSceneData,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
            m_GfxDevice.m_vmaAllocator
        );
    }

    for (auto &sceneDataBuffer : m_GPUSceneDataBuffers_F)
    {
        VkBufferDeviceAddressInfoKHR addressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
            .buffer = sceneDataBuffer.buffer
        };
        sceneDataBuffer.gpuAddress  = vkGetBufferDeviceAddress(m_GfxDevice, &addressInfo);
    }
}

void Renderer::update_texture_descriptors() {
    // Update descriptor set
    VkDescriptorImageInfo texture0Info = {
        .imageView = m_TextureCache.get_texture(1).allocatedImage.imageView, // TODO
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // TODO
    };

    VkDescriptorImageInfo linearSamplerInfo = {
        .sampler = m_linearSampler
    };

    VkWriteDescriptorSet texture0DescriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_bindlessDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = {},
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &texture0Info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet linearSamplerDescriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_bindlessDescriptorSet,
        .dstBinding = 1,
        .dstArrayElement = {},
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &linearSamplerInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet writes[2] = {texture0DescriptorWrite, linearSamplerDescriptorWrite};
    vkUpdateDescriptorSets(m_GfxDevice, 2, &writes[0], 0, nullptr);
}

// void build_pipelines() {

// }

// void build_render_objects() {
    
// }

void Renderer::init_imgui() {
    // Create a descriptor pool for IMGUI
    // The size of the pool is very oversized, but it's copied from imgui demo
    VkDescriptorPoolSize poolSizes[] = { 
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
        .pPoolSizes = poolSizes
    };
    vkCreateDescriptorPool(m_GfxDevice, &poolInfo, nullptr, &m_imguiPool);

    // Initialize core structures of imgui library
    ImGui::CreateContext();

    // Initialize imgui for SDL
    ImGui_ImplSDL3_InitForVulkan(m_window);

    // Initialize imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance = m_GfxDevice.get_instance(),
        .PhysicalDevice = m_GfxDevice.get_physical_device(),
        .Device = m_GfxDevice,
        .Queue = m_GfxDevice.get_graphics_queue(),
        .DescriptorPool = m_imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .UseDynamicRendering = true,
        .ColorAttachmentFormat = m_GfxDevice.m_swapChainFormat,
    };

    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

    // Execute a gpu command to upload imgui font textures
    // Update: No longer requires passing a command buffer (builds own pool + buffer internally), also no longer require destroy DestroyFontUploadObjects()
    // immediate_submit([=]() { 
        ImGui_ImplVulkan_CreateFontsTexture(); 
    // });

    // add the destroy the imgui created structures
}

void Renderer::draw_objects() {
    VkCommandBuffer cmdBuffer = m_GfxDevice.get_frame_command_buffer(m_currentFrame);
    VkDeviceAddress sceneDataBufferAddress = m_GPUSceneDataBuffers_F[m_currentFrame].gpuAddress;

    for (RenderObject renderObject :  m_sceneRenderObjects) {
       renderObject.bind_and_draw(cmdBuffer, std::span<const VkDescriptorSet>(&m_bindlessDescriptorSet, 1), sceneDataBufferAddress);
    }
}

void Renderer::draw_imgui(VkImageView targetImageView) {
    //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    VkRenderingAttachmentInfoKHR colorAttachment = rendering_attachment_info(
        targetImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr
    );
    VkRenderingInfoKHR renderingInfo = rendering_info_fullscreen(1, &colorAttachment, nullptr);

    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdBeginRenderingKHR"));

    VkCommandBuffer cmdBuffer = m_GfxDevice.get_frame_command_buffer(m_currentFrame);

    vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdEndRenderingKHR"));
    vkCmdEndRenderingKHR(cmdBuffer);
}

void Renderer::update_lights(uint32_t frameInFlightIndex) {
    int lightCircleRadius = 5;
    float lightCircleSpeed = 0.02f;
    m_CPUPointLights[0].worldSpacePosition = glm::vec3(
        lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber),
        0.0,
        lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber)
    );
    

    update_buffer(
        m_GPUPointLightsBuffers_F[frameInFlightIndex], 
        m_CPUPointLights.size() * sizeof(PointLight),
        m_CPUPointLights.data(),
        m_GfxDevice.m_vmaAllocator
    );
}

void Renderer::update_scene_data(uint32_t frameInFlightIndex) {
    m_CPUSceneData.view = camera.get_view_matrix();
    glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
    projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
    m_CPUSceneData.projection = projection;
    m_CPUSceneData.cameraWorldPosition = camera.get_world_position();
    // m_CPUSceneData.numPointLights = static_cast<uint32_t>(m_CPUPointLights.size());
    m_CPUSceneData.lightBufferAddress = m_GPUPointLightsBuffers_F[frameInFlightIndex].gpuAddress;

    update_buffer(
        m_GPUSceneDataBuffers_F[frameInFlightIndex], 
        sizeof(CPUSceneData),
        &m_CPUSceneData,
        m_GfxDevice.m_vmaAllocator
    );
}

// void Renderer::update_scene_data_descriptors(uint32_t frameInFlightIndex) {
//     m_CPUSceneData.view = camera.get_view_matrix();
//     glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
//     projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
//     m_CPUSceneData.projection = projection;
//     m_CPUSceneData.cameraWorldPosition = camera.get_world_position();
//     // m_CPUSceneData.numPointLights = static_cast<uint32_t>(m_CPUPointLights.size());
//     m_CPUSceneData.lightBufferAddress = m_GPUPointLightsBuffers_F[frameInFlightIndex].gpuAddress;
    
//     update_buffer(
//         m_GPUSceneDataBuffers_F[frameInFlightIndex], 
//         sizeof(CPUSceneData),
//         &m_CPUSceneData,
//         m_GfxDevice.m_vmaAllocator
//     );

//     // Update descriptor set(s)
//     VkDescriptorBufferInfo sceneDataBufferInfo = {
//         .buffer = m_GPUSceneDataBuffers_F[frameInFlightIndex].buffer,
//         .offset = 0,
//         .range = sizeof(CPUSceneData)
//     };

//     VkWriteDescriptorSet sceneDataDescriptorWrite = {
//         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//         .pNext = nullptr,
//         .dstSet = m_sceneDataDescriptorSets_F[frameInFlightIndex],
//         .dstBinding = 0,
//         .dstArrayElement = {},
//         .descriptorCount = 1,
//         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//         .pImageInfo = nullptr,
//         .pBufferInfo = &sceneDataBufferInfo,
//         .pTexelBufferView = nullptr // ???
//     };
//     vkUpdateDescriptorSets(m_GfxDevice, 1, &sceneDataDescriptorWrite, 0, nullptr);
// }

void Renderer::drawFrame() {
        
        // Wait for previous frame to finish rendering before allowing us to acquire another image
        VkFence renderFence = m_GfxDevice.get_frame_fence(m_currentFrame);
        VkResult res = vkWaitForFences(m_GfxDevice, 1, &renderFence, true, (std::numeric_limits<uint64_t>::max)());
        vkResetFences(m_GfxDevice, 1, &renderFence);
        update_lights(m_currentFrame); // Executes immediately
        update_scene_data(m_currentFrame);


        VkSemaphore imageAvaliableSemaphore = m_GfxDevice.get_frame_imageAvailableSemaphore(m_currentFrame);
        VkSemaphore renderFinishedSemaphore = m_GfxDevice.get_frame_renderFinishedSemaphore(m_currentFrame);

        uint32_t imageIndex;
        res = vkAcquireNextImageKHR(m_GfxDevice, m_GfxDevice.m_swapChain, std::numeric_limits<uint64_t>::max(), imageAvaliableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (!((res == VK_SUCCESS) || (res == VK_SUBOPTIMAL_KHR))) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Failed to acquire image from Swap Chain!");
        }
        VkCommandBuffer cmdBuffer = m_GfxDevice.get_frame_command_buffer(m_currentFrame);
        vkResetCommandBuffer(cmdBuffer, {});

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        {
            // Transition swapchain to color attachment write
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_GfxDevice.m_swapChainImages[imageIndex], 
                {}, 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );
            vkCmdPipelineBarrier(
                cmdBuffer, 
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                {},
                0, nullptr,
                0, nullptr,
                1, &imb
            );
        }

        VkRenderingAttachmentInfoKHR colorAttachmentInfo = rendering_attachment_info(
            m_GfxDevice.m_swapChainImageViews[imageIndex],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_COLOR
        );

        VkRenderingAttachmentInfoKHR depthAttachmentInfo  = rendering_attachment_info(
            m_GfxDevice.m_depthImage.imageView,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_DEPTH
        );

        VkRenderingInfoKHR renderingInfo = rendering_info_fullscreen(
            1, &colorAttachmentInfo, &depthAttachmentInfo
        );

        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdBeginRenderingKHR"));
        vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);

        vkCmdSetViewport(cmdBuffer, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
        vkCmdSetScissor(cmdBuffer, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);
        draw_objects();

        PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdEndRenderingKHR"));
        vkCmdEndRenderingKHR(cmdBuffer);



        // Draw imgui
        draw_imgui(m_GfxDevice.m_swapChainImageViews[imageIndex]);

        {
            // Transition swapchain to correct presentation layout
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_GfxDevice.m_swapChainImages[imageIndex], 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                {},
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            );
            vkCmdPipelineBarrier(
                cmdBuffer, 
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                {},
                0, nullptr,
                0, nullptr,
                1,&imb
            );
        }

        vkEndCommandBuffer(cmdBuffer);


        // Submit graphics workload
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvaliableSemaphore;
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
        vkQueueSubmit(m_GfxDevice.get_graphics_queue(), 1, &submitInfo, renderFence);


        // Present frame
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_GfxDevice.m_swapChain;
        presentInfo.pImageIndices = &imageIndex;
        // res = vkQueuePresentKHR(presentQueue, &presentInfo);
        res = vkQueuePresentKHR(m_GfxDevice.get_graphics_queue(), &presentInfo);


        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        frameNumber++;
        
}

void Renderer::mainLoop() {
    SDL_Event sdlEvent;
    bool bQuit = false;
    ImGuiIO& io = ImGui::GetIO();
    UNUSED(io);
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&sdlEvent) != 0) {
            ImGui_ImplSDL3_ProcessEvent(&sdlEvent);      
            if (sdlEvent.type == SDL_EVENT_QUIT) { // Built in Alt+F4 or hitting the 'x' button
                SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
                bQuit = true;
            }
            // For single key presses
            if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
                if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                    interactableUI = !interactableUI;
                    if (!interactableUI) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        camera.freeze_camera();
                    } else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        camera.unfreeze_camera();
                    }
                }
            }
            
            // Mouse motion
            if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
                float xoffset = sdlEvent.motion.xrel;
                float yoffset =  -sdlEvent.motion.yrel;
                const float sensitivity = 0.1f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;
                camera.process_mouse_movement(xoffset, yoffset, true);
            }
        }
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();
        drawFrame();

        lastFrameTick = currentFrameTick;
        currentFrameTick = SDL_GetTicks();
        deltaTime = (currentFrameTick - lastFrameTick) / 1000.f;

        // For multi-press/holding down keys
        const Uint8* state = SDL_GetKeyboardState(nullptr);
        if (state[SDL_SCANCODE_LSHIFT]) {
            cameraSpeed = 20.0f;
        } else {
            cameraSpeed = 10.0f;
        }
        if (state[SDL_SCANCODE_ESCAPE]) {
            SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
            bQuit = true;
        }
        if (state[SDL_SCANCODE_W]) {
            camera.process_keyboard_input(CameraMovementDirection::FORWARD, cameraSpeed * deltaTime);
        }
        if (state[SDL_SCANCODE_S]) {
            camera.process_keyboard_input(CameraMovementDirection::BACKWARD, cameraSpeed * deltaTime);
        }
        if (state[SDL_SCANCODE_A]) {
            camera.process_keyboard_input(CameraMovementDirection::LEFT, cameraSpeed * deltaTime);
        }
        if (state[SDL_SCANCODE_D]) {
            camera.process_keyboard_input(CameraMovementDirection::RIGHT, cameraSpeed * deltaTime);
        }
        if (state[SDL_SCANCODE_SPACE]) {
            camera.process_keyboard_input(CameraMovementDirection::UP, cameraSpeed * deltaTime);
        }
        if (state[SDL_SCANCODE_LCTRL]) {
            camera.process_keyboard_input(CameraMovementDirection::DOWN, cameraSpeed * deltaTime);
        }
    }
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(m_GfxDevice);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_GfxDevice, m_imguiPool, nullptr);

//        vkDestroyDescriptorSetLayout(m_GfxDevice, m_sceneDataDescriptorSetLayouts[0], nullptr);
    vkDestroyDescriptorSetLayout(m_GfxDevice, m_bindlessDescriptorSetLayout, nullptr);

    m_materialDataBuffer.cleanup(m_GfxDevice.m_vmaAllocator);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_GPUPointLightsBuffers_F[i].cleanup(m_GfxDevice.m_vmaAllocator);
        m_GPUSceneDataBuffers_F[i].cleanup(m_GfxDevice.m_vmaAllocator);
    }

    m_bindlessDescriptorAllocator.destroy_pool(m_GfxDevice);

    vkDestroySampler(m_GfxDevice, m_linearSampler, nullptr);
    vkDestroySampler(m_GfxDevice, m_nearestSampler, nullptr);

    m_TextureCache.cleanup(m_GfxDevice);
    m_PipelineCache.cleanup(m_GfxDevice);
    m_MeshCache.cleanup(m_GfxDevice);
    m_GfxDevice.cleanup();

    SDL_DestroyWindow(m_window);
}
