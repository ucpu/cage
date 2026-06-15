#ifndef guard_graphicsDevice_erzgukm78ndf
#define guard_graphicsDevice_erzgukm78ndf

#include <cage-engine/graphicsCommon.h>

namespace wgpu
{
	class Device;
	class Queue;
	class CommandBuffer;
	struct Future;
}

namespace cage
{
	class Window;
	class Texture;

	class CAGE_ENGINE_API GraphicsDevice : private Immovable
	{
	public:
		void wait(const wgpu::Future &future);
		void insertCommandBuffer(wgpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics);
		Holder<Texture> nextWindow(Window *window);
		GraphicsFrameStatistics nextFrame();

		Holder<wgpu::Device> nativeDevice(); // locks the device for thread-safe access
		Holder<wgpu::Queue> nativeQueue(); // locks the device for thread-safe access
	};

	struct CAGE_ENGINE_API GraphicsDeviceCreateConfig
	{
		Window *compatibility = nullptr;
		bool vsync = true;
	};

	CAGE_ENGINE_API Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config);
}

#endif
