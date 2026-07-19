#ifndef guard_graphicsDevice_erzgukm78ndf
#define guard_graphicsDevice_erzgukm78ndf

#include <cage-engine/gpuInterface.h>
#include <cage-engine/graphicsCommon.h>

namespace cage
{
	class Window;
	class Texture;

	struct CAGE_ENGINE_API GraphicsWindowPresentation
	{
		// in
		Window *window = nullptr;
		// out
		Holder<Texture> texture;
	};

	class CAGE_ENGINE_API GraphicsDevice : private Immovable
	{
	public:
		Holder<gpu::Device> nativeDevice(); // locks the device for thread-safe access
		void insertCommandBuffer(gpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics);
		GraphicsFrameStatistics nextFrame(PointerRange<GraphicsWindowPresentation> windows);
	};

	struct CAGE_ENGINE_API GraphicsDeviceCreateConfig
	{
		Window *compatibility = nullptr;
		bool vsync = true;
	};

	CAGE_ENGINE_API Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config);
}

#endif
