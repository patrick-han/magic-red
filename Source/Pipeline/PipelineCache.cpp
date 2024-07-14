#include "PipelineCache.h"

[[nodiscard]] GraphicsPipelineId GraphicsPipelineCache::add_pipeline([[maybe_unused]] const GfxDevice& gfxDevice, const GraphicsPipeline& pipeline) {
    const GraphicsPipelineId pipelineId = static_cast<uint32_t>(m_pipelines.size());
    m_pipelines.push_back(pipeline);
    return pipelineId;
}

[[nodiscard]] const GraphicsPipeline& GraphicsPipelineCache::get_pipeline(GraphicsPipelineId id) const {
    return m_pipelines[id];
}

void GraphicsPipelineCache::cleanup([[maybe_unused]] const GfxDevice& gfxDevice) {
    for (GraphicsPipeline& pipeline : m_pipelines)
    {
        pipeline.destroy();
    }
}
