#include <cage-core/assetsManager.h>
#include <cage-core/hashString.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsBindings.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shader.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		struct GaussianBlurConfig : public ScreenSpaceCommonConfig
		{
			Texture *inTex = nullptr;
			Texture *tmpTex = nullptr;
		};

		void gaussianBlur(const GaussianBlurConfig &config)
		{
			Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));
			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/gaussianBlur.glsl"));

			const auto &blur = [&](Texture *texIn, Texture *texOut, uint32 variant)
			{
				RenderPassConfig pass;
				pass.colorTargets.push_back({ texOut });
				config.encoder->nextPass(pass);
				const auto scope = config.encoder->namedScope("gaussian blur");

				GraphicsBindingsCreateConfig bind;
				bind.textures.push_back({ texIn, 0 });

				DrawConfig draw;
				draw.bindings = newGraphicsBindings(config.encoder->getDevice(), bind);
				draw.model = +model;
				draw.shader = +ms->get(variant);
				config.encoder->draw(draw);
			};
			blur(config.inTex, config.tmpTex, 0);
			blur(config.tmpTex, config.inTex, HashString("Vertical"));
		}

		std::vector<Holder<Texture>> generateMipsViews(GraphicsDevice *device, Holder<Texture> tex, uint32 mips)
		{
			std::vector<Holder<Texture>> mipTexs;
			mipTexs.reserve(mips);

			wgpu::SamplerDescriptor sd = {};
			sd.addressModeU = sd.addressModeV = sd.addressModeW = wgpu::AddressMode::ClampToEdge;
			sd.magFilter = sd.minFilter = wgpu::FilterMode::Linear;
			sd.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
			sd.label = "mip sampler";
			wgpu::Sampler samp = device->nativeDevice()->CreateSampler(&sd);

			for (uint32 i = 0; i < mips; i++)
			{
				wgpu::TextureViewDescriptor tvd = {};
				tvd.baseMipLevel = i;
				tvd.mipLevelCount = 1;
				wgpu::TextureView view = tex->nativeTexture().CreateView(&tvd);
				mipTexs.push_back(newTexture(tex->nativeTexture(), view, samp, "mip view"));
			}

			return mipTexs;
		}
	}

	namespace privat
	{
		PointerRange<const char> pointsForSsaoShader(uint32 count);
	}

	void screenSpaceAmbientOcclusion(const ScreenSpaceAmbientOcclusionConfig &config)
	{
		static constexpr int downscale = 3;

		GraphicsEncoder *q = config.encoder;
		GraphicsDevice *d = q->getDevice();

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));

		// prepare

		const Vec2i res = max(config.resolution / downscale, 1u);
		Holder<Texture> depthLowRes = [&]()
		{
			TransientTextureCreateConfig conf;
			conf.name = "ssao depth target";
			conf.resolution = Vec3i(res, 1);
			conf.format = wgpu::TextureFormat::R32Float;
			return newTexture(d, conf);
		}();
		Holder<Texture> ssaoLowRes = [&]()
		{
			TransientTextureCreateConfig conf;
			conf.name = "ssao lowres target";
			conf.resolution = Vec3i(res, 1);
			conf.format = wgpu::TextureFormat::R16Float;
			return newTexture(d, conf);
		}();

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
		const AggregatedBinding buffUni = config.aggregate->writeStruct(s, 0, true);

		const AggregatedBinding buffPoints = config.aggregate->writeBuffer(privat::pointsForSsaoShader(config.samplesCount), 1, true);

		{ // low-res depth
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +depthLowRes });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("ssao depth");

			GraphicsBindingsCreateConfig bind;
			bind.textures.push_back({ config.inDepth, 0 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/ssaoDownscaleDepth.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}

		{ // generate
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +ssaoLowRes });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("ssao generate");

			GraphicsBindingsCreateConfig bind;
			bind.buffers.push_back(buffUni);
			bind.buffers.push_back(buffPoints);
			bind.textures.push_back({ +depthLowRes, 2 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/ssaoGenerate.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.dynamicOffsets.push_back(buffUni);
			draw.dynamicOffsets.push_back(buffPoints);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}

		{ // blur
			Holder<Texture> tmp = [&]()
			{
				TransientTextureCreateConfig conf;
				conf.name = "ssao blur temporary";
				conf.resolution = Vec3i(res, 1);
				conf.format = wgpu::TextureFormat::R16Float;
				return newTexture(d, conf);
			}();

			GaussianBlurConfig gb;
			(ScreenSpaceCommonConfig &)gb = config;
			gb.inTex = +ssaoLowRes;
			gb.tmpTex = +tmp;
			for (uint32 i = 0; i < config.blurPasses; i++)
				gaussianBlur(gb);
		}

		{ // resolve
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +config.outAo });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("ssao resolve");

			GraphicsBindingsCreateConfig bind;
			bind.buffers.push_back(buffUni);
			bind.textures.push_back({ +ssaoLowRes, 1 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/ssaoResolve.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.dynamicOffsets.push_back(buffUni);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}
	}

	void screenSpaceDepthOfField(const ScreenSpaceDepthOfFieldConfig &config)
	{
		static constexpr int downscale = 3;

		GraphicsEncoder *q = config.encoder;
		GraphicsDevice *d = q->getDevice();

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));

		// prepare

		const Vec2i res = max(config.resolution / downscale, 1u);
		Holder<Texture> texDof = [&]()
		{
			TransientTextureCreateConfig conf;
			conf.name = "dof color target";
			conf.resolution = Vec3i(res, 1);
			conf.format = wgpu::TextureFormat::RGBA16Float;
			conf.samplerVariant = true;
			return newTexture(d, conf);
		}();

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
		const AggregatedBinding buff = config.aggregate->writeStruct(s, 0, true);

		{ // collect
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +texDof });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("dof collect");

			GraphicsBindingsCreateConfig bind;
			bind.textures.push_back({ config.inColor, 0 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/dofCollect.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}

		{ // blur
			Holder<Texture> tmp = [&]()
			{
				TransientTextureCreateConfig conf;
				conf.name = "dof blur temporary";
				conf.resolution = Vec3i(res, 1);
				conf.format = wgpu::TextureFormat::RGBA16Float;
				return newTexture(d, conf);
			}();

			GaussianBlurConfig gb;
			(ScreenSpaceCommonConfig &)gb = config;
			gb.inTex = +texDof;
			gb.tmpTex = +tmp;
			for (uint32 i = 0; i < config.blurPasses; i++)
				gaussianBlur(gb);
		}

		{ // apply
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +config.outColor });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("dof apply");

			GraphicsBindingsCreateConfig bind;
			bind.buffers.push_back(buff);
			bind.textures.push_back({ config.inColor, 1 });
			bind.textures.push_back({ config.inDepth, 3 });
			bind.textures.push_back({ +texDof, 5 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/dofApply.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.dynamicOffsets.push_back(buff);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}
	}

	void screenSpaceBloom(const ScreenSpaceBloomConfig &config)
	{
		static constexpr int downscale = 3;

		GraphicsEncoder *q = config.encoder;
		GraphicsDevice *d = q->getDevice();

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));

		// prepare

		const Vec2i res = max(config.resolution / downscale, 1u);
		const uint32 mips = max(config.blurPasses, 1u);
		Holder<Texture> tex = [&]()
		{
			TransientTextureCreateConfig conf;
			conf.name = "bloom target";
			conf.resolution = Vec3i(res, 1);
			conf.mipLevelCount = mips;
			conf.format = wgpu::TextureFormat::RGBA16Float;
			conf.samplerVariant = true;
			return newTexture(d, conf);
		}();
		std::vector<Holder<Texture>> mipViews = generateMipsViews(d, tex.share(), mips);

		struct Shader
		{
			Vec4 params; // threshold, blur passes
		} s;
		s.params[0] = config.threshold;
		s.params[1] = max(config.blurPasses, 1u) + 1e-5;
		const AggregatedBinding buff = config.aggregate->writeStruct(s, 0, true);

		{ // generate
			RenderPassConfig pass;
			pass.colorTargets.push_back({ +mipViews[0] });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("bloom generate");

			GraphicsBindingsCreateConfig bind;
			bind.buffers.push_back(buff);
			bind.textures.push_back({ config.inColor, 1 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/bloomGenerate.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.dynamicOffsets.push_back(buff);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}

		{ // generate mipmaps
			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/makeMip.glsl"));

			for (uint32 i = 1; i < mips; i++)
			{
				RenderPassConfig pass;
				pass.colorTargets.push_back({ +mipViews[i] });
				q->nextPass(pass);
				const auto scope = config.encoder->namedScope("bloom gen mip");

				GraphicsBindingsCreateConfig bind;
				bind.textures.push_back({ +mipViews[i - 1], 0 });

				DrawConfig draw;
				draw.bindings = newGraphicsBindings(d, bind);
				draw.model = +model;
				draw.shader = +ms->get(0);
				q->draw(draw);
			}
		}

		{ // blur
			Holder<Texture> tmp = [&]()
			{
				TransientTextureCreateConfig conf;
				conf.name = "bloom blur temporary";
				conf.resolution = Vec3i(res, 1);
				conf.mipLevelCount = mips;
				conf.format = wgpu::TextureFormat::RGBA16Float;
				return newTexture(d, conf);
			}();
			std::vector<Holder<Texture>> tmpViews = generateMipsViews(d, tmp.share(), mips);

			for (uint32 i = 0; i < mips; i++)
			{
				GaussianBlurConfig gb;
				(ScreenSpaceCommonConfig &)gb = config;
				gb.inTex = +mipViews[i];
				gb.tmpTex = +tmpViews[i];
				gaussianBlur(gb);
			}
		}

		{ // apply
			RenderPassConfig pass;
			pass.colorTargets.push_back({ config.outColor });
			q->nextPass(pass);
			const auto scope = config.encoder->namedScope("bloom apply");

			GraphicsBindingsCreateConfig bind;
			bind.buffers.push_back(buff);
			bind.textures.push_back({ config.inColor, 1 });
			bind.textures.push_back({ +tex, 3 });

			Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/bloomApply.glsl"));

			DrawConfig draw;
			draw.bindings = newGraphicsBindings(d, bind);
			draw.dynamicOffsets.push_back(buff);
			draw.model = +model;
			draw.shader = +ms->get(0);
			q->draw(draw);
		}
	}

	void screenSpaceTonemap(const ScreenSpaceTonemapConfig &config)
	{
		RenderPassConfig pass;
		pass.colorTargets.push_back({ +config.outColor });
		config.encoder->nextPass(pass);
		const auto scope = config.encoder->namedScope("tonemap");

		struct Shader
		{
			Vec4 params; // gamma, tonemapEnabled
		} s;
		s.params[0] = 1.0 / config.gamma;
		s.params[1] = config.tonemapEnabled;
		const AggregatedBinding buff = config.aggregate->writeStruct(s, 0, true);

		GraphicsBindingsCreateConfig bind;
		bind.buffers.push_back(buff);
		bind.textures.push_back({ config.inColor, 1 });

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));
		Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/tonemap.glsl"));

		DrawConfig draw;
		draw.bindings = newGraphicsBindings(config.encoder->getDevice(), bind);
		draw.dynamicOffsets.push_back(buff);
		draw.model = +model;
		draw.shader = +ms->get(0);
		config.encoder->draw(draw);
	}

	void screenSpaceFastApproximateAntiAliasing(const ScreenSpaceFastApproximateAntiAliasingConfig &config)
	{
		RenderPassConfig pass;
		pass.colorTargets.push_back({ +config.outColor });
		config.encoder->nextPass(pass);
		const auto scope = config.encoder->namedScope("fxaa");

		GraphicsBindingsCreateConfig bind;
		bind.textures.push_back({ config.inColor, 0 });

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));
		Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/fxaa.glsl"));

		DrawConfig draw;
		draw.bindings = newGraphicsBindings(config.encoder->getDevice(), bind);
		draw.model = +model;
		draw.shader = +ms->get(0);
		config.encoder->draw(draw);
	}

	void screenSpaceSharpening(const ScreenSpaceSharpeningConfig &config)
	{
		RenderPassConfig pass;
		pass.colorTargets.push_back({ +config.outColor });
		config.encoder->nextPass(pass);
		const auto scope = config.encoder->namedScope("sharpening");

		struct Shader
		{
			Vec4 params; // strength
		} s;
		s.params[0] = config.strength;
		const AggregatedBinding buff = config.aggregate->writeStruct(s, 0, true);

		GraphicsBindingsCreateConfig bind;
		bind.buffers.push_back(buff);
		bind.textures.push_back({ config.inColor, 1 });

		Holder<Model> model = config.assets->get<Model>(HashString("cage/models/square.obj"));
		Holder<MultiShader> ms = config.assets->get<MultiShader>(HashString("cage/shaders/effects/sharpening.glsl"));

		DrawConfig draw;
		draw.bindings = newGraphicsBindings(config.encoder->getDevice(), bind);
		draw.dynamicOffsets.push_back(buff);
		draw.model = +model;
		draw.shader = +ms->get(0);
		config.encoder->draw(draw);
	}
}
