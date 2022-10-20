#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-core/camera.h>
#include <cage-engine/inputs.h>

namespace cage
{
	class Texture;

	struct CAGE_ENGINE_API VirtualRealityProjection
	{
		Mat4 projection;
		Transform transform;
	};

	struct CAGE_ENGINE_API VirtualRealityCamera
	{
		PointerRange<const VirtualRealityProjection> projections; // contains multiple items for array or cube textures
		Vec2i resolution;
		Texture *colorTexture = nullptr;
		Texture *depthTexture = nullptr;
		mutable Real nearPlane = 0.3, farPlane = 10000; // fill in
	};

	struct CAGE_ENGINE_API VirtualRealityGraphicsFrame : private Immovable
	{
		PointerRange<const VirtualRealityCamera> cameras;

		// recalculate projection matrices with updated near/far planes
		void updateProjections();

		// acquire textures to render into
		// acquires no textures if rendering is unnecessary
		// requires opengl context
		void renderBegin();

		// submit textures to the device
		// requires opengl context
		void renderCommit();
	};

	class CAGE_ENGINE_API VirtualReality : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		Holder<VirtualRealityGraphicsFrame> graphicsFrame();
	};

	struct CAGE_ENGINE_API VirtualRealityCreateConfig
	{};

	// opengl context must be bound in the current thread (for both constructing and destroying the object)
	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config);
}

#endif
