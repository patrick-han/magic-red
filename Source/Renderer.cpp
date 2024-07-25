#include "Renderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define VMA_IMPLEMENTATION
#include <IncludeHelpers/VmaIncludes.h>

#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?

#include <cstdlib>
#include <span>
#include <array>
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

#include <External/tinygltf/stb_image.h>

// Frame data
int frameNumber = 0;
float deltaTime = 0.0f; // Time between current and last frame
uint64_t lastFrameTick = 0;
uint64_t currentFrameTick = 0;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -2.0f);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
Camera camera(cameraPos, worldUp, cameraFront, -90.0f, 0.0f, 45.0f, true);
float cameraSpeed = 0.0f;
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2; // Initial mouse positions

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
    init_bindless_descriptors();
    init_assets();
    init_material_data();
    init_scene_data();

    init_render_targets();
    init_render_stages();

    update_texture_descriptors();

    init_imgui();
}

void Renderer::init_lights() {
    // Directional Light
    m_directionalLight.direction.x = -0.2f;
    m_directionalLight.direction.y = -1.0f;
    m_directionalLight.direction.z = -0.3f;
    m_directionalLight.power = 1.0f;


    // Point lights
    m_CPUPointLights.emplace_back(glm::vec3(0.0f, 3.5f, -4.0f), 1.0f, glm::vec3(1.0f, 223.0f/255.0f, 188.0f/255.0f), 1.0f, 0.09f, 0.032f);
    m_CPUPointLights.emplace_back(glm::vec3(0.0f, 3.5f, 1.0f), 1.0f, glm::vec3(45.0f/255.0f, 25.0f/255.0f, 188.0f/255.0f), 1.0f, 0.09f, 0.032f);

    if (m_CPUPointLights.size() > 0)
    {
        m_pointLightsExist = true;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            upload_buffer(
                m_GPUPointLightsBuffers[i],
                m_CPUPointLights.size() * sizeof(PointLight),
                m_CPUPointLights.data(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
                m_GfxDevice.m_vmaAllocator
            );
        }

        for (auto& lightBuffer : m_GPUPointLightsBuffers)
        {
            VkBufferDeviceAddressInfoKHR addressInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
                .buffer = lightBuffer.buffer
            };
            lightBuffer.gpuAddress = vkGetBufferDeviceAddress(m_GfxDevice, &addressInfo);
        }
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

void Renderer::init_bindless_descriptors() {
    // maxPerStageResources on M2 Pro is 159, it's insanely high on a 4080S tho (4294967295)
    // maxPerStageDescriptorUpdateAfterBindSampledImages on M2 Pro is 128, 1048576 on the 4080S
    // This seems likely a driver restriction rather than HW related?
    constexpr uint32_t maxBindlessResourceCount = 16536;
    constexpr uint32_t maxSamplerCount = 2;

    // Create a global descriptor pool, and let it know how many of each descriptor type we want up front
    std::array<VkDescriptorPoolSize, 2> bindlessDescriptorPoolSizes {{
        { VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplerCount}, // TODO: We'll just have 1 nearest and 1 linear sampler for now
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxBindlessResourceCount}
    }};
    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT, // Allows us to update textures in a bindless array
        // .maxSets = maxBindlessResourceCount * static_cast<uint32_t>(bindlessDescriptorPoolSizes.size()), // ?
        .maxSets = maxBindlessResourceCount + maxSamplerCount, // ? potentially 1 set for each resource
        .poolSizeCount = static_cast<uint32_t>(bindlessDescriptorPoolSizes.size()),
        .pPoolSizes = bindlessDescriptorPoolSizes.data()
    };
    vkCreateDescriptorPool(m_GfxDevice, &poolCreateInfo, nullptr, &m_bindlessPool);

    // Build a descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> bindlessDescriptorSetLayoutBindings;
    uint32_t bindingIndex = 0;
    for(VkDescriptorPoolSize poolSize : bindlessDescriptorPoolSizes)
    {
        VkDescriptorSetLayoutBinding newBinding = {
            .binding = bindingIndex,
            .descriptorType = poolSize.type,
            .descriptorCount = poolSize.descriptorCount,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        bindlessDescriptorSetLayoutBindings.push_back(newBinding);
        bindingIndex++;
    }
    // Flags required for bindless stuff
    // We only need a single layout since they are all the same for each frame in flight
    // m_sceneDataDescriptorSetLayouts.push_back(layoutBuilder.buildLayout(m_GfxDevice, VK_SHADER_STAGE_FRAGMENT_BIT));
    const VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT 
                                                    | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    std::vector<VkDescriptorBindingFlags> descriptorBindingFlags;
    for(size_t i = 0; i < bindlessDescriptorSetLayoutBindings.size(); i++)
    {
        descriptorBindingFlags.push_back(bindlessFlags);
    }
    descriptorBindingFlags.back() |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT; // Permits use of variable array size for a set (with the caveat that only the last binding in the set can be of variable length)
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedBindingInfo { 
        .sType =  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount = static_cast<uint32_t>(descriptorBindingFlags.size()),
        .pBindingFlags = descriptorBindingFlags.data()
    };
    VkDescriptorSetLayoutCreateInfo bindlessSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &extendedBindingInfo,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        .bindingCount = static_cast<uint32_t>(bindlessDescriptorSetLayoutBindings.size()),
        .pBindings = bindlessDescriptorSetLayoutBindings.data()
    };
    vkCreateDescriptorSetLayout(m_GfxDevice, &bindlessSetLayoutCreateInfo, nullptr, &m_bindlessDescriptorSetLayout);

    // Allocate the descriptor set
    uint32_t maxBinding = maxBindlessResourceCount - 1;
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountInfo { 
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &maxBinding // Number of descriptors, -1?
    };
    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescriptorCountInfo,
        .descriptorPool = m_bindlessPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_bindlessDescriptorSetLayout
    };
    
    vkAllocateDescriptorSets(m_GfxDevice, &allocateInfo, &m_bindlessDescriptorSets.at(0));
}

void Renderer::init_assets() {
    {
        // Used as a placeholder texture when a material is missing an given texture map
        int width, height, numberComponents;
        unsigned char *data = stbi_load(ROOT_DIR "/Assets/EngineIncluded/default_1_texture.png", &width, &height, &numberComponents, 4);
        TextureLoadingData textureLoadingData = {
            .data = data,
            .texSize = {width, height, 4}
        };
        GPUTextureId placeholderTextureId = m_TextureCache.add_texture(m_GfxDevice, textureLoadingData, "default_1_texture.png");
        UNUSED(placeholderTextureId);
        stbi_image_free(data);
    }
    {
        // Used as a placeholder texture when a material is missing an given texture map
        int width, height, numberComponents;
        unsigned char *data = stbi_load(ROOT_DIR "/Assets/EngineIncluded/missing_diffuse_texture.png", &width, &height, &numberComponents, 4);
        TextureLoadingData textureLoadingData = {
            .data = data,
            .texSize = {width, height, 4}
        };
        GPUTextureId placeholderTextureId = m_TextureCache.add_texture(m_GfxDevice, textureLoadingData, "missing_diffuse_texture.png");
        UNUSED(placeholderTextureId);
        stbi_image_free(data);
    }

    // {
    //    // Sponza mesh
    //    CPUModel sponzaModel(ROOT_DIR "/Assets/Meshes/sponza-gltf/Sponza.gltf", false, m_MaterialCache, m_TextureCache, m_GfxDevice);
    //    glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
    //    glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.05f, 0.05f, 0.05f));
    //    for (CPUMesh& mesh : sponzaModel.m_cpuMeshes)
    //    {
    //        GPUMeshId sponzaMeshId = m_MeshCache.add_mesh(m_GfxDevice, mesh);
    //        RenderObject sponzaObject(defaultPipelineId, sponzaMeshId, m_GraphicsPipelineCache, m_MeshCache);
    //        sponzaObject.set_transform(translate * scale);
    //        m_sceneRenderObjects.push_back(sponzaObject);
    //    }
    // }

    glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 2.0f, 2.0f));
    glm::mat4 rotate = glm::rotate(translate, rm, glm::vec3(rx, ry, rz));
    glm::mat4 scale = glm::scale(rotate, glm::vec3(5.0f, 5.0f, 5.0f));


    // {
    //     // A beautiful game
    //     CPUModel beautifulGameModel(ROOT_DIR "/Assets/Meshes/ABeautifulGame/ABeautifulGame.gltf", false, m_MaterialCache, m_TextureCache, m_GfxDevice);
    //     for (CPUMesh& mesh : beautifulGameModel.m_cpuMeshes)
    //     {
    //         GPUMeshId beautifulGameMeshId = m_MeshCache.add_mesh(m_GfxDevice, mesh);

    //         // glm::vec3 scale;
    //         // glm::quat rotation;
    //         // glm::vec3 translation;
    //         // glm::vec3 skew;
    //         // glm::vec4 perspective;
    //         // glm::decompose(mesh.m_transform, scale, rotation, translation, skew, perspective);

    //         // Transform t = {
    //         //     translation,
    //         //     rotation,
    //         //     scale * 5.0f
    //         // };

    //         // beautifulGameObject.set_transform(t.TransformMatrix());
    //         // beautifulGameObject.set_transform(mesh.m_transform);
    //         //beautifulGameObject.set_transform(glm::mat4(1.0f));
    //         // m_sceneRenderObjects.push_back(beautifulGameObject);
    //         m_sceneRenderObjects.emplace_back(beautifulGameMeshId, m_MeshCache, scale);
    //     }
    // }

    //  {
    //      // Orientation test model
    //      CPUModel orientationTestModel(ROOT_DIR "/Assets/Meshes/OrientationTest.glb", true, m_MaterialCache, m_TextureCache, m_GfxDevice);
    //      for (CPUMesh& mesh : orientationTestModel.m_cpuMeshes)
    //      {
    //         GPUMeshId orientationTestMeshId = m_MeshCache.add_mesh(m_GfxDevice, mesh);
    //         RenderObject orientationTestObject(defaultPipelineId, orientationTestMeshId, m_GraphicsPipelineCache, m_MeshCache);

    //         // glm::vec3 scale;
    //         // glm::quat rotation;
    //         // glm::vec3 translation;
    //         // glm::vec3 skew;
    //         // glm::vec4 perspective;
    //         // glm::decompose(mesh.m_transform, scale, rotation, translation, skew, perspective);

    //         // Transform t = {
    //         //     translation + glm::vec3(0.f, 10.0f, 0.0f),
    //         //     rotation,
    //         //     scale * 0.5f
    //         // };

    //         // orientationTestObject.set_transform(t.TransformMatrix());
    //         // orientationTestObject.set_transform(mesh.m_transform);
    //         // orientationTestObject.set_transform(glm::mat4(1.0f));
    //         orientationTestObject.set_transform(scale);
    //         m_sceneRenderObjects.push_back(orientationTestObject);
    //      }
    //  }
    
    // {
    //     // Suzanne mesh
    //     CPUModel suzanneModel(ROOT_DIR "/Assets/Meshes/suzanne.glb", true, m_TextureCache);
    //     GPUMeshId suzanneMeshId = m_MeshCache.add_mesh(m_GfxDevice, suzanneModel.m_cpuMesh);
    //     RenderObject suzanneObject(defaultPipelineId, suzanneMeshId, m_GraphicsPipelineCache, m_MeshCache);
    //     glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
    //     suzanneObject.set_transform(monkeyTranslate);
    //     m_sceneRenderObjects.push_back(suzanneObject);
    // }

    {
        // Helmet mesh
        CPUModel helmetModel(ROOT_DIR "/Assets/Meshes/DamagedHelmet.glb", true, m_MaterialCache, m_TextureCache, m_GfxDevice);
       

        for (CPUMesh& mesh : helmetModel.m_cpuMeshes)
        {
            GPUMeshId helmetMeshId = m_MeshCache.add_mesh(m_GfxDevice, mesh);

            glm::mat4 helmetTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 3.0f, 0.0f));
            helmetTransform = glm::rotate(helmetTransform, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));

            m_sceneRenderObjects.emplace_back(helmetMeshId, m_MeshCache, helmetTransform);
        }
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

void Renderer::init_render_targets() {
    // G Buffer
    // VkFormat albedoRTFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat albedoRTFormat = VK_FORMAT_B8G8R8A8_UNORM; // TODO: SRGB?
    VkImageCreateInfo albedoRTImage_ci = image_create_info(albedoRTFormat, VkExtent3D(WINDOW_WIDTH, WINDOW_HEIGHT, 1), 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT   // Output from gbuffer
        | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT // Input to deferred lighting
        // | VK_IMAGE_USAGE_SAMPLED_BIT       // TODO: Possibly to use for post processing stuff
        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,    // TODO: Copy from to swapchain
        VK_IMAGE_TYPE_2D
    );
    m_albedoRTId = m_TextureCache.add_render_target_texture(m_GfxDevice, albedoRTFormat, albedoRTImage_ci);

    VkFormat worldNormalsRTFormat = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    VkImageCreateInfo worldNormalsRTImage_ci = image_create_info(worldNormalsRTFormat, VkExtent3D(WINDOW_WIDTH, WINDOW_HEIGHT, 1), 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT   // Output from gbuffer
        | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, // Input to deferred lighting
        VK_IMAGE_TYPE_2D
    );
    m_worldNormalsRTId = m_TextureCache.add_render_target_texture(m_GfxDevice, worldNormalsRTFormat, worldNormalsRTImage_ci);

    VkFormat metallicRoughnessRTFormat = VK_FORMAT_R8G8_UNORM;
    VkImageCreateInfo metallicRoughnessRTImage_ci = image_create_info(metallicRoughnessRTFormat, VkExtent3D(WINDOW_WIDTH, WINDOW_HEIGHT, 1), 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT   // Output from gbuffer
        | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, // Input to deferred lighting
        VK_IMAGE_TYPE_2D
    );
    m_metallicRoughnessRTId = m_TextureCache.add_render_target_texture(m_GfxDevice, metallicRoughnessRTFormat, metallicRoughnessRTImage_ci);
}


void Renderer::init_render_stages() {
    VkFormat colorAttachmentFormats[3] = {
        m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.imageFormat,
        m_TextureCache.get_render_target_texture(m_worldNormalsRTId).allocatedImage.imageFormat,
        m_TextureCache.get_render_target_texture(m_metallicRoughnessRTId).allocatedImage.imageFormat
    };
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 3,
        .pColorAttachmentFormats = colorAttachmentFormats,
        .depthAttachmentFormat = m_GfxDevice.m_depthImage.imageFormat,
        .stencilAttachmentFormat = {}
    };

    // m_renderStages.push_back(std::make_unique<GBufferStage>(m_GfxDevice, m_GraphicsPipelineCache, &pipelineRenderingCI, std::span<VkDescriptorSetLayout const>(std::array<VkDescriptorSetLayout, 1>{m_bindlessDescriptorSetLayout})));
    m_pGbufferStage = std::make_unique<GBufferStage>(
        m_GfxDevice, m_GraphicsPipelineCache, &pipelineRenderingCI, 
        std::array<VkDescriptorSetLayout, 1>{m_bindlessDescriptorSetLayout}, // TODO: This smells
        m_bindlessDescriptorSets);
}

void Renderer::init_scene_data() {
    m_CPUSceneData.view = camera.get_view_matrix();
    glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
    projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
    m_CPUSceneData.projection = projection;
    m_CPUSceneData.cameraWorldPosition = camera.get_world_position();
    m_CPUSceneData.lightBufferAddress = 0; // TODO during update_scene_data()? or now
    m_CPUSceneData.numPointLights = static_cast<int>(m_CPUPointLights.size());
    m_CPUSceneData.directionalLight = m_directionalLight;

    // Material data only set once at the beginning, since for now we are loading all assets in ahead of time
    VkBufferDeviceAddressInfoKHR materialBufferAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
        .buffer = m_materialDataBuffer.buffer
    };
    m_CPUSceneData.materialBufferAddress = vkGetBufferDeviceAddress(m_GfxDevice, &materialBufferAddressInfo);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)  
    {
        upload_buffer(
            m_GPUSceneDataBuffers[i],
            sizeof(CPUSceneData),
            &m_CPUSceneData,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
            m_GfxDevice.m_vmaAllocator
        );
    }

    for (auto &sceneDataBuffer : m_GPUSceneDataBuffers)
    {
        VkBufferDeviceAddressInfoKHR sceneDataBufferAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
            .buffer = sceneDataBuffer.buffer
        };
        sceneDataBuffer.gpuAddress  = vkGetBufferDeviceAddress(m_GfxDevice, &sceneDataBufferAddressInfo);
    }
}

void Renderer::update_texture_descriptors() {

    // TODO: should batch things per frame?

    // Done like this instead of constructing temps in a for loop because of pImageInfo
    std::vector<VkDescriptorImageInfo> textureInfos;
    std::vector<VkWriteDescriptorSet> textureDescriptorWrites;
    textureInfos.resize(m_TextureCache.get_texture_count());
    textureDescriptorWrites.resize(m_TextureCache.get_texture_count());

    for (uint32_t i = 0; i < m_TextureCache.get_texture_count(); i++)
    {
        textureInfos[i].imageView = m_TextureCache.get_texture(i).allocatedImage.imageView;
        textureInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        textureDescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureDescriptorWrites[i].pNext = nullptr;
        textureDescriptorWrites[i].dstSet = m_bindlessDescriptorSets.at(0);
        textureDescriptorWrites[i].dstBinding = 1;
        textureDescriptorWrites[i].dstArrayElement = i;
        textureDescriptorWrites[i].descriptorCount = 1;
        textureDescriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureDescriptorWrites[i].pImageInfo = &textureInfos[i];
        textureDescriptorWrites[i].pBufferInfo = nullptr;
        textureDescriptorWrites[i].pTexelBufferView = nullptr;
    }
    
    vkUpdateDescriptorSets(m_GfxDevice, static_cast<uint32_t>(textureDescriptorWrites.size()), textureDescriptorWrites.data(), 0, nullptr);


    VkDescriptorImageInfo linearSamplerInfo = {
        .sampler = m_linearSampler
    };
    VkWriteDescriptorSet linearSamplerDescriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_bindlessDescriptorSets.at(0),
        .dstBinding = 0,
        .dstArrayElement = {},
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &linearSamplerInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_GfxDevice, 1, &linearSamplerDescriptorWrite, 0, nullptr);
}

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
        .ColorAttachmentFormat = m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.imageFormat
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
    if (m_pointLightsExist)
    {
        int lightCircleRadius = 5;
        float lightCircleSpeed = 0.02f;
        m_CPUPointLights[0].worldSpacePosition = glm::vec3(
            lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber),
            0.0,
            lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber)
        );
        m_CPUPointLights[1].worldSpacePosition = glm::vec3(
            lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber),
            1.0,
            lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber)
        );
        

        update_buffer(
            m_GPUPointLightsBuffers[frameInFlightIndex], 
            m_CPUPointLights.size() * sizeof(PointLight),
            m_CPUPointLights.data(),
            m_GfxDevice.m_vmaAllocator
        );
    }
}

void Renderer::update_scene_data(uint32_t frameInFlightIndex) {
    m_CPUSceneData.view = camera.get_view_matrix();
    glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
    projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
    m_CPUSceneData.projection = projection;
    m_CPUSceneData.cameraWorldPosition = camera.get_world_position();
    m_CPUSceneData.lightBufferAddress = m_GPUPointLightsBuffers[frameInFlightIndex].gpuAddress;
    m_CPUSceneData.numPointLights = static_cast<int>(m_CPUPointLights.size());
    m_CPUSceneData.directionalLight = m_directionalLight;

    update_buffer(
        m_GPUSceneDataBuffers[frameInFlightIndex], 
        sizeof(CPUSceneData),
        &m_CPUSceneData,
        m_GfxDevice.m_vmaAllocator
    );
}

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
            MRCERR("Failed to acquire image from Swap Chain!");
        }
        VkCommandBuffer cmdBuffer = m_GfxDevice.get_frame_command_buffer(m_currentFrame);
        vkResetCommandBuffer(cmdBuffer, {});

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        // Transition albedo and world normals RTs to color attachment
        {
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.image, 
                VK_ACCESS_NONE, 
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

            VkImageMemoryBarrier imb2 = image_memory_barrier(
                m_TextureCache.get_render_target_texture(m_worldNormalsRTId).allocatedImage.image, 
                VK_ACCESS_NONE, 
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
                1, &imb2
            );

            VkImageMemoryBarrier imb3 = image_memory_barrier(
                m_TextureCache.get_render_target_texture(m_metallicRoughnessRTId).allocatedImage.image, 
                VK_ACCESS_NONE, 
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
                1, &imb3
            );
        }

        VkRenderingAttachmentInfoKHR albedoAttachmentInfo = rendering_attachment_info(
            m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.imageView,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_COLOR
        );

        VkRenderingAttachmentInfoKHR worldNormalsAttachmentInfo = rendering_attachment_info(
            m_TextureCache.get_render_target_texture(m_worldNormalsRTId).allocatedImage.imageView,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_ZERO
        );

        VkRenderingAttachmentInfoKHR metallicRoughnessAttachmentInfo = rendering_attachment_info(
            m_TextureCache.get_render_target_texture(m_metallicRoughnessRTId).allocatedImage.imageView,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_ZERO
        );

        constexpr uint32_t colorAttachmentCount = 3;

        VkRenderingAttachmentInfoKHR colorAttachmentInfos[colorAttachmentCount] = { albedoAttachmentInfo, worldNormalsAttachmentInfo, metallicRoughnessAttachmentInfo };

        VkRenderingAttachmentInfoKHR depthAttachmentInfo  = rendering_attachment_info(
            m_GfxDevice.m_depthImage.imageView,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            &DEFAULT_CLEAR_VALUE_DEPTH
        );

        VkRenderingInfoKHR renderingInfo = rendering_info_fullscreen(
            colorAttachmentCount, colorAttachmentInfos, &depthAttachmentInfo
        );

        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdBeginRenderingKHR"));
        vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);

        m_pGbufferStage->Draw(cmdBuffer, m_GPUSceneDataBuffers[m_currentFrame].gpuAddress, m_sceneRenderObjects);

        PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_GfxDevice, "vkCmdEndRenderingKHR"));
        vkCmdEndRenderingKHR(cmdBuffer);

        // Draw imgui
        draw_imgui(m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.imageView);

        // Transition albedo image to copy src
        {
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.image, 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT, 
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            );
            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                {},
                0, nullptr,
                0, nullptr,
                1, &imb
            );
        }

        // Transition swapchain to copy dst
        {
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_GfxDevice.m_swapChainImages[imageIndex],
                VK_ACCESS_NONE,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );
            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                {},
                0, nullptr,
                0, nullptr,
                1, &imb
            );

        }

        // Copy albedo image to swapchain
        const VkImageCopy imageCopy= {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
            },
            .srcOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
            },
            .dstOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .extent = {
                .width = WINDOW_WIDTH,
                .height = WINDOW_HEIGHT,
                .depth = 1
            }
        };

        vkCmdCopyImage(
            cmdBuffer,
            m_TextureCache.get_render_target_texture(m_albedoRTId).allocatedImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_GfxDevice.m_swapChainImages[imageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopy
        );
        

        {
            // Transition swapchain to correct presentation layout
            VkImageMemoryBarrier imb = image_memory_barrier(
                m_GfxDevice.m_swapChainImages[imageIndex], 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                {},
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
                if (sdlEvent.key.keysym.sym == SDLK_BACKQUOTE) {
                    m_bInteractableUI = !m_bInteractableUI;
                    if (m_bInteractableUI) {
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
        // ImGui::ShowDemoWindow();
        // Create a window called "My First Tool", with a menu bar.
        ImGui::Begin("Rendering Menu", &m_bShowRenderingMenu, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
                if (ImGui::MenuItem("Close", "Ctrl+W"))  { m_bShowRenderingMenu = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::SliderFloat("Directional Light x", &m_directionalLight.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Directional Light y", &m_directionalLight.direction.y, -1.0f, 1.0f);
        ImGui::SliderFloat("Directional Light z", &m_directionalLight.direction.z, -1.0f, 1.0f);
        ImGui::SliderFloat("Directional Light power", &m_directionalLight.power,  0.0f, 1.0f);

        ImGui::SliderFloat("rx", &rx,  -1.0f, 1.0f);
        ImGui::SliderFloat("ry", &ry,  -1.0f, 1.0f);
        ImGui::SliderFloat("rz", &rz,  -1.0f, 1.0f);
        ImGui::SliderFloat("rm", &rm,  2.0f * -3.14f, 2.0f *3.14f);
        for (auto& renderObject : m_sceneRenderObjects)
        {
                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 2.0f, 2.0f));
                glm::mat4 rotate = glm::rotate(translate, rm, glm::vec3(rx, ry, rz));
                glm::mat4 scale = glm::scale(rotate, glm::vec3(5.0f, 5.0f, 5.0f));
                renderObject.m_transformMatrix = scale;
        }
        ImGui::End();
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

    if (m_pointLightsExist)
    {
        for (auto buffer: m_GPUPointLightsBuffers)
        {
            buffer.cleanup(m_GfxDevice.m_vmaAllocator);
        }
    }

    for (auto buffer: m_GPUSceneDataBuffers)
    {
        buffer.cleanup(m_GfxDevice.m_vmaAllocator);
    }

    // m_bindlessDescriptorAllocator.destroy_pool(m_GfxDevice);
    vkDestroyDescriptorPool(m_GfxDevice, m_bindlessPool, nullptr);

    vkDestroySampler(m_GfxDevice, m_linearSampler, nullptr);
    vkDestroySampler(m_GfxDevice, m_nearestSampler, nullptr);

    m_TextureCache.cleanup(m_GfxDevice);
    m_GraphicsPipelineCache.cleanup(m_GfxDevice);
    m_MeshCache.cleanup(m_GfxDevice);
    m_GfxDevice.cleanup();

    SDL_DestroyWindow(m_window);
}
