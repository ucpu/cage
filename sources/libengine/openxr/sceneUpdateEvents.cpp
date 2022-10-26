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
		Entity *findOrigin(EntityManager *ents)
		{
			auto r = ents->component<VrOriginComponent>()->entities();
			if (r.size() != 1)
				CAGE_THROW_ERROR(Exception, "there must be exactly one entity with VrOriginComponent");
			return r[0];
		}
	}

	namespace privat
	{
		CAGE_API_EXPORT Transform vrTransformByOrigin(EntityManager *ents, const Transform &in)
		{
			Entity *e = findOrigin(ents);
			const TransformComponent &t = e->value<TransformComponent>();
			const Transform &c = e->value<VrOriginComponent>().manualCorrection;
			return t * c * in;
		}
	}

	namespace
	{
		void headsetPose(EntityManager *ents, const InputHeadsetPose &in)
		{
			entitiesVisitor([&](Entity *e, TransformComponent &t, const VrCameraComponent &cc) {
				t = privat::vrTransformByOrigin(ents, in.pose);
			}, ents, false);
		}

		void controllerPose(EntityManager *ents, const InputControllerPose &in)
		{
			entitiesVisitor([&](Entity *e, TransformComponent &t, VrControllerComponent &cc) {
				if (cc.controller != in.controller)
					return;
				t = privat::vrTransformByOrigin(ents, in.pose);
				cc.aim = privat::vrTransformByOrigin(ents, in.controller->aimPose());
			}, ents, false);
		}
	}

	void virtualRealitySceneUpdate(EntityManager *ents, const GenericInput &gin)
	{
		switch (gin.type)
		{
		case InputClassEnum::HeadsetPose: return headsetPose(ents, gin.data.get<InputHeadsetPose>());
		case InputClassEnum::ControllerPose: return controllerPose(ents, gin.data.get<InputControllerPose>());
		}
	}

	void virtualRealitySceneRecenter(EntityManager *ents, Real height, bool keepUp)
	{
		Entity *e = findOrigin(ents);
		const TransformComponent &tr = e->value<TransformComponent>();
		VrOriginComponent &vc = e->value<VrOriginComponent>();
		Transform headset = vc.virtualReality->pose();
		if (keepUp)
			headset.orientation = Quat(headset.orientation * Vec3(0, 0, -1), Vec3(0, 1, 0), true);
		headset.position += headset.orientation * Vec3(0, -height, 0);
		vc.manualCorrection = inverse(headset);
	}
}
