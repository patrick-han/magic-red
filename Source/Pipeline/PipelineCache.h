#pragma once
#include <Common/IdTypes.h>
#include <Pipeline/GraphicsPipeline.h>
#include <vector>

class GfxDevice;

class GraphicsPipelineCache
{
public:
    GraphicsPipelineCache() = default;
    ~GraphicsPipelineCache() = default;
    GraphicsPipelineCache(const GraphicsPipelineCache&) = delete;
    GraphicsPipelineCache& operator=(const GraphicsPipelineCache&) = delete;
    GraphicsPipelineCache(GraphicsPipelineCache&&) = delete;
    GraphicsPipelineCache& operator=(GraphicsPipelineCache&&) = delete;

    [[nodiscard]] GraphicsPipelineId add_pipeline([[maybe_unused]] const GfxDevice& gfxDevice, const GraphicsPipeline& pipeline);
    [[nodiscard]] const GraphicsPipeline& get_pipeline(GraphicsPipelineId id) const;
    void cleanup([[maybe_unused]] const GfxDevice& gfxDevice);

private:
    std::vector<GraphicsPipeline> m_pipelines;
};