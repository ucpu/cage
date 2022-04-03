#ifndef guard_camera_h_mnsbd5xfj
#define guard_camera_h_mnsbd5xfj

#include "screenSpaceEffectsProperties.h"

namespace cage
{
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

	enum class CameraTypeEnum : uint32
	{
		Perspective,
		Orthographic,
	};

	struct CAGE_ENGINE_API CameraProperties
	{
		union CameraUnion
		{
			Vec2 orthographicSize;
			Rads perspectiveFov = Degs(60);
			CameraUnion() {}
		} camera;

		Vec3 ambientColor = Vec3();
		Vec3 ambientDirectionalColor = Vec3(); // fake forward light affected by ssao
		Real ambientIntensity = 1;
		Real ambientDirectionalIntensity = 1;
		Real near = 1, far = 100;
		uint32 sceneMask = 1;
		CameraTypeEnum cameraType = CameraTypeEnum::Perspective;

		ScreenSpaceAmbientOcclusion ssao;
		ScreenSpaceBloom bloom;
		ScreenSpaceEyeAdaptation eyeAdaptation;
		ScreenSpaceTonemap tonemap;
		ScreenSpaceDepthOfField depthOfField;
		Real gamma = 2.2;
		CameraEffectsFlags effects = CameraEffectsFlags::None;
	};
}

#endif // guard_camera_h_mnsbd5xfj
