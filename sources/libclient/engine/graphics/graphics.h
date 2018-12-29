namespace cage
{
	struct shaderConfigStruct
	{
		uint32 shaderRoutineStage[MaxRoutines];
		uint32 shaderRoutineName[MaxRoutines];
		uint32 shaderRoutineValue[MaxRoutines];
		shaderConfigStruct();
	};

	struct mat3x4
	{
		vec4 data[3];

		mat3x4(const mat3 &in);
		mat3x4(const mat4 &in);
	};

	struct objectsStruct
	{
		shaderConfigStruct shaderConfig;
		struct shaderMeshStruct
		{
			mat4 mvpMat;
			mat4 mvpPrevMat;
			mat3x4 normalMat;
			mat3x4 mMat;
			vec4 color;
			vec4 aniTexFrames;
		} *shaderMeshes;
		mat3x4 *shaderArmatures;
		textureClass *textures[MaxTexturesCountPerMaterial];
		meshClass *mesh;
		objectsStruct *next;
		uint32 count;
		const uint32 max;
		objectsStruct(meshClass *mesh, uint32 max);
	};

	struct lightsStruct
	{
		shaderConfigStruct shaderConfig;
		struct shaderLightStruct
		{
			mat4 shadowMat;
			mat4 mvpMat;
			vec4 color; // + angle
			vec4 position;
			vec4 direction;
			vec4 attenuation; // + exponent
		} *shaderLights;
		lightsStruct *next;
		uint32 count;
		const uint32 max;
		sint32 shadowmap;
		lightTypeEnum lightType;
		lightsStruct(lightTypeEnum lightType, sint32 shadowmap, uint32 max);
	};

	struct translucentStruct
	{
		objectsStruct object;
		lightsStruct *firstLight, *lastLight;
		translucentStruct *next;
		translucentStruct(meshClass *mesh);
	};

	struct renderPassStruct
	{
		struct shaderViewportStruct
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 viewport;
			vec4 ambientLight;
		} shaderViewport;
		objectsStruct *firstOpaque, *lastOpaque;
		lightsStruct *firstLighting, *lastLighting;
		translucentStruct *firstTranslucent, *lastTranslucent;
		renderPassStruct *next;
		textureClass *targetTexture;
		sint32 targetShadowmap; // 0 = window (or texture); positive = 2d shadowmap; negative = cube shadowmap
		uint32 shadowmapResolution;
		uint32 clearFlags;
		uint32 vpX, vpY, vpW, vpH;
		cameraEffectsFlags effects;
		renderPassStruct();
	};

	struct graphicsDispatchStruct
	{
		meshClass *meshSquare, *meshSphere, *meshCone, *meshFake;
		shaderClass *shaderBlitColor, *shaderBlitDepth, *shaderBlitVelocity;
		shaderClass *shaderDepth, *shaderGBuffer, *shaderLighting, *shaderTranslucent;
		shaderClass *shaderMotionBlur;
		uint32 windowWidth, windowHeight;
		renderPassStruct *firstRenderPass, *lastRenderPass;
	};

	extern graphicsDispatchStruct *graphicsDispatch;
}
