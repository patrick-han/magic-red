#pragma once

#include <span>
#include <vulkan/vulkan.h>

class GfxDevice;
struct RenderObject;

class StageBase {
public:
    StageBase() = delete;
    StageBase(const GfxDevice& _gfxDevice);
    virtual ~StageBase() = 0;

    virtual void Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects) = 0; // TODO: pass in draw command wrapper class

protected:
    const GfxDevice& m_gfxDevice;
};