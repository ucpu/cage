#ifndef guard_lodSelection_h_dsthgiztr8
#define guard_lodSelection_h_dsthgiztr8

#include <vector>

#include <cage-engine/core.h>

namespace cage
{
	struct CameraComponent;
	class RenderObject;
	class Model;
	class AssetsOnDemand;

	struct CAGE_ENGINE_API LodSelection
	{
		Vec3 center = Vec3::Nan(); // center of camera
		Real screenSize = -1; // vertical size of screen in pixels, one meter in front of the camera
		bool orthographic = false;

		LodSelection() = default;
		LodSelection(Vec3 center, const CameraComponent &cam, sint32 screenHeightPx);

		uint32 selectLod(const Vec3 position, const RenderObject *object) const;
		void selectModels(std::vector<Holder<Model>> &outModels, const Vec3 position, const RenderObject *object, AssetsOnDemand *assets) const;
		void selectModels(std::vector<Holder<Model>> &outModels, const Vec3 position, const RenderObject *object, const AssetsManager *assets) const;
	};
}

#endif // guard_lodSelection_h_dsthgiztr8
