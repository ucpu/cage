#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-engine/inputs.h>

namespace cage
{
	class Texture;

	struct VirtualRealityGraphicsFrame
	{
		Texture *colorTexture = nullptr;
		Texture *depthTexture = nullptr;
		Vec2i resolution;
		Real nearPlane = 1, farPlane = 100;
	};

	class CAGE_ENGINE_API VirtualReality : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		void graphicsFrame(VirtualRealityGraphicsFrame &frame);
	};

	struct CAGE_CORE_API VirtualRealityCreateConfig
	{};

	// opengl context must be bound in the current thread
	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config);
}

#endif
