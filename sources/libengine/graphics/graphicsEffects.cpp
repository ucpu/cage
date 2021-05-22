#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>

#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/graphicsEffects.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/model.h>
#include <cage-engine/texture.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/provisionalRenderData.h>

#include <cmath>

namespace cage
{
	namespace
	{
		void updateTexture(RenderQueue *q, const TextureHandle &tex, ivec2 resolution, uint32 internalFormat)
		{
			if (!tex.tryResolve() || tex.resolve()->resolution() != resolution)
			{
				q->bind(tex, CAGE_SHADER_TEXTURE_COLOR);
				q->image2d(resolution, internalFormat);
				q->filters(GL_LINEAR, GL_LINEAR, 0);
				q->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
			}
		}

		TextureHandle provTex(ProvisionalRenderData *prov, RenderQueue *q, const string &prefix, ivec2 resolution, uint32 internalFormat)
		{
			const string name = stringizer() + prefix + resolution + internalFormat;
			TextureHandle tex = prov->texture(name);
			updateTexture(q, tex, resolution, internalFormat);
			return tex;
		}

		struct GfGaussianBlurConfig : public GfCommonConfig
		{
			TextureHandle texture;
			uint32 internalFormat = 0;
			uint32 mipmapLevel = 0;
			uint32 mipmapsCount = 0;
		};

		void gfGaussianBlur(const GfGaussianBlurConfig &config)
		{
			RenderQueue *q = config.queue;
			const auto graphicsDebugScope = q->scopedNamedPass("blur");

			const ivec2 res = max(config.resolution / std::pow(2, config.mipmapLevel), 1);
			q->viewport(ivec2(), res);
			FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
			q->bind(fb);

			q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/gaussianBlur.glsl")));
			q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
			q->uniform(1, (int)config.mipmapLevel);

			TextureHandle tex = provTex(config.provisionals, config.queue, stringizer() + "blur" + config.mipmapsCount, config.resolution, config.internalFormat);
			if (config.mipmapsCount && !tex.tryResolve())
			{
				q->bind(tex, CAGE_SHADER_TEXTURE_COLOR);
				q->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
				q->generateMipmaps();
			}

			const auto &blur = [&](const TextureHandle &texIn, const TextureHandle &texOut, const vec2 &direction)
			{
				q->colorTexture(0, texOut, config.mipmapLevel);
				q->checkFrameBuffer();
				q->bind(texIn, CAGE_SHADER_TEXTURE_COLOR);
				q->uniform(0, direction);
				q->draw();
			};
			blur(config.texture, tex, vec2(1, 0));
			blur(tex, config.texture, vec2(0, 1));
		}
	}

	namespace privat
	{
		PointerRange<const char> pointsForSsaoShader();
	}

	void gfSsao(const GfSsaoConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("ssao");

		const ivec2 res = max(config.resolution / CAGE_SHADER_SSAO_DOWNSCALE, 1u);
		q->viewport(ivec2(), res);
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
			mat4 viewProj;
			mat4 viewProjInv;
			vec4 params; // strength, bias, power, radius
			ivec4 iparams; // sampleCount, frameIndex
		} s;
		s.viewProj = config.viewProj;
		s.viewProjInv = inverse(config.viewProj);
		s.params = vec4(config.strength, config.bias, config.power, config.worldRadius);
		s.iparams[0] = config.samplesCount;
		s.iparams[1] = config.frameIndex;
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		// generate
		updateTexture(q, config.outAo, res, GL_R8);
		q->colorTexture(0, config.outAo);
		q->checkFrameBuffer();
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(config.inNormal, CAGE_SHADER_TEXTURE_NORMAL);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoGenerate.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// blur
		GfGaussianBlurConfig gb;
		(GfCommonConfig &)gb = config;
		gb.texture = config.outAo;
		gb.resolution = res;
		gb.internalFormat = GL_R8;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		// apply - update outAo inplace
		q->bind(config.outAo, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoApply.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfDepthOfField(const GfDepthOfFieldConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("depth of field");

		const ivec2 res = max(config.resolution / CAGE_SHADER_DOF_DOWNSCALE, 1u);
		q->viewport(ivec2(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		struct Shader
		{
			mat4 projInv;
			vec4 dofNear; // near, far
			vec4 dofFar; // near, far
		} s;
		const real fd = config.focusDistance;
		const real fr = config.focusRadius;
		const real br = config.blendRadius;
		s.projInv = inverse(config.proj);
		s.dofNear = vec4(fd - fr - br, fd - fr, 0, 0);
		s.dofFar = vec4(fd + fr, fd + fr + br, 0, 0);
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		TextureHandle texNear = provTex(config.provisionals, config.queue, "dofNear", res, GL_RGB16F);
		TextureHandle texFar = provTex(config.provisionals, config.queue, "dofFar", res, GL_RGB16F);

		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/dofCollect.glsl")));
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
		(GfCommonConfig &)gb = config;
		gb.resolution = res;
		gb.internalFormat = GL_RGB16F;
		gb.texture = texNear;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);
		gb.texture = texFar;
		for (uint32 i = 0; i < config.blurPasses; i++)
			gfGaussianBlur(gb);

		// apply
		q->viewport(ivec2(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(texNear, CAGE_SHADER_TEXTURE_EFFECTS + 0);
		q->bind(texFar, CAGE_SHADER_TEXTURE_EFFECTS + 1);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/dofApply.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfMotionBlur(const GfMotionBlurConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("motion blur");

		q->viewport(ivec2(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inVelocity, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/motionBlur.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfEyeAdaptationPrepare(const GfEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("eye adaptation prepare");

		const ivec2 res = max(config.resolution / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u);
		q->viewport(ivec2(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// collection
		TextureHandle texCollect = provTex(config.provisionals, config.queue, "luminanceCollection", res, GL_R16F);
		q->bind(texCollect);
		q->filters(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0); // is linear necessary?
		q->colorTexture(0, texCollect);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCollection.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// downscale
		q->bind(texCollect);
		q->generateMipmaps();

		// accumulation / copy
		q->viewport(ivec2(), ivec2(1));
		TextureHandle texAccum = provTex(config.provisionals, config.queue, stringizer() + "luminanceAccumulation" + config.cameraId, ivec2(1), GL_R16F);
		q->colorTexture(0, texAccum);
		q->checkFrameBuffer();
		q->bind(texAccum, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(texCollect, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCopy.glsl")));
		q->uniform(0, vec2(config.darkerSpeed, config.lighterSpeed));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfBloom(const GfBloomConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("bloom");

		const ivec2 res = max(config.resolution / CAGE_SHADER_BLOOM_DOWNSCALE, 1u);
		q->viewport(ivec2(), res);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		// generate
		TextureHandle tex = provTex(config.provisionals, config.queue, "bloom", res, GL_RGB16F);
		q->colorTexture(0, tex);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomGenerate.glsl")));
		q->uniform(0, vec4(config.threshold, 0, 0, 0));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// prepare mipmaps
		q->bind(tex);
		q->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
		q->generateMipmaps();

		// blur
		GfGaussianBlurConfig gb;
		(GfCommonConfig &)gb = config;
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
		q->viewport(ivec2(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(tex, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomApply.glsl")));
		q->uniform(0, (int)max(config.blurPasses, 1u));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfEyeAdaptationApply(const GfEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("eye adaptation apply");

		q->viewport(ivec2(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		TextureHandle texAccum = provTex(config.provisionals, config.queue, stringizer() + "luminanceAccumulation" + config.cameraId, ivec2(1), GL_R16F);
		q->bind(texAccum, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceApply.glsl")));
		q->uniform(0, vec2(config.key, config.strength));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfTonemap(const GfTonemapConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("tonemap");

		q->viewport(ivec2(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		struct Shader
		{
			GfTonemap tonemap; // 7 reals
			real tonemapEnabled;
			vec4 gamma;
		} s;
		s.tonemap = config;
		s.tonemapEnabled = config.tonemapEnabled;
		s.gamma = vec4(1.0 / config.gamma, 0, 0, 0);
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/tonemap.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfFxaa(const GfFxaaConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("fxaa");

		q->viewport(ivec2(), config.resolution);
		FrameBufferHandle fb = config.provisionals->frameBufferDraw("graphicsEffects");
		q->bind(fb);

		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/fxaa.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}
}
