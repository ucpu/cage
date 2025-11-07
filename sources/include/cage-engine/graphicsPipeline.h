#ifndef guard_graphicsPipeline_aw56sdtf4k
#define guard_graphicsPipeline_aw56sdtf4k

#include <svector.h>
#include <webgpu/webgpu_cpp.h>

#include <cage-engine/graphicsCommon.h>

namespace wgpu
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

namespace cage
{
	struct RenderPassConfig;
	struct DrawConfig;

	struct CAGE_ENGINE_API PipelineConfig : public GraphicsPipelineCommonConfig
	{
		ankerl::svector<wgpu::BindGroupLayout, 3> bindingsLayouts;
		ankerl::svector<wgpu::TextureFormat, 1> colorTargets;
		wgpu::VertexBufferLayout vertexBufferLayout = {};
		wgpu::PrimitiveTopology primitiveTopology = wgpu::PrimitiveTopology::Undefined;
		wgpu::TextureFormat depthFormat = wgpu::TextureFormat::Undefined;
		MeshComponentsFlags meshComponents = MeshComponentsFlags::None;

		bool operator==(const PipelineConfig &) const = default;
	};

	CAGE_ENGINE_API wgpu::RenderPipeline newGraphicsPipeline(GraphicsDevice *device, const PipelineConfig &config);

	CAGE_ENGINE_API PipelineConfig convertPipelineConfig(const RenderPassConfig &pass, const DrawConfig &draw);
}

#endif
