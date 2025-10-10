#ifndef guard_graphicsDevice_erzgukm78ndf
#define guard_graphicsDevice_erzgukm78ndf

#include <cage-engine/core.h>

namespace wgpu
{
	class Device;
	class Queue;
}

namespace cage
{
	class Window;
	class Texture;

	class CAGE_ENGINE_API GraphicsDevice : private Immovable
	{
	public:
		void processEvents();

		Holder<Texture> nextFrame(Window *window);

		wgpu::Device nativeDevice();
		wgpu::Queue nativeQueue();
	};

	struct CAGE_CORE_API GraphicsDeviceCreateConfig
	{
		Window *compatibility = nullptr;
	};

	CAGE_ENGINE_API Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config);
}

#endif
