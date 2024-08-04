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
    virtual void Cleanup() = 0;

protected:
    const GfxDevice& m_gfxDevice;
};