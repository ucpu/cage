#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>

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
					{
						tex->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
						tex->generateMipmaps();
					}
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
			Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/gaussianBlur.glsl"))->get(0);
			q->bind(shader);
			q->uniform(shader, 1, (int)config.mipmapLevel);
			TextureHandle tex = provTex(config.provisionals, "blur", config.resolution, config.mipmapsCount, config.internalFormat);
			Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));

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
		PointerRange<const char> pointsForSsaoShader();
	}

	void screenSpaceAmbientOcclusion(const ScreenSpaceAmbientOcclusionConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("ssao");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		UniformBufferHandle ssaoPoints = config.provisionals->uniformBuffer("ssaoPoints");
		{ // potentially dangerous hack
			ProvisionalUniformBuffer *pub = (ProvisionalUniformBuffer *)ssaoPoints.pointer();
			CAGE_ASSERT(pub);
			if (!pub->ready())
				q->writeWhole(ssaoPoints, privat::pointsForSsaoShader(), GL_STATIC_DRAW);
		}
		q->bind(ssaoPoints, 3);

		struct Shader
		{
			Mat4 proj;
			Mat4 projInv;
			Vec4 params; // strength, bias, power, radius
			Vec4i iparams; // sampleCount, hashSeed
		} s;
		s.proj = config.proj;
		s.projInv = inverse(config.proj);
		s.params = Vec4(config.strength, config.bias, config.power, config.worldRadius);
		s.iparams[0] = config.samplesCount;
		s.iparams[1] = hash(config.frameIndex);
		q->universalUniformStruct(s, 2);

		// generate
		config.outAo = provTex(config.provisionals, "ssao", config.resolution, 1, GL_R8);
		q->colorTexture(fb, 0, config.outAo);
		q->checkFrameBuffer(fb);
		q->bind(config.inDepth, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/ssaoGenerate.glsl"))->get(0));
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
		q->draw(model);

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.texture = config.outAo;
		gb.internalFormat = GL_R8;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		// resolve - update outAo inplace
		q->bind(config.outAo, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/ssaoResolve.glsl"))->get(0));
		q->draw(model);
	}

	void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config)
	{
		const int downscale = 3;

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
		q->universalUniformStruct(s, 2);

		TextureHandle texNear = provTex(config.provisionals, "dofNear", res, 1, GL_RGB16F);
		TextureHandle texFar = provTex(config.provisionals, "dofFar", res, 1, GL_RGB16F);

		q->bind(config.inColor, 0);
		q->bind(config.inDepth, 1);
		Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/dofCollect.glsl"))->get(0);
		q->bind(shader);
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
		{ // collect near
			q->colorTexture(fb, 0, texNear);
			q->checkFrameBuffer(fb);
			q->uniform(shader, 0, 0);
			q->draw(model);
		}
		{ // collect far
			q->colorTexture(fb, 0, texFar);
			q->checkFrameBuffer(fb);
			q->uniform(shader, 0, 1);
			q->draw(model);
		}

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.resolution = res;
		gb.internalFormat = GL_RGB16F;
		gb.texture = texNear;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);
		gb.texture = texFar;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		// apply
		q->viewport(Vec2i(), config.resolution);
		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.inDepth, 1);
		q->bind(texNear, 2);
		q->bind(texFar, 3);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/dofApply.glsl"))->get(0));
		q->draw(model);
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
		const int downscale = 4;

		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation prepare");

		const Vec2i res = max(config.resolution / downscale, 1u);

		q->universalUniformStruct(eyeAdaptationShaderParams(config), 0);

		q->bindImage(config.inColor, 0, true, false);
		TextureHandle texHist = provTex(config.provisionals, Stringizer() + "luminanceHistogram", Vec2i(256, 1), 1, GL_R32UI);
		q->bindImage(texHist, 1, true, true);
		TextureHandle texAccum = provTex(config.provisionals, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), 1, GL_R32F);
		q->bindImage(texAccum, 2, true, true);

		// collection
		Holder<ShaderProgram> shaderCollection = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceCollection.glsl"))->get(0);
		q->compute(shaderCollection, Vec3i(roundUpTo16(res[0]), roundUpTo16(res[1]), 1));
		q->memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// histogram and accumulation
		Holder<ShaderProgram> shaderHistogram = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceHistogram.glsl"))->get(0);
		q->compute(shaderHistogram, Vec3i(1));
		q->memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT); // which one?
	}

	void screenSpaceBloom(const ScreenSpaceBloomConfig &config)
	{
		const int downscale = 3;

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
		Holder<ShaderProgram> shaderGenerate = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/bloomGenerate.glsl"))->get(0);
		q->bind(shaderGenerate);
		q->uniform(shaderGenerate, 0, Vec4(config.threshold, 0, 0, 0));
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
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
		Holder<ShaderProgram> shaderApply = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/bloomApply.glsl"))->get(0);
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

		q->universalUniformStruct(eyeAdaptationShaderParams(config), 0);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		TextureHandle texAccum = provTex(config.provisionals, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), 1, GL_R32F);
		q->bind(texAccum, 1);
		Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceApply.glsl"))->get(0);
		q->bind(shader);
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
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
			ScreenSpaceTonemap tonemap; // 7 reals
			Real tonemapEnabled;
			Vec4 gamma;
		} s;
		s.tonemap = config;
		s.tonemapEnabled = config.tonemapEnabled;
		s.gamma = Vec4(1.0 / config.gamma, 0, 0, 0);
		q->universalUniformStruct(s, 2);

		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/tonemap.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
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
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/fxaa.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
	}
}
