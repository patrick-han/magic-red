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
#include <Light/DirectionalLight.h>
#include <Wrappers/Buffer.h>
#include <SceneData.h>
#include <Common/Config.h>

class SDL_window;

class Renderer {
public:
    Renderer() = default;
    void run();
private:
    SDL_Window *m_window;
    GfxDevice m_GfxDevice;
    MeshCache m_MeshCache;
    GraphicsPipelineCache m_GraphicsPipelineCache;
    MaterialCache m_MaterialCache;
    TextureCache m_TextureCache;

    VkDescriptorPool m_imguiPool;
    uint32_t m_currentFrame = 0;

    std::vector<RenderObject> m_sceneRenderObjects;

    // Lights
    std::vector<PointLight> m_CPUPointLights;
    std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> m_GPUPointLightsBuffers;
    bool m_pointLightsExist = false;
    DirectionalLight m_directionalLight;

    // MaterialData
    AllocatedBuffer m_materialDataBuffer;

    // SceneData
    CPUSceneData m_CPUSceneData;
    std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> m_GPUSceneDataBuffers;

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

    // Imgui
    bool m_bShowRenderingMenu = true;
    bool m_bInteractableUI = false;

    float rx{0.0f};
    float ry{1.0f};
    float rz{0.0f};
    float rm{ 0.0f };

    void initWindow();
    void init_graphics();

    void init_lights();
    void create_samplers();
    void init_bindless_descriptors();
    void init_assets();
    void init_material_data();
    void init_scene_data();

    void update_texture_descriptors();
    
    void init_imgui();
    
    void draw_objects();
    void draw_imgui(VkImageView targetImageView);
    void update_lights(uint32_t frameInFlightIndex);
    void update_scene_data(uint32_t frameInFlightIndex);
    void drawFrame();
    void mainLoop();
    void cleanup();
};
