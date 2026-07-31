#ifndef guard_graphicsPipeline_aw56sdtf4k
#define guard_graphicsPipeline_aw56sdtf4k

#include <svector.h>

#include <cage-engine/gpuInterface.h>
#include <cage-engine/graphicsCommon.h>

namespace cage
{
	struct RenderPassConfig;
	struct DrawConfig;

	struct CAGE_ENGINE_API PipelineConfig : public GraphicsPipelineCommonConfig
	{
		ankerl::svector<gpu::BindGroupLayout, 3> bindingsLayouts;
		ankerl::svector<gpu::TextureFormatEnum, 1> colorTargets;
		gpu::VertexBufferLayout vertexBufferLayout;
		gpu::PrimitiveTopologyEnum primitiveTopology = gpu::PrimitiveTopologyEnum::Undefined;
		gpu::TextureFormatEnum depthFormat = gpu::TextureFormatEnum::Undefined;
		MeshComponentsFlags meshComponents = MeshComponentsFlags::None;

		bool operator==(const PipelineConfig &) const = default;
	};

	CAGE_ENGINE_API gpu::RenderPipeline newGraphicsPipeline(GraphicsDevice *device, const PipelineConfig &config);

	CAGE_ENGINE_API PipelineConfig convertPipelineConfig(const RenderPassConfig &pass, const DrawConfig &draw);
}

#endif
