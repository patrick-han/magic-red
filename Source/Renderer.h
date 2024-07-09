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

class Renderer {
public:
    Renderer();
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
    VkDescriptorPool m_bindlessPool;
    VkDescriptorSetLayout m_bindlessDescriptorSetLayout;
    // DescriptorAllocator m_bindlessDescriptorAllocator;
    // std::vector<VkDescriptorSetLayout> m_sceneDataDescriptorSetLayouts;
    // std::vector<VkDescriptorSet> m_sceneDataDescriptorSets_F;
    VkDescriptorSet m_bindlessDescriptorSet;

    // Samplers
    VkSampler m_linearSampler;
    VkSampler m_nearestSampler;

    void initWindow();
    void init_graphics();

    void init_lights();
    void create_samplers();
    void init_texture_descriptors();
    void init_assets();
    void init_material_data();
    void init_scene_data();

    void update_texture_descriptors();
    
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
