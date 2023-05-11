#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/inputs.h>

namespace cage
{
	namespace
	{
		Entity *findOrigin(EntityManager *scene)
		{
			auto r = scene->component<VrOriginComponent>()->entities();
			if (r.size() != 1)
				CAGE_THROW_ERROR(Exception, "there must be exactly one entity with VrOriginComponent");
			return r[0];
		}
	}

	void virtualRealitySceneUpdate(EntityManager *scene)
	{
		Entity *origin = findOrigin(scene);
		const Transform tr = origin->value<TransformComponent>() * origin->value<VrOriginComponent>().manualCorrection;
		entitiesVisitor([&](Entity *e, TransformComponent &t, const VrCameraComponent &cc) {
			t = tr * cc.virtualReality->pose();
		}, scene, false);
		entitiesVisitor([&](Entity *e, TransformComponent &t, VrControllerComponent &cc) {
			t = tr * cc.controller->gripPose();
			cc.aim = tr * cc.controller->aimPose();
		}, scene, false);
	}

	void virtualRealitySceneRecenter(EntityManager *scene, Real height, bool keepUp)
	{
		Entity *origin = findOrigin(scene);
		const TransformComponent &tr = origin->value<TransformComponent>();
		VrOriginComponent &vc = origin->value<VrOriginComponent>();
		Transform headset = vc.virtualReality->pose();
		if (keepUp)
			headset.orientation = Quat(headset.orientation * Vec3(0, 0, -1), Vec3(0, 1, 0), true);
		headset.position += headset.orientation * Vec3(0, -height, 0);
		vc.manualCorrection = inverse(headset);
	}
}
