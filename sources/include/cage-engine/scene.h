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

	// used with model/icon/text/light
	struct CAGE_ENGINE_API ColorComponent
	{
		Vec3 color = Vec3::Nan(); // sRGB
		Real intensity = Real::Nan();
		Real opacity = Real::Nan();
	};

	struct CAGE_ENGINE_API SceneComponent
	{
		uint32 sceneMask = 1;
	};

	// generic data passed to shader
	union CAGE_ENGINE_API ShaderDataComponent
	{
		Mat4 matrix;
		Vec4 data[4];
		ShaderDataComponent() : data() {}
	};

	// used by animations and sounds
	struct CAGE_ENGINE_API SpawnTimeComponent
	{
		uint64 spawnTime = 0; // automatically initialized by the engine
	};

	// used by texture arrays and skeletal animations
	struct CAGE_ENGINE_API AnimationSpeedComponent
	{
		Real speed = Real::Nan();
		Real offset = Real::Nan(); // normalized 0..1
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		uint32 animation = 0;
	};

	struct CAGE_ENGINE_API ModelComponent
	{
		uint32 model = 0;
		sint32 renderLayer = 0;
	};

	struct CAGE_ENGINE_API IconComponent
	{
		uint32 icon = 0;
		uint32 model = 0; // optional
		sint32 renderLayer = 0;
	};

	struct CAGE_ENGINE_API TextComponent
	{
		uint32 textId = 0;
		uint32 font = 0;
		TextAlignEnum align = TextAlignEnum::Center;
		Real lineSpacing = 1;
		sint32 renderLayer = 0;
	};

	// list of parameters separated by '|' when formatted, otherwise the string as is
	struct CAGE_ENGINE_API TextValueComponent
	{
		String value;
		inline TextValueComponent &operator=(const auto &v) requires requires { String(v); }
		{
			value = v;
			return *this;
		}
	};

	struct CAGE_ENGINE_API LightComponent
	{
		Rads spotAngle = Degs(40);
		Real spotExponent = 80;
		Real ssaoFactor = 0;
		LightTypeEnum lightType = LightTypeEnum::Point;
		LightAttenuationEnum attenuation = LightAttenuationEnum::Logarithmic;
		Real minDistance = 1;
		Real maxDistance = 100;
		sint32 priority = 0;
	};

	struct CAGE_ENGINE_API ShadowmapComponent
	{
		uint32 resolution = 1024;
		Real shadowFactor = 1;
		Real normalOffsetScale = 0.2;
		Real cascadesPaddingDistance = 20; // distance by which the bounding box is expanded to avoid excluding shadow casters in front of it
		Real cascadesSplitLogFactor = 0.5; // fraction of log vs uniform scales used for depth splits for cascades
		uint32 cascadesCount = 4;
	};

	struct CAGE_ENGINE_API CameraCommonProperties
	{
		Vec3 ambientColor = Vec3(0); // sRGB
		Real ambientIntensity = 1;
		Vec3 skyColor = Vec3(0); // sRGB
		Real skyIntensity = 1;
		uint32 maxLights = 100;
		Real shadowmapFrustumDepthFraction = 1; // maximum depth (linear) of the frustum that will be covered by directional light shadowmaps
	};

	struct CAGE_ENGINE_API CameraComponent : public CameraCommonProperties
	{
		Vec2 orthographicSize;
		Texture *target = nullptr;
		Rads perspectiveFov = Degs(60);
		CameraTypeEnum cameraType = CameraTypeEnum::Perspective;
		Real near = 0.1, far = 1000;
	};

	struct CAGE_ENGINE_API SoundComponent
	{
		uint32 sound = 0;
		SoundAttenuationEnum attenuation = SoundAttenuationEnum::Logarithmic;
		Real minDistance = 1;
		Real maxDistance = 500;
		Real gain = 1; // linear amplitude multiplier
		sint32 priority = 0;
		bool loop = false;
	};

	struct CAGE_ENGINE_API ListenerComponent
	{
		uint32 maxSounds = 100;
		Real maxGainThreshold = Real::Infinity(); // all sounds will have proportionally reduced gain when the sum of effective gains reaches this threshold
		Real gain = 1; // linear amplitude multiplier
	};
}

#endif // guard_scene_qedf4gh8hj555j4k
