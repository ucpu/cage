namespace cage
{
	struct ShaderConfig
	{
		uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES];

		ShaderConfig();
		void set(uint32 name, uint32 value);
	};

	struct Mat3x4
	{
		vec4 data[3];

		Mat3x4();
		explicit Mat3x4(const mat3 &in);
		explicit Mat3x4(const mat4 &in);
	};

	struct Objects
	{
		ShaderConfig shaderConfig;
		struct ShaderMesh
		{
			mat4 mvpMat;
			mat4 mvpPrevMat;
			Mat3x4 normalMat;
			Mat3x4 mMat;
			vec4 color;
			vec4 aniTexFrames;
		} *shaderMeshes;
		Mat3x4 *shaderArmatures;
		Texture *textures[MaxTexturesCountPerMaterial];
		Mesh *const mesh;
		Objects *next;
		uint32 count;
		const uint32 max;
		Objects(Mesh *mesh, uint32 max);
	};

	struct Lights
	{
		ShaderConfig shaderConfig;
		struct ShaderLight
		{
			mat4 shadowMat;
			mat4 mvpMat;
			vec4 color; // + angle
			vec4 position;
			vec4 direction;
			vec4 attenuation; // + exponent
		} *shaderLights;
		Lights *next;
		uint32 count;
		const uint32 max;
		const sint32 shadowmap;
		const LightTypeEnum lightType;
		Lights(LightTypeEnum lightType, sint32 shadowmap, uint32 max);
	};

	struct Translucent
	{
		Objects object;
		Lights *firstLight, *lastLight;
		Translucent *next;
		Translucent(Mesh *mesh);
	};

	struct Texts
	{
		struct Render
		{
			Font::FormatStruct format;
			mat4 transform;
			vec3 color;
			uint32 *glyphs;
			uint32 count;
			Render *next;
			Render();
		} *firtsRender, *lastRender;
		Font *font;
		Texts *next;
		Texts();
	};

	struct RenderPass : public CameraEffects
	{
		struct ShaderViewport
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 eyeDir;
			vec4 viewport;
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		} shaderViewport;
		Objects *firstOpaque, *lastOpaque;
		Lights *firstLight, *lastLight;
		Translucent *firstTranslucent, *lastTranslucent;
		Texts *firstText, *lastText;
		RenderPass *next;
		Texture *targetTexture;
		mat4 view;
		mat4 proj;
		mat4 viewProj;
		uintPtr entityId;
		sint32 targetShadowmap; // 0 = window (or texture); positive = 2d shadowmap; negative = cube shadowmap
		uint32 shadowmapResolution;
		uint32 clearFlags;
		uint32 vpX, vpY, vpW, vpH;
		RenderPass();
	};

	struct GraphicsDispatch
	{
		Mesh *meshSquare, *meshSphere, *meshCone;
		ShaderProgram *shaderVisualizeColor, *shaderVisualizeDepth, *shaderVisualizeMonochromatic, *shaderVisualizeVelocity;
		ShaderProgram *shaderAmbient, *shaderBlit, *shaderDepth, *shaderGBuffer, *shaderLighting, *shaderTranslucent;
		ShaderProgram *shaderGaussianBlur, *shaderSsaoGenerate, *shaderSsaoApply, *shaderMotionBlur, *shaderBloomGenerate, *shaderBloomApply, *shaderLuminanceCollection, *shaderLuminanceCopy, *shaderFinalScreen, *shaderFxaa;
		ShaderProgram *shaderFont;
		uint32 windowWidth, windowHeight;
		RenderPass *firstRenderPass, *lastRenderPass;
	};

	extern GraphicsDispatch *graphicsDispatch;
}
