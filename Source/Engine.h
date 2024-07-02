#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include <Rendering/GfxDevice.h>
#include <Mesh/MeshCache.h>
#include <Mesh/RenderObject.h>
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Pipeline/PipelineCache.h>
#include <Texture/TextureCache.h>
#include <Material/MaterialCache.h>
#include <Light/PointLight.h>
#include <Wrappers/Buffer.h>
#include <SceneData.h>
#include <Common/Config.h>

class SDL_window;

class Engine {
public:
    Engine();
    void run();
private:
    SDL_Window *m_window;
    GfxDevice m_GfxDevice;
    MeshCache m_MeshCache;
    GraphicsPipelineCache m_PipelineCache;
    MaterialCache m_MaterialCache;
    TextureCache m_TextureCache;

    VkDescriptorPool m_imguiPool;
    uint32_t m_currentFrame = 0;

    std::vector<RenderObject> m_sceneRenderObjects;

    // Lights
    std::vector<PointLight> m_CPUPointLights;
    std::vector<AllocatedBuffer> m_GPUPointLightsBuffers_F;

    // MaterialData
   AllocatedBuffer m_materialDataBuffer;

    // SceneData
    CPUSceneData m_CPUSceneData;
    std::vector<AllocatedBuffer> m_GPUSceneDataBuffers_F;

    // Descriptors
//    DescriptorAllocator m_globalDescriptorAllocator;
    // std::vector<VkDescriptorSetLayout> m_sceneDataDescriptorSetLayouts;
    // VkDescriptorSetLayout m_sceneDataDescriptorSetLayout;
    // std::vector<VkDescriptorSet> m_sceneDataDescriptorSets_F;
    // std::vector<DescriptorAllocator::DescriptorTypeCount> m_descriptorTypeCounts = {
    //         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT } // We want 1 buffer for each frame in flight
    // };

    void initWindow();
    void init_graphics();

    void init_lights();
    void init_scene_data();
    // void init_scene_descriptors();
    void init_assets();
    // void init_material_buffer();
    void init_imgui();
    
    void draw_objects();
    void draw_imgui(VkImageView targetImageView);
    void update_lights(uint32_t frameInFlightIndex);
    void update_scene_data(uint32_t frameInFlightIndex);
    // void update_scene_data_descriptors(uint32_t frameInFlightIndex);
    void drawFrame();
    void mainLoop();
    void cleanup();
};
