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

		Transform transformByOrigin(EntityManager *scene, const Transform &in)
		{
			Entity *e = findOrigin(scene);
			const TransformComponent &t = e->value<TransformComponent>();
			const Transform &c = e->value<VrOriginComponent>().manualCorrection;
			return t * c * in;
		}

		void headsetPose(EntityManager *scene, const InputHeadsetPose &in)
		{
			entitiesVisitor([&](Entity *e, TransformComponent &t, const VrCameraComponent &cc) {
				t = transformByOrigin(scene, in.pose);
			}, scene, false);
		}

		void controllerPose(EntityManager *scene, const InputControllerPose &in)
		{
			entitiesVisitor([&](Entity *e, TransformComponent &t, VrControllerComponent &cc) {
				if (cc.controller != in.controller)
					return;
				t = transformByOrigin(scene, in.pose);
				cc.aim = transformByOrigin(scene, in.controller->aimPose());
			}, scene, false);
		}
	}

	void virtualRealitySceneUpdate(EntityManager *scene, const GenericInput &in)
	{
		switch (in.type)
		{
		case InputClassEnum::HeadsetPose: return headsetPose(scene, in.data.get<InputHeadsetPose>());
		case InputClassEnum::ControllerPose: return controllerPose(scene, in.data.get<InputControllerPose>());
		}
	}

	void virtualRealitySceneRecenter(EntityManager *scene, Real height, bool keepUp)
	{
		Entity *e = findOrigin(scene);
		const TransformComponent &tr = e->value<TransformComponent>();
		VrOriginComponent &vc = e->value<VrOriginComponent>();
		Transform headset = vc.virtualReality->pose();
		if (keepUp)
			headset.orientation = Quat(headset.orientation * Vec3(0, 0, -1), Vec3(0, 1, 0), true);
		headset.position += headset.orientation * Vec3(0, -height, 0);
		vc.manualCorrection = inverse(headset);
	}
}
