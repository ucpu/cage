namespace cage
{
	enum class CameraEffectsFlags : uint32
	{
		None = 0,
		AmbientOcclusion = 1 << 0,
		MotionBlur = 1 << 1,
		Bloom = 1 << 2,
		EyeAdaptation = 1 << 3,
		ToneMapping = 1 << 4,
		GammaCorrection = 1 << 5,
		AntiAliasing = 1 << 6,
		GeometryPass = AmbientOcclusion | MotionBlur,
		ScreenPass = Bloom | ToneMapping | GammaCorrection | AntiAliasing,
		CombinedPass = GeometryPass | ScreenPass,
	};

	struct CAGE_API CameraSsao
	{
		real worldRadius;
		real bias;
		real power;
		real strength;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount;
		uint32 blurPasses;
		CameraSsao();
	};

	struct CAGE_API CameraMotionBlur
	{
		// todo
	};

	struct CAGE_API CameraBloom
	{
		uint32 blurPasses;
		real threshold;
		CameraBloom();
	};

	struct CAGE_API CameraEyeAdaptation
	{
		real key;
		real strength;
		real darkerSpeed;
		real lighterSpeed;
		CameraEyeAdaptation();
	};

	struct CAGE_API CameraTonemap
	{
		real shoulderStrength;
		real linearStrength;
		real linearAngle;
		real toeStrength;
		real toeNumerator;
		real toeDenominator;
		real white;
		CameraTonemap();
	};

	struct CAGE_API CameraEffects
	{
		CameraSsao ssao;
		CameraMotionBlur motionBlur;
		CameraBloom bloom;
		CameraEyeAdaptation eyeAdaptation;
		CameraTonemap tonemap;
		real gamma;
		CameraEffectsFlags effects;
		CameraEffects();
	};
}
