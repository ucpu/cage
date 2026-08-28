#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-engine/inputs.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/virtualReality.h>

namespace cage
{
	Entity *virtualRealityFindOrigin(EntityManager *scene)
	{
		auto r = scene->component<VrOriginComponent>()->entities();
		if (r.size() != 1)
			CAGE_THROW_ERROR(Exception, "there must be exactly one entity with VrOriginComponent");
		return r[0];
	}

	Entity *virtualRealityFindCamera(EntityManager *scene)
	{
		auto r = scene->component<VrCameraComponent>()->entities();
		if (r.size() != 1)
			CAGE_THROW_ERROR(Exception, "there must be exactly one entity with VrCameraComponent");
		return r[0];
	}

	void virtualRealitySceneUpdate(EntityManager *scene)
	{
		Entity *origin = virtualRealityFindOrigin(scene);
		const Transform tr = origin->value<TransformComponent>() * origin->value<VrOriginComponent>().manualCorrection;
		{
			Entity *e = virtualRealityFindCamera(scene);
			e->value<TransformComponent>() = tr * e->value<VrCameraComponent>().virtualReality->pose();
		}
		{
			entitiesVisitor(
				[&](Entity *e, TransformComponent &t, VrControllerComponent &cc)
				{
					t = tr * cc.controller->gripPose();
					cc.aim = tr * cc.controller->aimPose();
				},
				scene, false);
		}
	}

	void virtualRealitySceneRecenter(EntityManager *scene, Real height, bool keepUp)
	{
		Entity *origin = virtualRealityFindOrigin(scene);
		VrOriginComponent &vc = origin->value<VrOriginComponent>();
		Transform headset = vc.virtualReality->pose();
		if (keepUp)
			headset.orientation = Quat(headset.orientation * Vec3(0, 0, -1), Vec3(0, 1, 0), true);
		headset.position += headset.orientation * Vec3(0, -height, 0);
		vc.manualCorrection = inverse(headset);
	}
}
