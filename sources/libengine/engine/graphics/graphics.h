namespace cage
{
	struct shaderConfigStruct
	{
		uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES];

		shaderConfigStruct();
		void set(uint32 name, uint32 value);
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
		Texture *textures[MaxTexturesCountPerMaterial];
		Mesh *const mesh;
		objectsStruct *next;
		uint32 count;
		const uint32 max;
		objectsStruct(Mesh *mesh, uint32 max);
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
		const sint32 shadowmap;
		const LightTypeEnum lightType;
		lightsStruct(LightTypeEnum lightType, sint32 shadowmap, uint32 max);
	};

	struct translucentStruct
	{
		objectsStruct object;
		lightsStruct *firstLight, *lastLight;
		translucentStruct *next;
		translucentStruct(Mesh *mesh);
	};

	struct textsStruct
	{
		struct renderStruct
		{
			Font::FormatStruct format;
			mat4 transform;
			vec3 color;
			uint32 *glyphs;
			uint32 count;
			renderStruct *next;
			renderStruct();
		} *firtsRender, *lastRender;
		Font *font;
		textsStruct *next;
		textsStruct();
	};

	struct renderPassStruct : public CameraEffects
	{
		struct shaderViewportStruct
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 eyeDir;
			vec4 viewport;
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		} shaderViewport;
		objectsStruct *firstOpaque, *lastOpaque;
		lightsStruct *firstLight, *lastLight;
		translucentStruct *firstTranslucent, *lastTranslucent;
		textsStruct *firstText, *lastText;
		renderPassStruct *next;
		Texture *targetTexture;
		mat4 view;
		mat4 proj;
		mat4 viewProj;
		uintPtr entityId;
		sint32 targetShadowmap; // 0 = window (or texture); positive = 2d shadowmap; negative = cube shadowmap
		uint32 shadowmapResolution;
		uint32 clearFlags;
		uint32 vpX, vpY, vpW, vpH;
		renderPassStruct();
	};

	struct graphicsDispatchStruct
	{
		Mesh *meshSquare, *meshSphere, *meshCone;
		ShaderProgram *shaderVisualizeColor, *shaderVisualizeDepth, *shaderVisualizeMonochromatic, *shaderVisualizeVelocity;
		ShaderProgram *shaderAmbient, *shaderBlit, *shaderDepth, *shaderGBuffer, *shaderLighting, *shaderTranslucent;
		ShaderProgram *shaderGaussianBlur, *shaderSsaoGenerate, *shaderSsaoApply, *shaderMotionBlur, *shaderBloomGenerate, *shaderBloomApply, *shaderLuminanceCollection, *shaderLuminanceCopy, *shaderFinalScreen, *shaderFxaa;
		ShaderProgram *shaderFont;
		uint32 windowWidth, windowHeight;
		renderPassStruct *firstRenderPass, *lastRenderPass;
	};

	extern graphicsDispatchStruct *graphicsDispatch;
}
