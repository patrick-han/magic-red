#pragma once

class GfxDevice;

class StageBase {
public:
    StageBase() = delete;
    StageBase(const GfxDevice& _gfxDevice);
    virtual ~StageBase() = 0;

protected:
    const GfxDevice& m_gfxDevice;
};