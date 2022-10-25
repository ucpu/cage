#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/inputs.h>

namespace cage
{
	namespace privat
	{
		CAGE_API_EXPORT Transform vrTransformByOrigin(EntityManager *ents, const Transform &in)
		{
			auto r = ents->component<VrOriginComponent>()->entities();
			if (!r.empty())
				return r[0]->value<TransformComponent>() * in;
			return in;
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
}
