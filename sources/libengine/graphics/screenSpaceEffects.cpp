#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>

#include <cage-engine/opengl.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/model.h>
#include <cage-engine/texture.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/provisionalGraphics.h>

#include <cmath>

namespace cage
{
	namespace
	{
		void updateTexture(RenderQueue *q, TextureHandle tex, Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat)
		{
			if (tex.first())
			{
				q->image2d(tex, resolution, mipmapLevels, internalFormat);
				q->wraps(tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				if (mipmapLevels > 1)
				{
					q->filters(tex, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
					q->generateMipmaps(tex);
				}
				else
					q->filters(tex, GL_LINEAR, GL_LINEAR, 0);
			}
		}

		TextureHandle provTex(ProvisionalGraphics *prov, RenderQueue *q, const String &prefix, Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat)
		{
			const String name = Stringizer() + prefix + "_" + resolution + "_" + mipmapLevels + "_" + internalFormat;
			TextureHandle tex = prov->texture(name);
			updateTexture(q, tex, resolution, mipmapLevels, internalFormat);
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

			const Vec2i res = max(config.resolution / numeric_cast<uint32>(std::pow(2, config.mipmapLevel)), 1);
			q->viewport(Vec2i(), res);
			FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
			q->bind(fb);
			Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/gaussianBlur.glsl"))->get(0);
			q->bind(shader);
			q->uniform(shader, 1, (int)config.mipmapLevel);
			TextureHandle tex = provTex(config.provisionals, config.queue, "blur", config.resolution, config.mipmapsCount, config.internalFormat);
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

		uint32 mipsForResolution(const Vec2i res)
		{
			uint32 s = min(res[0], res[1]);
			return numeric_cast<uint32>(floor(log2(s))) + 1;
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
		if (ssaoPoints.first())
			q->writeWhole(ssaoPoints, privat::pointsForSsaoShader(), GL_STATIC_DRAW);
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
		updateTexture(q, config.outAo, config.resolution, 1, GL_R8);
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

		TextureHandle texNear = provTex(config.provisionals, config.queue, "dofNear", res, 1, GL_RGB16F);
		TextureHandle texFar = provTex(config.provisionals, config.queue, "dofFar", res, 1, GL_RGB16F);

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
		updateTexture(q, config.outColor, config.resolution, 1, GL_RGB16F);
		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.inDepth, 1);
		q->bind(texNear, 2);
		q->bind(texFar, 3);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/dofApply.glsl"))->get(0));
		q->draw(model);
	}

	void screenSpaceEyeAdaptationPrepare(const ScreenSpaceEyeAdaptationConfig &config)
	{
		const int downscale = 4;

		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation prepare");

		const Vec2i res = max(config.resolution / downscale, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// collection
		TextureHandle texCollect = provTex(config.provisionals, config.queue, "luminanceCollection", res, mipsForResolution(res), GL_R16F);
		q->filters(texCollect, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0); // is linear necessary?
		q->colorTexture(fb, 0, texCollect);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceCollection.glsl"))->get(0));
		Holder<Model> model = config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
		q->draw(model);

		// downscale
		q->generateMipmaps(texCollect);

		// accumulation / copy
		q->viewport(Vec2i(), Vec2i(1));
		TextureHandle texAccum = provTex(config.provisionals, config.queue, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), 1, GL_R16F);
		q->bind(fb);
		q->colorTexture(fb, 0, texAccum);
		q->checkFrameBuffer(fb);
		q->bind(texCollect, 0);
		q->bind(texAccum, 1);
		Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceCopy.glsl"))->get(0);
		q->bind(shader);
		q->uniform(shader, 0, Vec2(config.darkerSpeed, config.lighterSpeed));
		q->draw(model);
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
		TextureHandle tex = provTex(config.provisionals, config.queue, "bloom", res, mips, GL_RGB16F);
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
		updateTexture(q, config.outColor, config.resolution, 1, GL_RGB16F);
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

		updateTexture(q, config.outColor, config.resolution, 1, GL_RGB16F);
		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		TextureHandle texAccum = provTex(config.provisionals, config.queue, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), 1, GL_R16F);
		q->bind(texAccum, 1);
		Holder<ShaderProgram> shader = config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/luminanceApply.glsl"))->get(0);
		q->bind(shader);
		q->uniform(shader, 0, Vec2(config.key, config.strength));
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

		updateTexture(q, config.outColor, config.resolution, 1, GL_RGB16F);
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

		updateTexture(q, config.outColor, config.resolution, 1, GL_RGB16F);
		q->colorTexture(fb, 0, config.outColor);
		q->checkFrameBuffer(fb);
		q->bind(config.inColor, 0);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/effects/fxaa.glsl"))->get(0));
		q->draw(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
	}
}
