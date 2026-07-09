#ifndef guard_graphicsDevice_erzgukm78ndf
#define guard_graphicsDevice_erzgukm78ndf

#include <cage-engine/gpuInterface.h>
#include <cage-engine/graphicsCommon.h>

namespace cage
{
	class Window;
	class Texture;

	class CAGE_ENGINE_API GraphicsDevice : private Immovable
	{
	public:
		void wait(const gpu::Future &future);
		void insertCommandBuffer(gpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics);
		Holder<Texture> nextWindow(Window *window);
		GraphicsFrameStatistics nextFrame();

		Holder<gpu::Device> nativeDevice(); // locks the device for thread-safe access
	};

	struct CAGE_ENGINE_API GraphicsDeviceCreateConfig
	{
		Window *compatibility = nullptr;
		bool vsync = true;
	};

	CAGE_ENGINE_API Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config);
}

#endif
