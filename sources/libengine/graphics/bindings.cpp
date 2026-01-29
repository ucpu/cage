#include <svector.h>

#include <cage-core/assetsManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-engine/graphicsBindings.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/model.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace privat
	{
		wgpu::TextureViewDimension textureViewDimension(TextureFlags flags);

		namespace
		{
			bool isFormatFilterable(wgpu::TextureFormat format)
			{
				switch (format)
				{
					// depth/stencil
					case wgpu::TextureFormat::Depth16Unorm:
					case wgpu::TextureFormat::Depth24Plus:
					case wgpu::TextureFormat::Depth24PlusStencil8:
					case wgpu::TextureFormat::Depth32Float:
					case wgpu::TextureFormat::Depth32FloatStencil8:
					// high-p floats
					case wgpu::TextureFormat::R32Float:
					case wgpu::TextureFormat::RG32Float:
					case wgpu::TextureFormat::RGBA32Float:
					// integers
					case wgpu::TextureFormat::R8Sint:
					case wgpu::TextureFormat::R8Uint:
					case wgpu::TextureFormat::RG8Sint:
					case wgpu::TextureFormat::RG8Uint:
					case wgpu::TextureFormat::RGBA8Sint:
					case wgpu::TextureFormat::RGBA8Uint:
					case wgpu::TextureFormat::R16Sint:
					case wgpu::TextureFormat::R16Uint:
					case wgpu::TextureFormat::RG16Sint:
					case wgpu::TextureFormat::RG16Uint:
					case wgpu::TextureFormat::RGBA16Sint:
					case wgpu::TextureFormat::RGBA16Uint:
					case wgpu::TextureFormat::R32Sint:
					case wgpu::TextureFormat::R32Uint:
					case wgpu::TextureFormat::RG32Sint:
					case wgpu::TextureFormat::RG32Uint:
					case wgpu::TextureFormat::RGBA32Sint:
					case wgpu::TextureFormat::RGBA32Uint:
						return false;
					default:
						return true;
				}
			}

			wgpu::BindGroupLayout createLayout(GraphicsDevice *device, const GraphicsBindingsCreateConfig &config, AssetLabel label)
			{
				ankerl::svector<wgpu::BindGroupLayoutEntry, 10> entries;
				entries.reserve(config.buffers.size() + config.textures.size() * 2);

				for (const auto &b : config.buffers)
				{
					CAGE_ASSERT(b.buffer && b.buffer->nativeBuffer());
					wgpu::BindGroupLayoutEntry e = {};
					e.binding = b.binding;
					e.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
					e.buffer.type = b.uniform ? wgpu::BufferBindingType::Uniform : wgpu::BufferBindingType::ReadOnlyStorage;
					e.buffer.hasDynamicOffset = b.dynamic;
					entries.push_back(e);
				}

				for (const auto &t : config.textures)
				{
					CAGE_ASSERT(t.texture && t.texture->nativeTexture() && t.texture->nativeView() && t.texture->nativeSampler());
					CAGE_ASSERT(t.bindTexture || t.bindSampler);
					const bool filterable = isFormatFilterable(t.texture->nativeTexture().GetFormat());
					if (t.bindTexture)
					{
						wgpu::BindGroupLayoutEntry e = {};
						e.binding = t.binding;
						e.visibility = wgpu::ShaderStage::Fragment;
						e.texture.sampleType = filterable ? wgpu::TextureSampleType::Float : wgpu::TextureSampleType::UnfilterableFloat;
						e.texture.viewDimension = textureViewDimension(t.texture->flags);
						entries.push_back(e);
					}
					if (t.bindSampler)
					{
						wgpu::BindGroupLayoutEntry e = {};
						e.binding = t.binding + (t.bindTexture ? 1 : 0);
						e.visibility = wgpu::ShaderStage::Fragment;
						e.sampler.type = filterable ? wgpu::SamplerBindingType::Filtering : wgpu::SamplerBindingType::NonFiltering;
						entries.push_back(e);
					}
				}

				wgpu::BindGroupLayoutDescriptor desc = {};
				desc.entryCount = entries.size();
				desc.entries = entries.data();
				if (label.empty())
					label = Stringizer() + "layout (b: " + config.buffers.size() + ", t: " + config.textures.size() + ")";
				desc.label = label.c_str();
				return device->nativeDevice()->CreateBindGroupLayout(&desc);
			}

			wgpu::BindGroup createGroup(GraphicsDevice *device, const wgpu::BindGroupLayout &layout, const GraphicsBindingsCreateConfig &config, const AssetLabel &label)
			{
				CAGE_ASSERT(layout);

				ankerl::svector<wgpu::BindGroupEntry, 10> entries;
				entries.reserve(config.buffers.size() + config.textures.size() * 2);

				for (const auto &b : config.buffers)
				{
					CAGE_ASSERT(b.buffer);
					wgpu::BindGroupEntry e = {};
					e.binding = b.binding;
					e.buffer = b.buffer->nativeBuffer();
					CAGE_ASSERT(e.buffer);
					e.offset = 0;
					e.size = b.size == m ? wgpu::kWholeSize : b.size;
					entries.push_back(e);
				}

				for (const auto &t : config.textures)
				{
					CAGE_ASSERT(t.texture && t.texture->nativeTexture() && t.texture->nativeView() && t.texture->nativeSampler());
					CAGE_ASSERT(t.bindTexture || t.bindSampler);
					if (t.bindTexture)
					{
						wgpu::BindGroupEntry e = {};
						e.binding = t.binding;
						e.textureView = t.texture->nativeView();
						CAGE_ASSERT(e.textureView);
						entries.push_back(e);
					}
					if (t.bindSampler)
					{
						wgpu::BindGroupEntry e = {};
						e.binding = t.binding + (t.bindTexture ? 1 : 0);
						e.sampler = t.texture->nativeSampler();
						CAGE_ASSERT(e.sampler);
						entries.push_back(e);
					}
				}

				wgpu::BindGroupDescriptor bgd = {};
				bgd.layout = layout;
				bgd.entryCount = entries.size();
				bgd.entries = entries.data();
				if (!label.empty())
					bgd.label = label.c_str();
				return device->nativeDevice()->CreateBindGroup(&bgd);
			}
		}

		struct DeviceBindingsCache : private Immovable
		{
		public:
			Holder<RwMutex> mutex = newRwMutex();
			GraphicsBindings bindingDummy;
			GraphicsDevice *device = nullptr;
			uint32 currentFrame = 0;

			struct LayoutKey
			{
				ankerl::svector<uint32, 6> keys;

				std::size_t hash = 0;

				LayoutKey(const GraphicsBindingsCreateConfig &config)
				{
					keys.reserve(config.buffers.size() + 1 + config.textures.size());
					for (const auto &b : config.buffers)
					{
						const uint32 unif = (uint32)b.uniform << 30;
						const uint32 dyn = (uint32)b.dynamic << 31;
						keys.push_back(b.binding + unif + dyn);
					}
					keys.push_back(75431564); // separator
					for (const auto &t : config.textures)
					{
						const uint32 filterable = (uint32)isFormatFilterable(t.texture->nativeTexture().GetFormat()) << 17;
						const uint32 bindTexture = (uint32)t.bindTexture << 18;
						const uint32 bindSampler = (uint32)t.bindSampler << 19;
						const uint32 flags = (uint32)t.texture->flags << 20;
						keys.push_back(t.binding + filterable + bindTexture + bindSampler + flags);
					}

					auto hashCombine = [&](std::unsigned_integral auto v) { hash ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2); };
					for (uint32 k : keys)
						hashCombine(k);
				}

				bool operator==(const LayoutKey &) const = default;
			};

			struct LayoutValue
			{
				wgpu::BindGroupLayout layout;
				uint32 lastUsedFrame = 0;
			};

			struct GroupKey
			{
				ankerl::svector<uint64, 10> keys;

				std::size_t hash = 0;

				GroupKey(const wgpu::BindGroupLayout &layout, const GraphicsBindingsCreateConfig &config)
				{
					keys.reserve(1 + config.buffers.size() + config.textures.size() * 2);
					keys.push_back((uint64)layout.Get());
					for (const auto &b : config.buffers)
					{
						const uint64 ptr = (uint64)b.buffer->nativeBuffer().Get() << 4;
						const uint64 size = (uint64)b.size << 40;
						keys.push_back(b.binding + ptr + size);
					}
					for (const auto &t : config.textures)
					{
						if (t.bindTexture)
						{
							const uint64 ptr = (uint64)t.texture->nativeView().Get() << 4;
							keys.push_back(t.binding + ptr);
						}
						if (t.bindSampler)
						{
							const uint64 ptr = (uint64)t.texture->nativeSampler().Get() << 4;
							keys.push_back(t.binding + ptr);
						}
					}

					auto hashCombine = [&](std::unsigned_integral auto v) { hash ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2); };
					for (uint64 k : keys)
						hashCombine(k);
				}

				bool operator==(const GroupKey &) const = default;
			};

			struct GroupValue
			{
				wgpu::BindGroup group;
				uint32 lastUsedFrame = 0;
			};

			struct Hash
			{
				std::size_t operator()(const LayoutKey &key) const { return key.hash; }
				std::size_t operator()(const GroupKey &key) const { return key.hash; }
			};

			std::unordered_map<LayoutKey, LayoutValue, Hash> layoutsCache;
			std::unordered_map<GroupKey, GroupValue, Hash> groupsCache;

			DeviceBindingsCache(GraphicsDevice *device) : device(device) { bindingDummy = getBindings({}, "emptyBinding"); }

			~DeviceBindingsCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics bindings layouts cache size: " + layoutsCache.size() + ", groups cache size: " + groupsCache.size()); }

			GraphicsBindings getBindings(const GraphicsBindingsCreateConfig &config, const AssetLabel &label)
			{
				GraphicsBindings binding;
				const LayoutKey lk(config);
				{
					ScopeLock lock(mutex, ReadLockTag());
					const auto lit = layoutsCache.find(lk);
					if (lit != layoutsCache.end())
					{
						lit->second.lastUsedFrame = currentFrame;
						binding.layout = lit->second.layout;

						const GroupKey gk(binding.layout, config);
						const auto git = groupsCache.find(gk);
						if (git != groupsCache.end())
						{
							git->second.lastUsedFrame = currentFrame;
							binding.group = git->second.group;
						}
					}
				}
				if (!binding.layout)
				{
					ScopeLock lock(mutex, WriteLockTag());
					auto &it = layoutsCache[lk];
					it.lastUsedFrame = currentFrame;
					if (!it.layout) // must check again after relocking
						it.layout = createLayout(device, config, label); // layout created within lock - it is used as part of key for pipelines, so avoid duplication
					binding.layout = it.layout;
				}
				if (!binding.group)
				{
					const GroupKey gk(binding.layout, config);
					binding.group = createGroup(device, binding.layout, config, label); // create the group outside lock - potential duplicates will be eliminated next frame
					ScopeLock lock(mutex, WriteLockTag());
					auto &it = groupsCache[gk];
					it.lastUsedFrame = currentFrame;
					if (!it.group) // must check again after relocking
						it.group = binding.group;
				}
				CAGE_ASSERT(binding.layout && binding.group);
				for (const auto &it : config.buffers)
					binding.dynamicBuffersCount += it.dynamic;
				return binding;
			}

			void nextFrame()
			{
				ScopeLock lock(mutex, WriteLockTag());
				currentFrame++;
				std::erase_if(layoutsCache, [&](const auto &it) { return it.second.lastUsedFrame + 60 * 60 < currentFrame; });
				std::erase_if(groupsCache, [&](const auto &it) { return it.second.lastUsedFrame + 5 < currentFrame; });
			}
		};

		Holder<DeviceBindingsCache> newDeviceBindingsCache(GraphicsDevice *device)
		{
			return systemMemory().createHolder<DeviceBindingsCache>(device);
		}

		void deviceCacheNextFrame(DeviceBindingsCache *cache)
		{
			cache->nextFrame();
		}

		DeviceBindingsCache *getDeviceBindingsCache(GraphicsDevice *device);

		GraphicsBindings getEmptyBindings(GraphicsDevice *device)
		{
			return privat::getDeviceBindingsCache(device)->bindingDummy;
		}
	}

	GraphicsBindings newGraphicsBindings(GraphicsDevice *device, const GraphicsBindingsCreateConfig &config, const AssetLabel &label)
	{
		return privat::getDeviceBindingsCache(device)->getBindings(config, label);
	}

	namespace privat
	{
		Texture *getTextureDummy2d(GraphicsDevice *device);
		Texture *getTextureDummyArray(GraphicsDevice *device);
		Texture *getTextureDummyCube(GraphicsDevice *device);
	}

	std::pair<GraphicsBindings, TextureFlags> newGraphicsBindings(GraphicsDevice *device, const AssetsManager *assets, const Model *model)
	{
		GraphicsBindingsCreateConfig config;
		TextureFlags flags = TextureFlags::None;

		if (model->materialBuffer && model->materialBuffer->size() > 0)
		{
			GraphicsBindingsCreateConfig::BufferBindingConfig bc;
			bc.buffer = +model->materialBuffer;
			bc.binding = 0;
			bc.uniform = true;
			config.buffers.push_back(std::move(bc));
		}

		if (assets)
		{
			config.textures.reserve(MaxTexturesCountPerMaterial);
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
			{
				GraphicsBindingsCreateConfig::TextureBindingConfig tc;
				if (model->textureNames[i])
				{
					tc.texture = +assets->get<Texture>(model->textureNames[i]);
					CAGE_ASSERT(tc.texture);
					flags |= tc.texture->flags | (TextureFlags)(1u << 31); // distinguish no textures from a regular texture
				}
				tc.binding = i * 2 + 1;
				config.textures.push_back(std::move(tc));
			}

			{ // figure out empty textures
				Texture *dummy = nullptr;
				if (any(flags & TextureFlags::Cubemap))
					dummy = privat::getTextureDummyCube(device);
				else if (any(flags & TextureFlags::Array))
					dummy = privat::getTextureDummyArray(device);
				else
					dummy = privat::getTextureDummy2d(device);

				for (auto &t : config.textures)
					if (!t.texture)
						t.texture = dummy;
			}
		}

		return { newGraphicsBindings(device, config, model->getLabel()), flags };
	}
}
