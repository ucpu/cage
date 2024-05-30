#ifndef guard_scene_qedf4gh8hj555j4k
#define guard_scene_qedf4gh8hj555j4k

#include <cage-engine/core.h>

namespace cage
{
	class Texture;

	struct CAGE_ENGINE_API TransformComponent : public Transform
	{
		using Transform::Transform;
		using Transform::operator=;
	};

	struct CAGE_ENGINE_API RenderComponent
	{
		Vec3 color = Vec3::Nan(); // sRGB
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
		Vec4 attenuation = Vec4(1, 0, 1, 0); // constant, linear, quadratic, ~cubic~
		Vec3 color = Vec3(1); // sRGB
		Real intensity = 1;
		Rads spotAngle = Degs(40);
		Real spotExponent = 80;
		Real ssaoFactor = 0;
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
		Vec3 color = Vec3(1); // sRGB
		Real intensity = 1;
		// real opacity; // todo
		uint32 textId = 0;
		uint32 font = 0;
		uint32 sceneMask = 1;
		Real lineSpacing = 1;
		TextAlignEnum align = TextAlignEnum::Center;
	};

	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};

	struct CameraCommonProperties
	{
		Vec3 ambientColor = Vec3(); // sRGB
		Real ambientIntensity = 1;
		uint32 sceneMask = 1;
	};

	struct CAGE_ENGINE_API CameraComponent : public CameraCommonProperties
	{
		union CameraUnion
		{
			Vec2 orthographicSize;
			Rads perspectiveFov = Degs(60);
			CameraUnion() {}
		} camera;
		CameraTypeEnum cameraType = CameraTypeEnum::Perspective;
		Real near = 1, far = 100;

		Texture *target = nullptr;
	};

	struct CAGE_ENGINE_API SoundComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		uint32 sceneMask = 1;
		SoundAttenuationEnum attenuation = SoundAttenuationEnum::Logarithmic;
		Real minDistance = 1;
		Real maxDistance = 500;
		Real gain = 1; // linear amplitude multiplier
		bool loop = false;
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		uint32 sceneMask = 1;
		uint32 maxActiveVoices = 100;
		Real gain = 1; // linear amplitude multiplier
	};
}

#endif // guard_scene_qedf4gh8hj555j4k
