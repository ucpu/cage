namespace cage
{
	struct CAGE_API cameraSsao
	{
		real worldRadius;
		real bias;
		real power;
		real strength;
		// ao = pow(ao - bias, power) * strength
		uint32 samplesCount;
		uint32 blurPasses;
		cameraSsao();
	};

	struct CAGE_API cameraMotionBlur
	{
		// todo
	};

	struct CAGE_API cameraBloom
	{
		uint32 blurPasses;
		real threshold;
		cameraBloom();
	};

	struct CAGE_API cameraEyeAdaptation
	{
		real key;
		real strength;
		real darkerSpeed;
		real lighterSpeed;
		cameraEyeAdaptation();
	};

	struct CAGE_API cameraTonemap
	{
		real shoulderStrength;
		real linearStrength;
		real linearAngle;
		real toeStrength;
		real toeNumerator;
		real toeDenominator;
		real white;
		cameraTonemap();
	};

	struct CAGE_API cameraEffects
	{
		cameraSsao ssao;
		cameraMotionBlur motionBlur;
		cameraBloom bloom;
		cameraEyeAdaptation eyeAdaptation;
		cameraTonemap tonemap;
		real gamma;
		cameraEffectsFlags effects;
		cameraEffects();
	};
}
