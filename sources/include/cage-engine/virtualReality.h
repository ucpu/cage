#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-core/camera.h>
#include <cage-engine/inputs.h>

namespace cage
{
	class Texture;

	class CAGE_ENGINE_API VirtualRealityController
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		bool connected() const;
		bool tracking() const;
		Transform aimPose() const;
		Transform gripPose() const;
		PointerRange<const Real> axes() const;
		PointerRange<const bool> buttons() const;
	};

	struct CAGE_ENGINE_API VirtualRealityCamera
	{
		Mat4 projection;
		Transform transform;
		Vec2i resolution;
		Texture *colorTexture = nullptr;
		Texture *depthTexture = nullptr;
		mutable Real nearPlane = 0.3, farPlane = 10000; // fill in
		bool primary = false; // primary cameras share same origin and should use optimized rendering
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

		VirtualRealityController &leftController();
		VirtualRealityController &rightController();

		Holder<VirtualRealityGraphicsFrame> graphicsFrame();
	};

	struct CAGE_ENGINE_API VirtualRealityCreateConfig
	{};

	// opengl context must be bound in the current thread (for both constructing and destroying the object)
	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config);
}

#endif
