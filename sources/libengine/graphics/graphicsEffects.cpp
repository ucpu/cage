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

namespace cage
{
	namespace
	{
		FrameBufferHandle genFB(ProvisionalRenderData *prov, const string &prefix, ivec2 resolution, bool read = false)
		{
			const string name = stringizer() + prefix + resolution + read;
			return read ? prov->frameBufferRead(name) : prov->frameBufferDraw(name);
		}

		void updateTexture(RenderQueue *q, const TextureHandle &tex, ivec2 resolution, uint32 internalFormat)
		{
			if (!tex.tryResolve() || tex.resolve()->resolution() != resolution)
			{
				q->bind(tex);
				q->image2d(resolution, internalFormat);
				q->filters(GL_LINEAR, GL_LINEAR, 0);
				q->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
			}
		}

		TextureHandle genTex(ProvisionalRenderData *prov, RenderQueue *q, const string &prefix, ivec2 resolution, uint32 internalFormat)
		{
			const string name = stringizer() + prefix + resolution + internalFormat;
			TextureHandle tex = prov->texture(name);
			updateTexture(q, tex, resolution, internalFormat);
			return tex;
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
		FrameBufferHandle fb = genFB(config.provisionals, "ssao", res);
		q->bind(fb);
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
		q->bind(fb);
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

		TextureHandle texNear = genTex(config.provisionals, config.queue, "dofNear", res, GL_RGB16F);
		TextureHandle texFar = genTex(config.provisionals, config.queue, "dofFar", res, GL_RGB16F);

		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.inDepth, CAGE_SHADER_TEXTURE_DEPTH);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/dofCollect.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		{ // collect near
			FrameBufferHandle fbNear = genFB(config.provisionals, "dofNear", res);
			q->bind(fbNear);
			q->colorTexture(0, texNear);
			q->checkFrameBuffer();
			q->uniform(0, 0);
			q->draw();
		}
		{ // collect far
			FrameBufferHandle fbFar = genFB(config.provisionals, "dofFar", res);
			q->bind(fbFar);
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
		FrameBufferHandle fbApply = genFB(config.provisionals, "dofApply", config.resolution);
		q->bind(fbApply);
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
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		FrameBufferHandle fb = genFB(config.provisionals, "motionBlur", config.resolution);
		q->bind(fb);
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

		// collection
		TextureHandle texCollect = genTex(config.provisionals, config.queue, "luminanceCollection", res, GL_R16F);
		q->bind(texCollect);
		q->filters(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0); // is linear necessary?
		FrameBufferHandle fbCollect = genFB(config.provisionals, "luminanceCollection", res);
		q->bind(fbCollect);
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
		TextureHandle texAccum = genTex(config.provisionals, config.queue, stringizer() + "luminanceAccumulation" + config.cameraId, ivec2(1), GL_R16F);
		FrameBufferHandle fbAccum = genFB(config.provisionals, stringizer() + "luminanceAccumulation" + config.cameraId, ivec2(1));
		q->bind(fbAccum);
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

		// generate
		TextureHandle tex = genTex(config.provisionals, config.queue, "bloom", res, GL_RGB16F);
		q->bind(tex);
		q->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
		FrameBufferHandle fbGen = genFB(config.provisionals, "bloomGenerate", res);
		q->bind(fbGen);
		q->colorTexture(0, tex);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomGenerate.glsl")));
		q->uniform(0, vec4(config.threshold, 0, 0, 0));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();

		// blur
		GfGaussianBlurConfig gb;
		(GfCommonConfig &)gb = config;
		gb.texture = tex;
		gb.resolution = res;
		gb.internalFormat = GL_RGB16F;
		for (uint32 i = 0; i < config.blurPasses; i++)
		{
			gb.mipmapLevel = i;
			gfGaussianBlur(gb);
		}

		// apply
		q->viewport(ivec2(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		FrameBufferHandle fbApply = genFB(config.provisionals, "bloomApply", config.resolution);
		q->bind(fbApply);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(tex, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomApply.glsl")));
		q->uniform(0, (int)config.blurPasses);
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfEyeAdaptationApply(const GfEyeAdaptationConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("eye adaptation apply");

		struct Shader
		{
			vec4 luminanceParams;
		} s;
		s.luminanceParams = vec4(config.key, config.strength, 0, 0);
		q->universalUniformStruct(s, CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES);

		q->viewport(ivec2(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		FrameBufferHandle fb = genFB(config.provisionals, "luminanceApply", config.resolution);
		q->bind(fb);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		TextureHandle texAccum = genTex(config.provisionals, config.queue, stringizer() + "luminanceAccumulation" + config.cameraId, ivec2(1), GL_R16F);
		q->bind(texAccum, CAGE_SHADER_TEXTURE_EFFECTS);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceApply.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfTonemap(const GfTonemapConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("tonemap");

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

		q->viewport(ivec2(), config.resolution);
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		FrameBufferHandle fb = genFB(config.provisionals, "tonemap", config.resolution);
		q->bind(fb);
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
		updateTexture(q, config.outColor, config.resolution, GL_RGB16F);
		FrameBufferHandle fb = genFB(config.provisionals, "fxaa", config.resolution);
		q->bind(fb);
		q->colorTexture(0, config.outColor);
		q->checkFrameBuffer();
		q->bind(config.inColor, CAGE_SHADER_TEXTURE_COLOR);
		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/fxaa.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->draw();
	}

	void gfGaussianBlur(const GfGaussianBlurConfig &config)
	{
		RenderQueue *q = config.queue;
		const auto graphicsDebugScope = q->scopedNamedPass("blur");

		const ivec2 res = config.resolution / (1 + config.mipmapLevel);
		q->viewport(ivec2(), res);

		q->bind(config.assets->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/gaussianBlur.glsl")));
		q->bind(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
		q->uniform(1, (int)config.mipmapLevel);

		TextureHandle tex = genTex(config.provisionals, config.queue, "blur", config.resolution, config.internalFormat);
		FrameBufferHandle fb = genFB(config.provisionals, "blur", config.resolution);
		q->bind(fb);

		const auto &blur = [&](const TextureHandle &tex1, const TextureHandle &tex2, const vec2 &direction)
		{
			q->colorTexture(0, tex2, config.mipmapLevel);
			q->checkFrameBuffer();
			q->bind(tex1, CAGE_SHADER_TEXTURE_COLOR);
			q->uniform(0, direction);
			q->draw();
		};
		blur(config.texture, tex, vec2(1, 0));
		blur(tex, config.texture, vec2(0, 1));
	}
}
