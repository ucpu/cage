#ifndef guard_scene_qedf4gh8hj555j4k
#define guard_scene_qedf4gh8hj555j4k

#include "camera.h"

namespace cage
{
	struct CAGE_ENGINE_API TransformComponent : public Transform
	{
		using Transform::Transform;
		using Transform::operator =;
	};

	struct CAGE_ENGINE_API RenderComponent
	{
		Vec3 color = Vec3::Nan();
		Real intensity = Real::Nan();
		Real opacity = Real::Nan();
		uint32 object = 0;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API TextureAnimationComponent
	{
		uint64 startTime = 0;
		Real speed = Real::Nan();
		Real offset = Real::Nan(); // normalized 0..1
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		Real speed = Real::Nan();
		Real offset = Real::Nan(); // normalized 0..1
	};

	enum class LightTypeEnum : uint32
	{
		Directional,
		Point,
		Spot,
	};

	struct CAGE_ENGINE_API LightComponent
	{
		Vec3 attenuation = Vec3(0, 0, 1); // constant, linear, quadratic
		Vec3 color = Vec3(1);
		Real intensity = 1;
		Rads spotAngle = Degs(40);
		Real spotExponent = 80;
		uint32 sceneMask = 1;
		LightTypeEnum lightType = LightTypeEnum::Point;
	};

	struct CAGE_ENGINE_API ShadowmapComponent
	{
		// directional: width, height, depth
		// spot: near, far, unused
		Vec3 worldSize = Vec3();
		Real normalOffsetScale = 0.2;
		uint32 resolution = 256;
	};

	struct CAGE_ENGINE_API TextComponent
	{
		String value; // list of parameters separated by '|' when formatted, otherwise the string as is
		Vec3 color = Vec3(1);
		Real intensity = 1;
		// real opacity; // todo
		uint32 assetName = 0;
		uint32 textName = 0;
		uint32 font = 0;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API CameraComponent : public CameraProperties
	{
		Texture *target = nullptr;
		sint32 cameraOrder = 0;
	};

	struct CAGE_ENGINE_API SoundComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		uint32 sceneMask = 1;
		Real gain = 1; // linear amplitude multiplier
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		uint32 sceneMask = 1;
		Real rolloffFactor = 1; // distance multiplier
		Real gain = 1; // linear amplitude multiplier
	};
}

#endif // guard_scene_qedf4gh8hj555j4k
