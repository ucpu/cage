#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-core/camera.h>
#include <cage-engine/inputs.h>

namespace cage
{
	class Texture;

	struct VirtualRealityProjection
	{
		Mat4 projection;
		Transform transform;
	};

	struct VirtualRealityView
	{
		PointerRange<const VirtualRealityProjection> projections; // contains multiple items for array or cube textures
		Vec2i resolution;
		Texture *colorTexture = nullptr;
		Texture *depthTexture = nullptr;
		mutable Real nearPlane = 1, farPlane = 100; // fill in
	};

	struct VirtualRealityGraphicsFrame
	{
		PointerRange<const VirtualRealityView> views;
	};

	class CAGE_ENGINE_API VirtualReality : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		const VirtualRealityGraphicsFrame &graphicsFrame();
	};

	struct CAGE_ENGINE_API VirtualRealityCreateConfig
	{};

	// opengl context must be bound in the current thread (for both constructing and destroying the object)
	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config);
}

#endif
