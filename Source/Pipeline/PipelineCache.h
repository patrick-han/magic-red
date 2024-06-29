#pragma once
#include <Common/IdTypes.h>
#include <Pipeline/GraphicsPipeline.h>
#include <vector>

class GfxDevice;

class GraphicsPipelineCache
{
public:
    [[nodiscard]] GraphicsPipelineId add_pipeline([[maybe_unused]] const GfxDevice& gfxDevice, const GraphicsPipeline& pipeline);
    [[nodiscard]] const GraphicsPipeline& get_pipeline(GraphicsPipelineId id) const;
    void cleanup([[maybe_unused]] const GfxDevice& gfxDevice);

private:
    std::vector<GraphicsPipeline> m_pipelines;
};