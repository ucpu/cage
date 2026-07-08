#ifndef guard_graphicsPipeline_aw56sdtf4k
#define guard_graphicsPipeline_aw56sdtf4k

#include <svector.h>

#include <cage-engine/gpuInterface.h>
#include <cage-engine/graphicsCommon.h>

namespace cage
{
	namespace gpu
	{
		// hack -> i dont care for this comparison, it is validated with the meshComponents
		inline bool operator==(const VertexBufferLayout &, const VertexBufferLayout &)
		{
			return true;
		}

		inline bool operator==(const BindGroupLayout &a, const BindGroupLayout &b)
		{
			return a.Get() == b.Get();
		}
	}

	struct RenderPassConfig;
	struct DrawConfig;

	struct CAGE_ENGINE_API PipelineConfig : public GraphicsPipelineCommonConfig
	{
		ankerl::svector<gpu::BindGroupLayout, 3> bindingsLayouts;
		ankerl::svector<gpu::TextureFormat, 1> colorTargets;
		gpu::VertexBufferLayout vertexBufferLayout = {};
		gpu::PrimitiveTopology primitiveTopology = gpu::PrimitiveTopology::Undefined;
		gpu::TextureFormat depthFormat = gpu::TextureFormat::Undefined;
		MeshComponentsFlags meshComponents = MeshComponentsFlags::None;

		bool operator==(const PipelineConfig &) const = default;
	};

	CAGE_ENGINE_API gpu::RenderPipeline newGraphicsPipeline(GraphicsDevice *device, const PipelineConfig &config);

	CAGE_ENGINE_API PipelineConfig convertPipelineConfig(const RenderPassConfig &pass, const DrawConfig &draw);
}

#endif
