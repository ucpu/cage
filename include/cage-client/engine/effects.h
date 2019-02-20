namespace cage
{
	struct CAGE_API cameraSsaoStruct
	{
		real worldRadius;
		real blurRadius;
		real strength;
		real bias;
		real power;
		// ao = pow(ao * strength + bias, power)
		uint32 samplesCount;
		cameraSsaoStruct();
	};

	struct CAGE_API cameraMotionBlurStruct
	{
		// todo
	};

	struct CAGE_API cameraBloomStruct
	{
		uint32 blurPasses;
		real blurRadius;
		real threshold;
		cameraBloomStruct();
	};

	struct CAGE_API cameraEyeAdaptationStruct
	{
		real key;
		real strength;
		real darkerSpeed;
		real lighterSpeed;
		cameraEyeAdaptationStruct();
	};

	struct CAGE_API cameraTonemapStruct
	{
		real shoulderStrength;
		real linearStrength;
		real linearAngle;
		real toeStrength;
		real toeNumerator;
		real toeDenominator;
		real white;
		cameraTonemapStruct();
	};

	struct CAGE_API cameraEffectsStruct
	{
		cameraSsaoStruct ssao;
		cameraMotionBlurStruct motionBlur;
		cameraBloomStruct bloom;
		cameraEyeAdaptationStruct eyeAdaptation;
		cameraTonemapStruct tonemap;
		real gamma;
		cameraEffectsFlags effects;
		cameraEffectsStruct();
	};
}
