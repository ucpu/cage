#include <unordered_map>

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
		gpu::BlendState convertBlending(BlendingEnum blending)
		{
			gpu::BlendState bs = {};
			bs.color.operation = gpu::BlendOperation::Add;
			bs.alpha.operation = gpu::BlendOperation::Add;
			switch (blending)
			{
				case cage::BlendingEnum::None:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactor::Zero;
					break;
				case cage::BlendingEnum::Additive:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactor::One;
					break;
				case cage::BlendingEnum::AlphaTransparency:
					bs.color.srcFactor = gpu::BlendFactor::SrcAlpha;
					bs.alpha.srcFactor = gpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactor::OneMinusSrcAlpha;
					break;
				case cage::BlendingEnum::PremultipliedTransparency:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactor::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactor::OneMinusSrcAlpha;
					break;
			}
			return bs;
		}

		gpu::PrimitiveTopology convertTopology(const Model *model)
		{
			switch (model->primitiveType)
			{
				case 1:
					return gpu::PrimitiveTopology::PointList;
				case 2:
					return gpu::PrimitiveTopology::LineList;
				case 3:
				default:
					return gpu::PrimitiveTopology::TriangleList;
			}
		}

		gpu::CompareFunction convertDepthTest(const DepthTestEnum &dt)
		{
			static_assert((uint32)gpu::CompareFunction::Undefined == (uint32)DepthTestEnum::None);
			static_assert((uint32)gpu::CompareFunction::Never == (uint32)DepthTestEnum::Never);
			static_assert((uint32)gpu::CompareFunction::Less == (uint32)DepthTestEnum::Less);
			static_assert((uint32)gpu::CompareFunction::Equal == (uint32)DepthTestEnum::Equal);
			static_assert((uint32)gpu::CompareFunction::LessEqual == (uint32)DepthTestEnum::LessEqual);
			static_assert((uint32)gpu::CompareFunction::Greater == (uint32)DepthTestEnum::Greater);
			static_assert((uint32)gpu::CompareFunction::NotEqual == (uint32)DepthTestEnum::NotEqual);
			static_assert((uint32)gpu::CompareFunction::GreaterEqual == (uint32)DepthTestEnum::GreaterEqual);
			static_assert((uint32)gpu::CompareFunction::Always == (uint32)DepthTestEnum::Always);
			return (gpu::CompareFunction)(uint32)dt;
		}
	}

	namespace privat
	{
		void logImpl(SeverityEnum severity, gpu::StringView message);

		struct DevicePipelinesCache : private Immovable
		{
		public:
			Holder<RwMutex> mutex = newRwMutex();
			GraphicsDevice *device = nullptr;
			uint32 currentFrame = 0;
			uint32 outstandingCreating = 0;

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
					for (gpu::TextureFormat f : colorTargets)
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
				gpu::RenderPipeline pipeline;
				uint32 lastUsedFrame = 0;
				bool creating = false;
			};

			std::unordered_map<Key, Value, Hash> cache;

			DevicePipelinesCache(GraphicsDevice *device) : device(device) {}

			~DevicePipelinesCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics pipelines cache size: " + cache.size()); }

			void createPipeline(const PipelineConfig &config, Value *target)
			{
				gpu::DepthStencilState dss = {};
				if (config.depthFormat != gpu::TextureFormat::Undefined)
				{
					dss.format = config.depthFormat;
					dss.depthCompare = convertDepthTest(config.depthTest);
					dss.depthWriteEnabled = config.depthWrite;
				}

				gpu::BlendState blendState = convertBlending(config.blending);
				ankerl::svector<gpu::ColorTargetState, 1> colors;
				colors.reserve(config.colorTargets.size());
				for (gpu::TextureFormat ct : config.colorTargets)
				{
					gpu::ColorTargetState cts = {};
					cts.format = ct;
					if (config.blending != BlendingEnum::None)
						cts.blend = &blendState;
					colors.push_back(cts);
				}

				gpu::FragmentState fs = {};
				fs.module = config.shader->nativeFragment();
				fs.targets = colors;

				gpu::RenderPipelineDescriptor rpd = {};
				rpd.vertex.module = config.shader->nativeVertex();
				rpd.vertex.buffers = PointerRange<const gpu::VertexBufferLayout>(&config.vertexBufferLayout, &config.vertexBufferLayout + 1);
				rpd.primitive.cullMode = config.backFaceCulling ? gpu::CullMode::Back : gpu::CullMode::None;
				rpd.primitive.topology = config.primitiveTopology;
				if (config.depthFormat != gpu::TextureFormat::Undefined)
					rpd.depthStencil = &dss;
				rpd.fragment = &fs;

				gpu::PipelineLayoutDescriptor pld = {};
				pld.bindGroupLayouts = config.bindingsLayouts;
				Holder<gpu::Device> dev = device->nativeDevice();
				rpd.layout = dev->CreatePipelineLayout(pld);
				dev->CreateRenderPipelineAsync(rpd, gpu::CallbackMode::AllowProcessEvents,
					[this, target](gpu::CreatePipelineAsyncStatus status, gpu::RenderPipeline pipeline, gpu::StringView message)
					{
						if (status == gpu::CreatePipelineAsyncStatus::Success)
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

			gpu::RenderPipeline getPipeline(const PipelineConfig &config)
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
						outstandingCreating++; // make sure that every new pipeline is reported in at least one frame
						createPipeline(config, &it);
					}
					return it.pipeline;
				}
			}

			uint32 nextFrame()
			{
				ScopeLock lock(mutex, WriteLockTag());
				currentFrame++;
				uint32 tmpCreating = outstandingCreating;
				outstandingCreating = 0;
				std::erase_if(cache,
					[&](const auto &it)
					{
						tmpCreating += it.second.creating;
						return !it.second.creating && it.second.lastUsedFrame + 60 * 60 < currentFrame;
					});
				return tmpCreating;
			}
		};

		Holder<DevicePipelinesCache> newDevicePipelinesCache(GraphicsDevice *device)
		{
			return systemMemory().createHolder<DevicePipelinesCache>(device);
		}

		uint32 deviceCacheNextFrame(DevicePipelinesCache *cache)
		{
			return cache->nextFrame();
		}

		DevicePipelinesCache *getDevicePipelinesCache(GraphicsDevice *device);
	}

	gpu::RenderPipeline newGraphicsPipeline(GraphicsDevice *device, const PipelineConfig &config)
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
