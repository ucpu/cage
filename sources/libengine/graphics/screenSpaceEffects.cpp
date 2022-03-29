#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>

#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
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
		void updateTexture(RenderQueue *q, const TextureHandle &tex, Vec2i resolution, uint32 internalFormat)
		{
			if (!tex.tryResolve() || tex.resolve()->resolution() != resolution)
			{
				q->bind(tex, CAGE_SHADER_TEXTURE_COLOR);
				q->image2d(resolution, internalFormat);
				q->filters(GL_LINEAR, GL_LINEAR, 0);
				q->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
			}
		}

		TextureHandle provTex(ProvisionalGraphics *prov, RenderQueue *q, const String &prefix, Vec2i resolution, uint32 internalFormat)
		{
			const String name = Stringizer() + prefix + resolution + internalFormat;
			TextureHandle tex = prov->texture(name);
			updateTexture(q, tex, resolution, internalFormat);
			return tex;
		}

		struct GfGaussianBlurConfig : public ScreenSpaceCommonConfig
		{
			TextureHandle texture;
			uint32 internalFormat = 0;
			uint32 mipmapLevel = 0;
			uint32 mipmapsCount = 0;
		};

		void gfGaussianBlur(const GfGaussianBlurConfig &config)
		{
			RenderQueue *q = config.queue;
			const auto graphicsDebugScope = q->namedScope("blur");

			const Vec2i res = max(config.resolution / numeric_cast<uint32>(std::pow(2, config.mipmapLevel)), 1);
			q->viewport(Vec2i(), res);
			FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
			q->bind(fb);

			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/gaussianBlur.glsl")));
			q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
			q->uniform(1, (int)config.mipmapLevel);

			TextureHandle tex = provTex(config.provisionals, config.queue, Stringizer() + "blur" + config.mipmapsCount, config.resolution, config.internalFormat);
			if (config.mipmapsCount && !tex.tryResolve())
			{
				q->bind(tex, CAGE_SHADER_TEXTURE_COLOR);
				q->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
				q->generateMipmaps();
			}

			const auto &blur = [&](const TextureHandle &texIn, const TextureHandle &texOut, const Vec2 &direction)
			{
				q->colorTexture(0, texOut, config.mipmapLevel);
				q->checkFrameBuffer();
				q->bind(texIn, CAGE_SHADER_TEXTURE_COLOR);
				q->uniform(0, direction);
				q->draw();
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

		const Vec2i res = max(config.resolution / CAGE_SHADER_SSAO_DOWNSCALE, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		UniformBufferHandle ssaoPoints = config.provisionals->uniformBuffer("ssaoPoints");
		if (!ssaoPoints.tryResolve())
		{
			q->bind(ssaoPoints);
			q->writeWhole(privat::pointsForSsaoShader(), GL_STATIC_DRAW);
		}
		q->bind(ssaoPoints, CAGE_SHADER_UNIBLOCK_SSAO_POINTS);

		struct Shader
		{
			Mat4 viewProj;
			Mat4 viewProjInv;
			Vec4 params; // strength, bias, power, radius
			Vec4i iparams; // sampleCount, frameIndex, resolution x, resolution y
		} s;
		s.viewProj = config.viewProj;
		s.viewProjInv = inverse(config.viewProj);
		s.params = Vec4(config.strength, config.bias, config.power, config.worldRadius);
		s.iparams[0] = config.samplesCount;
		s.iparams[1] = config.frameIndex;
		s.iparams[2] = config.resolution[0];
		s.iparams[3] = config.resolution[1];
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		// generate
		updateTexture(q, config.outAo, res, GL_R8);
		q->colorTexture(0, config.outAo);
		q->checkFrameBuffer();
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/ssaoGenerate.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.texture = config.outAo;
		gb.resolution = res;
		gb.internalFormat = GL_R8;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		// resolve - update outAo inplace
		q->bind(config.outAo, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/ssaoResolve.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("depth of field");

		const Vec2i res = max(config.resolution / CAGE_SHADER_DOF_DOWNSCALE, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

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
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		TextureHandle texNear = provTex(config.provisionals, config.queue, "dofNear", res, GL_RGB16F);
		TextureHandle texFar = provTex(config.provisionals, config.queue, "dofFar", res, GL_RGB16F);

		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/dofCollect.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		{ // collect near
			q->colorTexture(0, texNear);
			q->checkFrameBuffer();
			q->uniform(0, 0);
			q->draw();
		}
		{ // collect far
			q->colorTexture(0, texFar);
			q->checkFrameBuffer();
			q->uniform(0, 1);
			q->draw();
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
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(texNear, CAGE_SHADER_TEXTURE_EFFECTS + 0);
		q->bind(texFar, CAGE_SHADER_TEXTURE_EFFECTS + 1);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/dofApply.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void screenSpaceEyeAdaptationPrepare(const ScreenSpaceEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation prepare");

		const Vec2i res = max(config.resolution / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// collection
		TextureHandle texCollect = provTex(config.provisionals, config.queue, "luminanceCollection", res, GL_R16F);
		q->bind(texCollect, CAGE_SHADER_TEXTURE_COLOR);
		q->filters(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0); // is linear necessary?
		q->colorTexture(0, texCollect);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/luminanceCollection.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// downscale
		q->bind(texCollect, CAGE_SHADER_TEXTURE_COLOR);
		q->generateMipmaps();

		// accumulation / copy
		q->viewport(Vec2i(), Vec2i(1));
		TextureHandle texAccum = provTex(config.provisionals, config.queue, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), GL_R16F);
		q->colorTexture(0, texAccum);
		q->checkFrameBuffer();
		q->bind(texAccum, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(texCollect, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/luminanceCopy.glsl")));
		q->uniform(0, Vec2(config.darkerSpeed, config.lighterSpeed));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void screenSpaceBloom(const ScreenSpaceBloomConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("bloom");

		const Vec2i res = max(config.resolution / CAGE_SHADER_BLOOM_DOWNSCALE, 1u);
		q->viewport(Vec2i(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// generate
		TextureHandle tex = provTex(config.provisionals, config.queue, "bloom", res, GL_RGB16F);
		q->colorTexture(0, tex);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/bloomGenerate.glsl")));
		q->uniform(0, Vec4(config.threshold, 0, 0, 0));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// prepare mipmaps
		q->bind(tex, CAGE_SHADER_TEXTURE_COLOR);
		q->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
		q->generateMipmaps();

		// blur
		GfGaussianBlurConfig gb;
		(ScreenSpaceCommonConfig &)gb = config;
		gb.texture = tex;
		gb.resolution = res;
		gb.internalFormat = GL_RGB16F;
		gb.mipmapsCount = max(config.blurPasses, 1u) - 1;
		for (uint32 i = 0; i < config.blurPasses; i++)
		{
			gb.mipmapLevel = i;
			gfGaussianBlur(gb);
		}

		// apply
		q->viewport(Vec2i(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(tex, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/bloomApply.glsl")));
		q->uniform(0, (int)max(config.blurPasses, 1u));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void screenSpaceEyeAdaptationApply(const ScreenSpaceEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("eye adaptation apply");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		TextureHandle texAccum = provTex(config.provisionals, config.queue, Stringizer() + "luminanceAccumulation" + config.cameraId, Vec2i(1), GL_R16F);
		q->bind(texAccum, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/luminanceApply.glsl")));
		q->uniform(0, Vec2(config.key, config.strength));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
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
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/tonemap.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void screenSpaceFastApproximateAntiAliasing(const ScreenSpaceFastApproximateAntiAliasingConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->namedScope("fxaa");

		q->viewport(Vec2i(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/effects/fxaa.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}
}
