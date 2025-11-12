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

			wgpu::BindGroupLayout createLayout(GraphicsDevice *device, const GraphicsBindingsCreateConfig &config)
			{
				ankerl::svector<wgpu::BindGroupLayoutEntry, 10> entries;
				entries.reserve(config.buffers.size() + config.textures.size() * 2);

				for (const auto &b : config.buffers)
				{
					CAGE_ASSERT(b.buffer && b.buffer->nativeBuffer());
					wgpu::BindGroupLayoutEntry e = {};
					e.binding = b.binding;
					e.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
					e.buffer.type = [type = b.buffer->type()]() -> wgpu::BufferBindingType
					{
						switch (type)
						{
							case 0:
								return wgpu::BufferBindingType::Uniform;
							case 1:
								return wgpu::BufferBindingType::ReadOnlyStorage;
							case 2:
								return wgpu::BufferBindingType::Undefined;
							default:
								return wgpu::BufferBindingType::Undefined;
						}
					}();
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
				String lbl = Stringizer() + "layout (b: " + config.buffers.size() + ", t: " + config.textures.size() + ")";
				desc.label = lbl.c_str();
				return device->nativeDevice()->CreateBindGroupLayout(&desc);
			}

			wgpu::BindGroup createGroup(GraphicsDevice *device, const wgpu::BindGroupLayout &layout, const GraphicsBindingsCreateConfig &config)
			{
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

			struct Key
			{
				ankerl::svector<uint32, 5> keys;

				std::size_t hash = 0;

				Key(const GraphicsBindingsCreateConfig &config)
				{
					keys.reserve(config.buffers.size() + 1 + config.textures.size());
					for (const auto &b : config.buffers)
					{
						const uint32 dyn = (uint32)b.dynamic << 10;
						const uint32 type = (uint32)b.buffer->type() << 11;
						keys.push_back(b.binding + dyn + type);
					}
					keys.push_back(75431564); // separator
					for (const auto &t : config.textures)
					{
						const uint32 filterable = (uint32)isFormatFilterable(t.texture->nativeTexture().GetFormat()) << 10;
						const uint32 bindTexture = (uint32)t.bindTexture << 11;
						const uint32 bindSampler = (uint32)t.bindSampler << 12;
						const uint32 flags = (uint32)t.texture->flags << 13;
						keys.push_back(t.binding + filterable + bindTexture + bindSampler + flags);
					}

					auto hashCombine = [&](std::unsigned_integral auto v) { hash ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2); };
					for (uint32 k : keys)
						hashCombine(k);
				}

				bool operator==(const Key &) const = default;
			};

			struct Hash
			{
				std::size_t operator()(const Key &key) const { return key.hash; }
			};

			struct Value
			{
				wgpu::BindGroupLayout layout;
				uint32 lastUsedFrame = 0;
			};

			std::unordered_map<Key, Value, Hash> cache;

			void generateDummyBindings()
			{
				bindingDummy.layout = getLayout({});
				bindingDummy.layout.SetLabel("emptyBinding");
				bindingDummy.group = createGroup(device, bindingDummy.layout, {});
				bindingDummy.group.SetLabel("emptyBinding");
			}

			DeviceBindingsCache(GraphicsDevice *device) : device(device) { generateDummyBindings(); }

			~DeviceBindingsCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics bindings cache size: " + cache.size()); }

			wgpu::BindGroupLayout getLayout(const GraphicsBindingsCreateConfig &config)
			{
				const Key key(config);

				{
					ScopeLock lock(mutex, ReadLockTag());
					const auto it = cache.find(key);
					if (it != cache.end())
					{
						it->second.lastUsedFrame = currentFrame;
						return it->second.layout;
					}
				}

				{
					ScopeLock lock(mutex, WriteLockTag());
					auto &it = cache[key];
					it.lastUsedFrame = currentFrame;
					if (!it.layout) // must check again after relocking
						it.layout = createLayout(device, config);
					return it.layout;
				}
			}

			void nextFrame()
			{
				ScopeLock lock(mutex, WriteLockTag());
				currentFrame++;
				std::erase_if(cache, [&](const auto &it) { return it.second.lastUsedFrame + 60 * 60 < currentFrame; });
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

	GraphicsBindings newGraphicsBindings(GraphicsDevice *device, const GraphicsBindingsCreateConfig &config)
	{
		GraphicsBindings binding;
		binding.layout = privat::getDeviceBindingsCache(device)->getLayout(config);
		//binding.layout = privat::createLayout(device, config);
		binding.group = privat::createGroup(device, binding.layout, config);
		for (const auto &it : config.buffers)
			binding.dynamicBuffersCount += it.dynamic;
		return binding;
	}

	namespace privat
	{
		Texture *getTextureDummy2d(GraphicsDevice *device);
		Texture *getTextureDummyArray(GraphicsDevice *device);
		Texture *getTextureDummyCube(GraphicsDevice *device);
	}

	void prepareModelBindings(GraphicsDevice *device, const AssetsManager *assets, Model *model)
	{
		CAGE_ASSERT(model);
		GraphicsBindings &b = model->bindings();
		if (b)
			return;

		GraphicsBindingsCreateConfig config;

		if (model->materialBuffer && model->materialBuffer->size() > 0)
		{
			GraphicsBindingsCreateConfig::BufferBindingConfig bc;
			bc.buffer = +model->materialBuffer;
			bc.binding = 0;
			config.buffers.push_back(std::move(bc));
		}

		config.textures.reserve(MaxTexturesCountPerMaterial);
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			GraphicsBindingsCreateConfig::TextureBindingConfig tc;
			if (model->textureNames[i])
			{
				tc.texture = +assets->get<Texture>(model->textureNames[i]);
				CAGE_ASSERT(tc.texture);
			}
			tc.binding = i * 2 + 1;
			config.textures.push_back(std::move(tc));
		}

		{ // figure out empty textures
			TextureFlags flags = TextureFlags::None;
			for (const auto &t : config.textures)
				if (t.texture)
					flags |= t.texture->flags;

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

		b = newGraphicsBindings(device, config);
		b.layout.SetLabel(model->getLabel().c_str());
		b.group.SetLabel(model->getLabel().c_str());
	}
}
