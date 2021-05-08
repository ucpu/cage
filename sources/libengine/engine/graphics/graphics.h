#include <cage-engine/renderObject.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/font.h>

#include "../engine.h"

#include <vector>

namespace cage
{
	struct ShaderConfig
	{
		uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};

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
		struct UniModel
		{
			mat4 mvpMat;
			mat4 mvpPrevMat;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
			Mat3x4 mMat;
			vec4 color; // color rgb is linear, and NOT alpha-premultiplied
			vec4 aniTexFrames;
		};
		std::vector<UniModel> uniModeles;
		std::vector<Mat3x4> uniArmatures;
		Holder<Texture> textures[MaxTexturesCountPerMaterial];
		const Holder<Model> model;

		Objects(Holder<Model> model, uint32 max);
	};

	struct Lights
	{
		ShaderConfig shaderConfig;
		struct UniLight
		{
			mat4 shadowMat;
			mat4 mvpMat;
			vec4 color; // + spotAngle
			vec4 position;
			vec4 direction; // + normalOffsetScale
			vec4 attenuation; // + spotExponent
		};
		std::vector<UniLight> uniLights;
		const sint32 shadowmap;
		const LightTypeEnum lightType;

		Lights(LightTypeEnum lightType, sint32 shadowmap, uint32 max);
	};

	struct Translucent
	{
		Objects object;
		std::vector<Holder<Lights>> lights;

		Translucent(Holder<Model> model);
	};

	struct Texts
	{
		struct Render
		{
			std::vector<uint32> glyphs;
			FontFormat format;
			mat4 transform;
			vec3 color;
		};
		std::vector<Holder<Render>> renders;
		const Holder<Font> font;

		Texts(Holder<Font> font);
	};

	struct RenderPass : public CameraEffects
	{
		struct UniViewport
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 eyeDir;
			vec4 viewport;
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		} uniViewport;
		std::vector<Holder<Objects>> opaques;
		std::vector<Holder<Lights>> lights;
		std::vector<Holder<Translucent>> translucents;
		std::vector<Holder<Texts>> texts;
		Texture *targetTexture = nullptr;
		mat4 view;
		mat4 proj;
		mat4 viewProj;
		uintPtr entityId = 0;
		sint32 targetShadowmap = 0; // 0 = window (or texture); positive = 2d shadowmap; negative = cube shadowmap
		uint32 shadowmapResolution = 0;
		uint32 clearFlags = 0;
		uint32 vpX = 0, vpY = 0, vpW = 0, vpH = 0;
	};

	struct GraphicsDispatch
	{
		std::vector<Holder<RenderPass>> renderPasses;
		uint32 windowWidth = 0, windowHeight = 0;
	};

	extern GraphicsDispatch *graphicsDispatch;
}
