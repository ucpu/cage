#include <webgpu/webgpu_cpp.h>

#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/graphicsPipeline.h>
#include <cage-engine/model.h>
#include <cage-engine/shader.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		wgpu::BlendState convertBlending(BlendingEnum blending)
		{
			wgpu::BlendState bs = {};
			bs.color.operation = wgpu::BlendOperation::Add;
			bs.alpha.operation = wgpu::BlendOperation::Add;
			switch (blending)
			{
				case cage::BlendingEnum::None:
					bs.color.srcFactor = bs.alpha.srcFactor = wgpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = wgpu::BlendFactor::Zero;
					break;
				case cage::BlendingEnum::Additive:
					bs.color.srcFactor = bs.alpha.srcFactor = wgpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = wgpu::BlendFactor::One;
					break;
				case cage::BlendingEnum::AlphaTransparency:
					bs.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
					bs.alpha.srcFactor = wgpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
					break;
				case cage::BlendingEnum::PremultipliedTransparency:
					bs.color.srcFactor = bs.alpha.srcFactor = wgpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
					break;
			}
			return bs;
		}

		wgpu::PrimitiveTopology convertTopology(const Model *model)
		{
			switch (model->primitiveType)
			{
				case 1:
					return wgpu::PrimitiveTopology::PointList;
				case 2:
					return wgpu::PrimitiveTopology::LineList;
				case 3:
				default:
					return wgpu::PrimitiveTopology::TriangleList;
			}
		}

		wgpu::CompareFunction convertDepthTest(const DepthTestEnum &dt)
		{
			static_assert((uint32)wgpu::CompareFunction::Undefined == (uint32)DepthTestEnum::None);
			static_assert((uint32)wgpu::CompareFunction::Never == (uint32)DepthTestEnum::Never);
			static_assert((uint32)wgpu::CompareFunction::Less == (uint32)DepthTestEnum::Less);
			static_assert((uint32)wgpu::CompareFunction::Equal == (uint32)DepthTestEnum::Equal);
			static_assert((uint32)wgpu::CompareFunction::LessEqual == (uint32)DepthTestEnum::LessEqual);
			static_assert((uint32)wgpu::CompareFunction::Greater == (uint32)DepthTestEnum::Greater);
			static_assert((uint32)wgpu::CompareFunction::NotEqual == (uint32)DepthTestEnum::NotEqual);
			static_assert((uint32)wgpu::CompareFunction::GreaterEqual == (uint32)DepthTestEnum::GreaterEqual);
			static_assert((uint32)wgpu::CompareFunction::Always == (uint32)DepthTestEnum::Always);
			return (wgpu::CompareFunction)(uint32)dt;
		}
	}

	namespace privat
	{
		void logImpl(SeverityEnum severity, wgpu::StringView message);

		struct DevicePipelinesCache : private Immovable
		{
		public:
			Holder<RwMutex> mutex = newRwMutex();
			GraphicsDevice *device = nullptr;
			uint32 currentFrame = 0;

			struct Key : public PipelineConfig
			{
				std::size_t hash = 0;

				Key(const PipelineConfig &config) : PipelineConfig(config)
				{
					auto hashCombine = [&](std::unsigned_integral auto v) { hash ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2); };
					hashCombine((uintPtr)shader);
					hashCombine((uint32)blending);
					hashCombine((uint32)depthTest);
					hashCombine((uint32(depthWrite) << 1) + uint32(backFaceCulling));
					for (const auto &it : bindingsLayouts)
						hashCombine((uintPtr)it.Get());
					for (wgpu::TextureFormat f : colorTargets)
						hashCombine((uint32)f);
					hashCombine((uint32)meshComponents); // replacement for vertexBufferLayout for the purpose of the key/hash
					hashCombine((uint32)primitiveTopology);
					hashCombine((uint32)depthFormat);
				}

				bool operator==(const Key &) const = default;
			};

			struct Hash
			{
				std::size_t operator()(const Key &key) const { return key.hash; }
			};

			struct Value
			{
				wgpu::RenderPipeline pipeline;
				uint32 lastUsedFrame = 0;
				bool creating = false;
			};

			std::unordered_map<Key, Value, Hash> cache;

			DevicePipelinesCache(GraphicsDevice *device) : device(device) {}

			~DevicePipelinesCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics pipelines cache size: " + cache.size()); }

			void createPipeline(const PipelineConfig &config, Value *target)
			{
				wgpu::DepthStencilState dss = {};
				if (config.depthFormat != wgpu::TextureFormat::Undefined)
				{
					dss.format = config.depthFormat;
					dss.depthCompare = convertDepthTest(config.depthTest);
					dss.depthWriteEnabled = config.depthWrite;
				}

				wgpu::BlendState blendState = convertBlending(config.blending);
				ankerl::svector<wgpu::ColorTargetState, 1> colors;
				colors.reserve(config.colorTargets.size());
				for (wgpu::TextureFormat ct : config.colorTargets)
				{
					wgpu::ColorTargetState cts = {};
					cts.format = ct;
					if (config.blending != BlendingEnum::None)
						cts.blend = &blendState;
					colors.push_back(cts);
				}

				wgpu::FragmentState fs = {};
				fs.module = config.shader->nativeFragment();
				fs.targetCount = colors.size();
				fs.targets = colors.data();

				wgpu::RenderPipelineDescriptor rpd = {};
				rpd.vertex.module = config.shader->nativeVertex();
				rpd.vertex.bufferCount = 1;
				rpd.vertex.buffers = &config.vertexBufferLayout;
				rpd.primitive.cullMode = config.backFaceCulling ? wgpu::CullMode::Back : wgpu::CullMode::None;
				rpd.primitive.topology = config.primitiveTopology;
				if (config.depthFormat != wgpu::TextureFormat::Undefined)
					rpd.depthStencil = &dss;
				rpd.fragment = &fs;

				wgpu::PipelineLayoutDescriptor pld = {};
				pld.bindGroupLayoutCount = config.bindingsLayouts.size();
				pld.bindGroupLayouts = config.bindingsLayouts.data();
				Holder<wgpu::Device> dev = device->nativeDevice();
				rpd.layout = dev->CreatePipelineLayout(&pld);
				dev->CreateRenderPipelineAsync(&rpd, wgpu::CallbackMode::AllowProcessEvents,
					[this, target](wgpu::CreatePipelineAsyncStatus status, wgpu::RenderPipeline pipeline, wgpu::StringView message)
					{
						if (status == wgpu::CreatePipelineAsyncStatus::Success)
						{
							ScopeLock lock(mutex, WriteLockTag());
							target->pipeline = pipeline;
							target->creating = false;
						}
						else
						{
							CAGE_LOG(SeverityEnum::Warning, "graphics", "error creating wgpu pipeline");
							logImpl(SeverityEnum::Note, message);
							//detail::debugBreakpoint();
						}
					});
			}

			wgpu::RenderPipeline getPipeline(const PipelineConfig &config)
			{
				const Key key(config);

				{
					ScopeLock lock(mutex, ReadLockTag());
					const auto it = cache.find(key);
					if (it != cache.end())
					{
						it->second.lastUsedFrame = currentFrame;
						return it->second.pipeline;
					}
				}

				{
					ScopeLock lock(mutex, WriteLockTag());
					auto &it = cache[key];
					it.lastUsedFrame = currentFrame;
					if (!it.pipeline && !it.creating) // must check again after relocking
					{
						it.creating = true;
						createPipeline(config, &it);
					}
					return it.pipeline;
				}
			}

			void nextFrame()
			{
				ScopeLock lock(mutex, WriteLockTag());
				currentFrame++;
				std::erase_if(cache, [&](const auto &it) { return !it.second.creating && it.second.lastUsedFrame + 60 * 60 < currentFrame; });
			}
		};

		Holder<DevicePipelinesCache> newDevicePipelinesCache(GraphicsDevice *device)
		{
			return systemMemory().createHolder<DevicePipelinesCache>(device);
		}

		void deviceCacheNextFrame(DevicePipelinesCache *cache)
		{
			cache->nextFrame();
		}

		DevicePipelinesCache *getDevicePipelinesCache(GraphicsDevice *device);
	}

	wgpu::RenderPipeline newGraphicsPipeline(GraphicsDevice *device, const PipelineConfig &config)
	{
		return privat::getDevicePipelinesCache(device)->getPipeline(config);
	}

	PipelineConfig convertPipelineConfig(const RenderPassConfig &pass, const DrawConfig &draw)
	{
		CAGE_ASSERT(pass.bindings);
		CAGE_ASSERT(draw.material);
		CAGE_ASSERT(draw.bindings);
		CAGE_ASSERT(draw.model);
		PipelineConfig result;
		result.bindingsLayouts.resize(3);
		result.bindingsLayouts[0] = pass.bindings.layout;
		result.bindingsLayouts[1] = draw.material.layout;
		result.bindingsLayouts[2] = draw.bindings.layout;
		for (const auto &it : pass.colorTargets)
			result.colorTargets.push_back(it.texture->nativeTexture().GetFormat());
		result.vertexBufferLayout = draw.model->getLayout();
		result.primitiveTopology = convertTopology(draw.model);
		if (pass.depthTarget)
			result.depthFormat = pass.depthTarget->texture->nativeTexture().GetFormat();
		result.meshComponents = draw.model->components;
		(GraphicsPipelineCommonConfig &)result = (const GraphicsPipelineCommonConfig &)draw;
		return result;
	}
}
