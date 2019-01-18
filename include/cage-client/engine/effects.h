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
		cameraSsaoStruct();
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
		cameraEyeAdaptationStruct eyeAdaptation;
		cameraTonemapStruct tonemap;
		real gamma;
		cameraEffectsFlags effects;
		cameraEffectsStruct();
	};
}
