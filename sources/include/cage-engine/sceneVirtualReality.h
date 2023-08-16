#ifndef guard_sceneVirtualReality_h_5es4tuhj
#define guard_sceneVirtualReality_h_5es4tuhj

#include <cage-engine/scene.h>

namespace cage
{
	class VirtualRealityController;
	class VirtualRealityCamera;
	class VirtualReality;

	// the transform of this entity maps the virtual reality coordinates space into the scene coordinates space
	struct CAGE_ENGINE_API VrOriginComponent
	{
		VirtualReality *virtualReality = nullptr;
		Transform manualCorrection;
	};

	// the transform of this entity is updated automatically by the virtual reality
	struct CAGE_ENGINE_API VrCameraComponent : public CameraCommonProperties
	{
		VirtualReality *virtualReality = nullptr;
		Real near = 0.2, far = 10000;
	};

	// the transform of this entity is updated automatically by the virtual reality
	// this entity represents the grip pose of the controller
	struct CAGE_ENGINE_API VrControllerComponent
	{
		VirtualRealityController *controller = nullptr;
		Transform aim; // aim pose of the controller in the scene coordinates space
	};
}

#endif // guard_sceneVirtualReality_h_5es4tuhj
