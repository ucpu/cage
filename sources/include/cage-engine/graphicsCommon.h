#ifndef guard_graphicsCommon_sdr5yx4tgftzu
#define guard_graphicsCommon_sdr5yx4tgftzu

#include <cage-engine/core.h>

namespace cage
{
	class GraphicsDevice;
	class GraphicsBuffer;
	class GraphicsEncoder;
	class Model;
	class Shader;
	class Texture;

	struct CAGE_ENGINE_API GraphicsPipelineCommonConfig
	{
		Shader *shader = nullptr;
		BlendingEnum blending = BlendingEnum::None;
		DepthTestEnum depthTest = DepthTestEnum::Less;
		bool depthWrite = true;
		bool backFaceCulling = true;

		bool operator==(const GraphicsPipelineCommonConfig &) const = default;
	};

	struct CAGE_ENGINE_API GraphicsFrameStatistics
	{
		uint32 passes = 0;
		uint32 pipelineSwitches = 0;
		uint32 drawCalls = 0;
		uint32 primitives = 0;
	};

	namespace privat
	{
		template<class T>
		concept GraphicsBufferWritable = std::is_trivially_copyable_v<T> && !std::is_pointer_v<T> && !requires(T t) {
			t.begin();
			t.end();
		};
	}
}

#endif
