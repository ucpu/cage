#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-core/camera.h>
#include <cage-engine/inputs.h>

namespace cage
{
	class Texture;
	class EntityManager;

	class CAGE_ENGINE_API VirtualRealityController : private Immovable
	{
	public:
		bool tracking() const;
		Transform aimPose() const;
		Transform gripPose() const;
		PointerRange<const Real> axes() const; // thumbstick x, y, trackpad x, y, trigger, squeeze
		PointerRange<const bool> buttons() const; // a, b, x, y, thumbstick, trackpad, select, menu, trigger, squeeze
	};

	struct CAGE_ENGINE_API VirtualRealityCamera
	{
		Mat4 projection;
		Transform transform;
		Vec2i resolution;
		Texture *colorTexture = nullptr;
		Real nearPlane = 0.2, farPlane = 10000; // fill in and call updateProjections
		Rads verticalFov; // for lod selection
		bool primary = false;
	};

	class CAGE_ENGINE_API VirtualRealityGraphicsFrame : private Immovable
	{
	public:
		PointerRange<VirtualRealityCamera> cameras;

		// recalculate projection matrices with updated near/far planes
		void updateProjections();

		// headset pose irrespective of individual views
		Transform pose() const;

		// time elapsed since first frame
		// warning: this time is not related to the application time
		uint64 displayTime() const;

		// notify beginning of rendering the frame
		// requires opengl context
		void renderBegin();

		// acquires no textures if rendering is unnecessary
		// requires opengl context
		void acquireTextures();

		// notify finishing of rendering the frame
		// also submits acquired textures to the device
		// requires opengl context
		void renderCommit();

		// requires opengl context
		void renderCancel();
	};

	class CAGE_ENGINE_API VirtualReality : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		bool tracking() const;
		Transform pose() const;

		VirtualRealityController &leftController();
		VirtualRealityController &rightController();

		// waits for the virtual reality runtime to get ready for next frame
		Holder<VirtualRealityGraphicsFrame> graphicsFrame();

		uint64 targetFrameTiming() const;
	};

	// opengl context must be bound in the current thread (for both constructing and destroying the object)
	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality();

	CAGE_ENGINE_API void virtualRealitySceneUpdate(EntityManager *scene);
	CAGE_ENGINE_API void virtualRealitySceneRecenter(EntityManager *scene, Real height, bool keepUp = true);
}

#endif
