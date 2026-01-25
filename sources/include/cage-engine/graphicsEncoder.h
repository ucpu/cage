#ifndef guard_graphicsEncoder_erdzr5dzwsqw
#define guard_graphicsEncoder_erdzr5dzwsqw

#include <optional>

#include <svector.h>

#include <cage-engine/graphicsBindings.h>

namespace cage
{
	struct CAGE_ENGINE_API RenderPassConfig
	{
		struct ColorTargetConfig
		{
			Texture *texture = nullptr;
			Vec4 clearValue = Vec4(0, 0, 0, 1);
			bool clear = true;
		};

		struct DepthTargetConfig
		{
			Texture *texture = nullptr;
			bool clear = true;
		};

		ankerl::svector<ColorTargetConfig, 1> colorTargets;
		std::optional<DepthTargetConfig> depthTarget;
		GraphicsBindings bindings; // binds to set = 0
	};

	struct CAGE_ENGINE_API DrawConfig : public GraphicsPipelineCommonConfig
	{
		GraphicsBindings material; // binds to set = 1 (optional, must match layout for the model)
		GraphicsBindings bindings; // binds to set = 2
		ankerl::svector<uint32, 5> dynamicOffsets = {}; // applies to buffers in set = 2
		const Model *model = nullptr;
		uint32 instances = 1;
	};

	namespace detail
	{
		struct CAGE_ENGINE_API RenderEncoderNamedScope : private Immovable
		{
			[[nodiscard]] RenderEncoderNamedScope(GraphicsEncoder *queue, StringPointer name);
			~RenderEncoderNamedScope();

		private:
			GraphicsEncoder *queue = nullptr;
		};
	}

	class CAGE_ENGINE_API GraphicsEncoder : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		void nextPass(const RenderPassConfig &config);

		[[nodiscard]] detail::RenderEncoderNamedScope namedScope(StringPointer name);
		void scissors(Vec2i origin, Vec2i size);
		void draw(const DrawConfig &config);

		void submit();

		GraphicsDevice *getDevice() const;
		const RenderPassConfig &getCurrentPass() const;

		const wgpu::CommandEncoder &nativeCommandEncoder();
		const wgpu::RenderPassEncoder &nativeRenderEncoder();
	};

	CAGE_ENGINE_API Holder<GraphicsEncoder> newGraphicsEncoder(GraphicsDevice *device, const AssetLabel &label);
}

#endif
