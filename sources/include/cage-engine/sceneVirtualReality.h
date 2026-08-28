#ifndef guard_sceneVirtualReality_h_5es4tuhj
#define guard_sceneVirtualReality_h_5es4tuhj

#include <cage-engine/core.h>

namespace cage
{
	class VirtualRealityController;
	class VirtualReality;

	// this entity represents player's chair/room in the scene world
	struct CAGE_ENGINE_API VrOriginComponent
	{
		VirtualReality *virtualReality = nullptr;
		Transform manualCorrection;
	};

	// the transform of this entity is updated by virtualRealitySceneUpdate
	struct CAGE_ENGINE_API VrCameraComponent
	{
		VirtualReality *virtualReality = nullptr;
	};

	// the transform of this entity is updated by virtualRealitySceneUpdate
	// this entity represents the grip pose of the controller
	struct CAGE_ENGINE_API VrControllerComponent
	{
		VirtualRealityController *controller = nullptr;
		Transform aim; // aim pose of the controller in the scene coordinates space
	};
}

#endif // guard_sceneVirtualReality_h_5es4tuhj
