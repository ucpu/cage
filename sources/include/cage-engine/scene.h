#ifndef guard_scene_qedf4gh8hj555j4k
#define guard_scene_qedf4gh8hj555j4k

#include "screenSpaceEffectsProperties.h"

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
		Real offset = Real::Nan();
	};

	struct CAGE_ENGINE_API SkeletalAnimationComponent
	{
		uint64 startTime = 0;
		uint32 name = 0;
		Real speed = Real::Nan();
		Real offset = Real::Nan();
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

	enum class CameraEffectsFlags : uint32
	{
		None = 0,
		AmbientOcclusion = 1 << 0,
		DepthOfField = 1 << 1,
		Bloom = 1 << 3,
		EyeAdaptation = 1 << 4,
		ToneMapping = 1 << 5,
		GammaCorrection = 1 << 6,
		AntiAliasing = 1 << 7,
		Default = AmbientOcclusion | Bloom | ToneMapping | GammaCorrection | AntiAliasing,
	};

	struct CAGE_ENGINE_API CameraEffects
	{
		ScreenSpaceAmbientOcclusion ssao;
		ScreenSpaceBloom bloom;
		ScreenSpaceEyeAdaptation eyeAdaptation;
		ScreenSpaceTonemap tonemap;
		ScreenSpaceDepthOfField depthOfField;
		Real gamma = 2.2;
		CameraEffectsFlags effects = CameraEffectsFlags::None;
	};

	enum class CameraClearFlags : uint32
	{
		None = 0,
		Depth = 1 << 0,
		Color = 1 << 1,
		Stencil = 1 << 2,
	};

	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};

	struct CAGE_ENGINE_API CameraComponent : public CameraEffects
	{
		Vec3 ambientColor = Vec3();
		Vec3 ambientDirectionalColor = Vec3(); // fake forward light affected by ssao
		union CameraUnion
		{
			Vec2 orthographicSize;
			Rads perspectiveFov = Degs(60);
			CameraUnion() {}
		} camera;
		Texture *target = nullptr;
		Real ambientIntensity = 1;
		Real ambientDirectionalIntensity = 1;
		Real near = 1, far = 100;
		Real zeroParallaxDistance = 10;
		Real eyeSeparation = 0.3;
		sint32 cameraOrder = 0;
		uint32 sceneMask = 1;
		CameraClearFlags clear = CameraClearFlags::Depth | CameraClearFlags::Color;
		CameraTypeEnum cameraType = CameraTypeEnum::Perspective;
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
