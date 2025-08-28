#include <cage-core/assetsManager.h>
#include <cage-core/hashString.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>
#include <cage-engine/uniformBuffer.h>

namespace cage
{
	namespace
	{
		TextureHandle provTex(ProvisionalGraphics *prov, const String &prefix, Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat)
		{
			const String name = Stringizer() + prefix + "_" + resolution + "_" + mipmapLevels + "_" + internalFormat;
			TextureHandle tex = prov->texture(name,
				[resolution, mipmapLevels, internalFormat](Texture *tex)
				{
					tex->initialize(resolution, mipmapLevels, internalFormat);
					tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					if (mipmapLevels > 1)
						tex->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
					else
						tex->filters(GL_LINEAR, GL_LINEAR, 0);
				});
			return tex;
		}

		struct GfGaussianBlurConfig : public ScreenSpaceCommonConfig
		{
			TextureHandle texture;
			uint32 internalFormat = 0;
			uint32 mipmapLevel = 0;
			uint32 mipmapsCount = 1;
		};

		void gfGaussianBlur(const GfGaussianBlurConfig &config)
		{
			RenderQueue *q = config.queue;
			const auto graphicsDebugScope = q->namedScope("blur");

			const Vec2i res = max(config.resolution / numeric_cast<uint32>(cage::pow2(config.mipmapLevel)), 1);
			q->viewport(Vec2i(), res);
			FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
			q->bind(fb);
			Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/gaussianBlur.glsl"))->get(0);
			q->bind(shader);
			q->uniform(shader, 1, (int)config.mipmapLevel);
			TextureHandle tex = provTex(config.provisionals, "blur", config.resolution, config.mipmapsCount, config.internalFormat);
			Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));

			const auto &blur = [&](const TextureHandle &texIn, const TextureHandle &texOut, const Vec2 &direction)
			{
				q->colorTexture(fb, 0, texOut, config.mipmapLevel);
				q->checkFrameBuffer(fb);
				q->bind(texIn, 0);
				q->uniform(shader, 0, direction);
				q->draw(model);
			};
			blur(config.texture, tex, Vec2(1, 0));
			blur(tex, config.texture, Vec2(0, 1));
		}
	}

	namespace privat
	{
		PointerRange<const char> pointsForSsaoShader(uint32 count);
	}

	void screenSpaceAmbientOcclusion(const ScreenSpaceAmbientOcclusionConfig &config_)
	{
		RenderQueue *q = config_.queue;
		const auto graphicsDebugScope = q->namedScope("ssao");
		ScreenSpaceAmbientOcclusionConfig config = config_;
		config.resolution /= 3;

		// params
		struct Shader
		{
			Mat4 proj;
			Mat4 projInv;
			Vec4 params; // strength, bias, power, raysLength
			Vec4i iparams; // sampleCount, hashSeed
		} s;
		s.proj = config.proj;
		s.projInv = inverse(config.proj);
		s.params = Vec4(config.strength, config.bias, config.power, config.raysLength);
		s.iparams[0] = config.samplesCount;
		s.iparams[1] = hash(config.frameIndex);
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		// framebuffer
		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));

		// low res depth
		TextureHandle depthTextureLowRes = provTex(config.provisionals, "ssaoDepthLowRes", config.resolution, 1, GL_R32F);
		{
			const auto graphicsDebugScope = q->namedScope("lowResDepth");
			q->colorTexture(fb, 0, depthTextureLowRes);
			q->checkFrameBuffer(fb);
			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/ssaoDownscaleDepth.glsl"))->get(0));
			q->bind(config.inDepth, 0);
			q->draw(model);
		}

		// points
		UniformBufferHandle ssaoPoints = config.provisionals->uniformBuffer(Stringizer() + "ssaoPoints_" + config.samplesCount, [count = config.samplesCount](UniformBuffer *ub) { ub->writeWhole(privat::pointsForSsaoShader(count), GL_STATIC_DRAW); });
		q->bind(ssaoPoints, 3);

		{ // generate
			const auto graphicsDebugScope = q->namedScope("generate");
			config.outAo = provTex(config.provisionals, "ssao", config.resolution, 1, GL_R8);
			q->colorTexture(fb, 0, config.outAo);
			q->checkFrameBuffer(fb);
			q->bind(depthTextureLowRes, 0);
			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/ssaoGenerate.glsl"))->get(0));
			q->draw(model);
		}

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.texture = config.outAo;
		gb.internalFormat = GL_R8;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		{ // resolve - update outAo inplace
			const auto graphicsDebugScope = q->namedScope("resolve");
			q->bind(config.outAo, 0);
			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/ssaoResolve.glsl"))->get(0));
			q->draw(model);
		}

		config_.outAo = config.outAo;
	}

	void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config)
	{
		static constexpr int downscale = 3;

		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("depth of field");

		const Vec2i res = max(config.resolution / downscale, 1u);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);
		q->viewport(Vec2i(), res);

		struct Shader
		{
			Mat4 projInv;
			Vec4 dofNear; // near, far
			Vec4 dofFar; // near, far
		} s;
		const Real fd = config.focusDistance;
		const Real fr = config.focusRadius;
		const Real br = config.blendRadius;
		s.projInv = inverse(config.proj);
		s.dofNear = Vec4(fd - fr - br, fd - fr, 0, 0);
		s.dofFar = Vec4(fd + fr, fd + fr + br, 0, 0);
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		TextureHandle texDof = provTex(config.provisionals, "dofColor", res, 1, GL_RGB16F);
		q->bind(texDof, 0); // ensure the texture is properly initialized
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));

		{ // collect
			q->bind(config.inColor, 0);
			q->bind(config.inDepth, 1);
			Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/dofCollect.glsl"))->get(0);
			q->bind(shader);
			q->colorTexture(fb, 0, texDof);
			q->checkFrameBuffer(fb);
			q->draw(model);
		}

		{ // blur
			GfGaussianBlurConfig gb;
			(ScreenSpaceCommonConfig &)gb = config;
			gb.resolution = res;
			gb.internalFormat = GL_RGB16F;
			gb.texture = texDof;
			for (uint32 i = 0; i < config.blurPasses; i++)
				gfGaussianBlur(gb);
		}

		{ // apply
			q->viewport(Vec2i(), config.resolution);
			q->colorTexture(fb, 0, config.outColor);
			q->checkFrameBuffer(fb);
			q->bind(config.inColor, 0);
			q->bind(config.inDepth, 1);
			q->bind(texDof, 2);
			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/dofApply.glsl"))->get(0));
			q->draw(model);
		}
	}

	namespace
	{
		struct EyeAdaptationParams
		{
			Vec4 logRange; // min, max range in log2 space
			Vec4 adaptationSpeed; // darker, lighter
			Vec4 nightParams; // nightOffset, nightDesaturation, nightContrast
			Vec4 applyParams; // key, strength
		};

		EyeAdaptationParams eyeAdaptationShaderParams(const ScreenSpaceEyeAdaptationConfig &config)
		{
			EyeAdaptationParams s;
			s.logRange = Vec4(config.lowLogLum, config.highLogLum, 0, 0);
			s.adaptationSpeed = Vec4(config.darkerSpeed, config.lighterSpeed, 0, 0) * config.elapsedTime;
			s.nightParams = Vec4(config.nightOffset, config.nightDesaturate, config.nightContrast, 0);
			s.applyParams = Vec4(config.key, config.strength, 0, 0);
			return s;
		}

		uint32 roundUpTo16(uint32 x)
		{
			return x / 16 + ((x % 16) > 0 ? 1 : 0);
		}
	}

	void screenSpaceEyeAdaptationPrepare(const ScreenSpaceEyeAdaptationConfig &config)
	{
		static constexpr int downscale = 4;

		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation prepare");

		const Vec2i res = max(config.resolution / downscale, 1u);

		q->universalUniformStruct(eyeAdaptationShaderParams(config), CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		q->bindImage(config.inColor, 0, true, false);
		TextureHandle texHist = provTex(config.provisionals, Stringizer() + "luminanceHistogram", Vec2i(256, 1), 1, GL_R32UI);
		q->bindImage(texHist, 1, true, true);
		TextureHandle texAccum = provTex(config.provisionals, Stringizer() + "luminanceAccumulation_" + config.cameraId, Vec2i(1), 1, GL_R32F);
		q->bindImage(texAccum, 2, true, true);

		// collection
		Holder<ShaderProgram> shaderCollection = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/luminanceCollection.glsl"))->get(0);
		q->compute(shaderCollection, Vec3i(roundUpTo16(res[0]), roundUpTo16(res[1]), 1));
		q->memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// histogram and accumulation
		Holder<ShaderProgram> shaderHistogram = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/luminanceHistogram.glsl"))->get(0);
		q->compute(shaderHistogram, Vec3i(1));
		q->memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT); // which one?
	}

	void screenSpaceBloom(const ScreenSpaceBloomConfig &config)
	{
		static constexpr int downscale = 3;

		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("bloom");

		const Vec2i res = max(config.resolution / downscale, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// generate
		const uint32 mips = max(config.blurPasses, 1u);
		TextureHandle tex = provTex(config.provisionals, "bloom", res, mips, GL_RGB16F);
		q->colorTexture(fb, 0, tex);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		Holder<ShaderProgram> shaderGenerate = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/bloomGenerate.glsl"))->get(0);
		q->bind(shaderGenerate);
		q->uniform(shaderGenerate, 0, Vec4(config.threshold, 0, 0, 0));
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));
		q->draw(model);

		// prepare mipmaps
		q->bind(tex, 0);
		q->filters(tex, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
		q->generateMipmaps(tex);

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.texture = tex;
		gb.resolution = res;
		gb.internalFormat = GL_RGB16F;
		gb.mipmapsCount = mips;
		for (uint32 i = 0; i < config.blurPasses; i++)
		{
			gb.mipmapLevel = i;
			gfGaussianBlur(gb);
		}

		// apply
		q->viewport(Vec2i(), config.resolution);
		q->bind(fb);
		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(tex, 1);
		Holder<ShaderProgram> shaderApply = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/bloomApply.glsl"))->get(0);
		q->bind(shaderApply);
		q->uniform(shaderApply, 0, (int)max(config.blurPasses, 1u));
		q->draw(model);
	}

	void screenSpaceEyeAdaptationApply(const ScreenSpaceEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation apply");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		q->universalUniformStruct(eyeAdaptationShaderParams(config), CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		TextureHandle texAccum = provTex(config.provisionals, Stringizer() + "luminanceAccumulation_" + config.cameraId, Vec2i(1), 1, GL_R32F);
		q->bind(texAccum, 1);
		Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/luminanceApply.glsl"))->get(0);
		q->bind(shader);
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj")));
	}

	void screenSpaceTonemap(const ScreenSpaceTonemapConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("tonemap");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		struct Shader
		{
			Vec4 params; // gamma, tonemapEnabled
		} s;
		s.params[0] = 1.0 / config.gamma;
		s.params[1] = config.tonemapEnabled;
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/tonemap.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj")));
	}

	void screenSpaceFastApproximateAntiAliasing(const ScreenSpaceFastApproximateAntiAliasingConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("fxaa");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/fxaa.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj")));
	}

	void screenSpaceSharpening(const ScreenSpaceSharpeningConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("sharpening");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		struct Shader
		{
			Vec4 params; // strength
		} s;
		s.params[0] = config.strength;
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/effects/sharpening.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj")));
	}
}
