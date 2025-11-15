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

	struct GraphicsFrameData : public GraphicsFrameStatistics
	{
		// next frame
		Holder<Texture> targetTexture;

		// previous frames (microseconds, multiple frames ago)
		uint64 frameExecution = 0; // time spent processing the frame
		uint64 frameDuration = 0; // time elapsed start-to-start
	};

	class CAGE_ENGINE_API GraphicsDevice : private Immovable
	{
	public:
		void processEvents();

		void insertCommandBuffer(wgpu::CommandBuffer &&commands, const GraphicsFrameStatistics &statistics);
		void submitCommandBuffers();
		GraphicsFrameData nextFrame(Window *window);
		void wait(const wgpu::Future &future);

		Holder<wgpu::Device> nativeDeviceNoLock();
		Holder<wgpu::Device> nativeDevice(); // locks the queue for thread-safe access
		Holder<wgpu::Queue> nativeQueue(); // locks the queue for thread-safe access
	};

	struct CAGE_ENGINE_API GraphicsDeviceCreateConfig
	{
		Window *compatibility = nullptr;
		bool vsync = true;
	};

	CAGE_ENGINE_API Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config);
}

#endif
