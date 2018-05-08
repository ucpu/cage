namespace cage
{
	struct shaderConfigStruct
	{
		uint32 shaderRoutineStage[MaxRoutines];
		uint32 shaderRoutineName[MaxRoutines];
		uint32 shaderRoutineValue[MaxRoutines];
		shaderConfigStruct();
	};

	struct objectsStruct
	{
		shaderConfigStruct shaderConfig;
		struct shaderMeshStruct
		{
			mat4 normalMat;
			mat4 mMat;
			mat4 mvpMat;
			vec4 color;
			vec4 aniTexFrames;
		} *shaderMeshes;
		struct shaderArmatureStruct
		{
			mat4 armature[CAGE_SHADER_MAX_ARMATURE_MATRICES];
		} *shaderArmatures;
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
		renderPassStruct();
	};

	struct graphicsDispatchStruct
	{
		meshClass *meshSquare, *meshSphere, *meshCone, *meshFake;
		shaderClass *shaderBlitColor, *shaderBlitDepth;
		shaderClass *shaderDepth, *shaderGBuffer, *shaderLighting, *shaderTranslucent;
		uint32 windowWidth, windowHeight;
		renderPassStruct *firstRenderPass, *lastRenderPass;
	};

	extern graphicsDispatchStruct *graphicsDispatch;
}