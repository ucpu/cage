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
		gpu::RenderPipelineDescriptor::BlendState convertBlending(BlendingEnum blending)
		{
			gpu::RenderPipelineDescriptor::BlendState bs = {};
			bs.color.operation = gpu::BlendOperationEnum::Add;
			bs.alpha.operation = gpu::BlendOperationEnum::Add;
			switch (blending)
			{
				case cage::BlendingEnum::None:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactorEnum::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactorEnum::Zero;
					break;
				case cage::BlendingEnum::Additive:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactorEnum::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactorEnum::One;
					break;
				case cage::BlendingEnum::AlphaTransparency:
					bs.color.srcFactor = gpu::BlendFactorEnum::SrcAlpha;
					bs.alpha.srcFactor = gpu::BlendFactorEnum::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactorEnum::OneMinusSrcAlpha;
					break;
				case cage::BlendingEnum::PremultipliedTransparency:
					bs.color.srcFactor = bs.alpha.srcFactor = gpu::BlendFactorEnum::One;
					bs.color.dstFactor = bs.alpha.dstFactor = gpu::BlendFactorEnum::OneMinusSrcAlpha;
					break;
			}
			return bs;
		}

		gpu::PrimitiveTopologyEnum convertTopology(const Model *model)
		{
			switch (model->primitiveType)
			{
				case 1:
					return gpu::PrimitiveTopologyEnum::PointList;
				case 2:
					return gpu::PrimitiveTopologyEnum::LineList;
				case 3:
				default:
					return gpu::PrimitiveTopologyEnum::TriangleList;
			}
		}

		gpu::CompareFunctionEnum convertDepthTest(const DepthTestEnum &dt)
		{
			static_assert((uint32)gpu::CompareFunctionEnum::Undefined == (uint32)DepthTestEnum::None);
			static_assert((uint32)gpu::CompareFunctionEnum::Never == (uint32)DepthTestEnum::Never);
			static_assert((uint32)gpu::CompareFunctionEnum::Less == (uint32)DepthTestEnum::Less);
			static_assert((uint32)gpu::CompareFunctionEnum::Equal == (uint32)DepthTestEnum::Equal);
			static_assert((uint32)gpu::CompareFunctionEnum::LessEqual == (uint32)DepthTestEnum::LessEqual);
			static_assert((uint32)gpu::CompareFunctionEnum::Greater == (uint32)DepthTestEnum::Greater);
			static_assert((uint32)gpu::CompareFunctionEnum::NotEqual == (uint32)DepthTestEnum::NotEqual);
			static_assert((uint32)gpu::CompareFunctionEnum::GreaterEqual == (uint32)DepthTestEnum::GreaterEqual);
			static_assert((uint32)gpu::CompareFunctionEnum::Always == (uint32)DepthTestEnum::Always);
			return (gpu::CompareFunctionEnum)(uint32)dt;
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
						hashCombine((uintPtr)it.get());
					for (gpu::TextureFormatEnum f : colorTargets)
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
				gpu::RenderPipelineDescriptor::DepthStencilState dss = {};
				if (config.depthFormat != gpu::TextureFormatEnum::Undefined)
				{
					dss.format = config.depthFormat;
					dss.depthCompare = convertDepthTest(config.depthTest);
					dss.depthWriteEnabled = config.depthWrite;
				}

				const gpu::RenderPipelineDescriptor::BlendState blendState = convertBlending(config.blending);
				ankerl::svector<gpu::RenderPipelineDescriptor::ColorTargetState, 1> colors;
				colors.reserve(config.colorTargets.size());
				for (gpu::TextureFormatEnum ct : config.colorTargets)
				{
					gpu::RenderPipelineDescriptor::ColorTargetState cts = {};
					cts.format = ct;
					if (config.blending != BlendingEnum::None)
						cts.blend = blendState;
					colors.push_back(std::move(cts));
				}

				gpu::RenderPipelineDescriptor::FragmentState fs = {};
				fs.module = config.shader->nativeFragment();
				fs.targets = colors;

				gpu::RenderPipelineDescriptor rpd = {};
				rpd.vertex.module = config.shader->nativeVertex();
				rpd.vertex.buffers = PointerRange<const gpu::VertexBufferLayout>(&config.vertexBufferLayout, &config.vertexBufferLayout + 1);
				rpd.primitive.cullMode = config.backFaceCulling ? gpu::CullModeEnum::Back : gpu::CullModeEnum::None;
				rpd.primitive.topology = config.primitiveTopology;
				if (config.depthFormat != gpu::TextureFormatEnum::Undefined)
					rpd.depthStencil = std::move(dss);
				rpd.fragment = std::move(fs);

				gpu::PipelineLayoutDescriptor pld = {};
				pld.bindGroupLayouts = config.bindingsLayouts;
				Holder<gpu::Device> dev = device->nativeDevice();
				rpd.layout = dev->createPipelineLayout(pld);
				dev->createRenderPipelineAsync(rpd, gpu::CallbackModeEnum::AllowProcessEvents,
					[this, target](gpu::StatusEnum status, gpu::RenderPipeline pipeline, gpu::StringView message)
					{
						if (status == gpu::StatusEnum::Success)
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
			result.colorTargets.push_back(it.texture->nativeTexture().getFormat());
		result.vertexBufferLayout = draw.model->getLayout();
		result.primitiveTopology = convertTopology(draw.model);
		if (pass.depthTarget)
			result.depthFormat = pass.depthTarget->texture->nativeTexture().getFormat();
		result.meshComponents = draw.model->components;
		(GraphicsPipelineCommonConfig &)result = (const GraphicsPipelineCommonConfig &)draw;
		return result;
	}
}
