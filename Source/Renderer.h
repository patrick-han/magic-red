#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include <Rendering/GfxDevice.h>
#include <Mesh/MeshCache.h>
#include <Mesh/RenderObject.h>
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Texture/TextureCache.h>
#include <Material/MaterialCache.h>
#include <Light/PointLight.h>
#include <Light/DirectionalLight.h>
#include <Wrappers/Buffer.h>
#include <SceneData.h>
#include <Common/Config.h>

#include <Rendering/GBufferStage.h>
#include <Rendering/BlinnPhongLightingStage.h>

class SDL_window;

class Renderer {
public:
    Renderer() = default;
    void run();
private:
    SDL_Window *m_window;
    GfxDevice m_GfxDevice;
    MeshCache m_MeshCache;
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
    VkDescriptorSet m_bindlessDescriptorSet;

    VkDescriptorPool m_globalDescriptorPool;

    // Samplers
    VkSampler m_linearSampler;
    VkSampler m_nearestSampler;

    // Imgui
    bool m_bShowRenderingMenu = true;
    bool m_bInteractableUI = false;

    // RTs TODO:
    GPUTextureId m_albedoRTId{NULL_GPU_TEXTURE_ID};
    GPUTextureId m_worldNormalsRTId{NULL_GPU_TEXTURE_ID};
    GPUTextureId m_metallicRoughnessRTId{NULL_GPU_TEXTURE_ID};

    GPUTextureId m_lightingRTId{NULL_GPU_TEXTURE_ID};
    
    // std::vector<std::unique_ptr<StageBase>> m_pRenderStages;
    std::unique_ptr<GBufferStage> m_pGbufferStage;
    std::unique_ptr<BlinnPhongLightingStage> m_pLightingStage;

    float rx{1.0f};
    float ry{0.0f};
    float rz{0.0f};
    float rm{ 3.14f * 3.0f / 2.0f };

    void initWindow();
    void init_graphics();

    
    void init_lights();
    void create_samplers();
    void init_bindless_descriptors();
    void init_assets();
    void init_material_data();
    void init_scene_data();

    void init_global_descriptor_pool();

    void init_render_textures();
    void init_render_stages();

    void update_texture_descriptors();
    
    void init_imgui();
    
    void draw_imgui(VkImageView targetImageView);
    void update_lights(uint32_t frameInFlightIndex);
    void update_scene_data(uint32_t frameInFlightIndex);
    void drawFrame();
    void mainLoop();
    void cleanup();
};
