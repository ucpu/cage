#include <array>

#include <webgpu/webgpu_cpp.h>

#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace privat
	{
		wgpu::TextureViewDimension textureViewDimension(TextureFlags flags)
		{
			if (any(flags & TextureFlags::Volume3D))
				return wgpu::TextureViewDimension::e3D;
			if (any(flags & TextureFlags::Cubemap))
				return any(flags & TextureFlags::Array) ? wgpu::TextureViewDimension::CubeArray : wgpu::TextureViewDimension::Cube;
			return any(flags & TextureFlags::Array) ? wgpu::TextureViewDimension::e2DArray : wgpu::TextureViewDimension::e2D;
		}

		struct DeviceTexturesCache : private Immovable
		{
		public:
			Holder<RwMutex> mutex = newRwMutex();
			Holder<Texture> dummy2d, dummyArray, dummyCube, shadowsSampler;
			GraphicsDevice *device = nullptr;
			uint32 currentFrame = 1;

			struct Key : TransientTextureCreateConfig
			{
				std::size_t hash = 0;

				Key(const TransientTextureCreateConfig &config) : TransientTextureCreateConfig(config)
				{
					auto hashCombine = [&](std::unsigned_integral auto v) { hash ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2); };
					hashCombine((uint32)HashString(name));
					hashCombine((uint32)config.resolution[0]);
					hashCombine((uint32)config.resolution[1]);
					hashCombine((uint32)config.resolution[2]);
					hashCombine(config.mipLevelCount);
					hashCombine((uint32)config.format);
					hashCombine((uint64)config.flags);
					hashCombine(config.entityId);
					hashCombine(config.samplerVariant);
				}

				bool operator==(const Key &) const = default;
			};

			struct Hash
			{
				std::size_t operator()(const Key &key) const { return key.hash; }
			};

			struct Value
			{
				Holder<Texture> texture;
				uint32 lastUsedFrame = 0;
			};

			std::unordered_map<Key, Value, Hash> cache;

			void generateDummyTextures()
			{
				ColorTextureCreateConfig cfg;
				cfg.resolution = Vec3i(1);
				cfg.channels = 4;
				cfg.mipLevels = 1;
				dummy2d = newTexture(device, cfg, "dummy2d");
				cfg.flags = TextureFlags::Array;
				dummyArray = newTexture(device, cfg, "dummyArray");
				cfg.resolution[2] = 6;
				cfg.flags = TextureFlags::Cubemap;
				dummyCube = newTexture(device, cfg, "dummyCube");

				wgpu::TexelCopyTextureInfo dest = {};
				dest.texture = dummy2d->nativeTexture();
				dest.mipLevel = 0;
				dest.aspect = wgpu::TextureAspect::All;
				wgpu::TexelCopyBufferLayout layout = {};
				layout.bytesPerRow = 4;
				layout.rowsPerImage = 1;
				wgpu::Extent3D extents = { 1, 1, 1 };
				static constexpr std::array<unsigned char, 4 * 6> data = {
					0,
					0,
					0,
					255,
					0,
					0,
					0,
					255,
					0,
					0,
					0,
					255,
					0,
					0,
					0,
					255,
					0,
					0,
					0,
					255,
					0,
					0,
					0,
					255,
				};
				device->nativeQueue()->WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
				dest.texture = dummyArray->nativeTexture();
				device->nativeQueue()->WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
				dest.texture = dummyCube->nativeTexture();
				extents.depthOrArrayLayers = 6;
				device->nativeQueue()->WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
			}

			void generateShadowsSampler()
			{
				wgpu::TextureDescriptor desc = {};
				desc.size.width = 1;
				desc.size.height = 1;
				desc.size.depthOrArrayLayers = 1;
				desc.format = wgpu::TextureFormat::Depth32Float;
				desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
				desc.label = "dummy shadowmap target";
				wgpu::Texture tex = device->nativeDevice()->CreateTexture(&desc);
				wgpu::TextureViewDescriptor vd = {};
				vd.dimension = wgpu::TextureViewDimension::e2DArray;
				wgpu::TextureView view = tex.CreateView(&vd);
				wgpu::Sampler samp = device->nativeDevice()->CreateSampler();
				Holder<Texture> t = newTexture(tex, view, samp, "dummy shadowmap target");
				t->flags = TextureFlags::Array;
				shadowsSampler = std::move(t);
			}

			DeviceTexturesCache(GraphicsDevice *device) : device(device)
			{
				generateDummyTextures();
				generateShadowsSampler();
			}

			~DeviceTexturesCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics textures cache size: " + cache.size()); }

			Holder<Texture> createTexture(const TransientTextureCreateConfig &config)
			{
				wgpu::TextureDescriptor desc = {};
				desc.size.width = config.resolution[0];
				desc.size.height = config.resolution[1];
				desc.size.depthOrArrayLayers = config.resolution[2];
				desc.mipLevelCount = config.mipLevelCount;
				desc.format = config.format;
				desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
				desc.label = config.name.c_str();
				wgpu::Texture tex = device->nativeDevice()->CreateTexture(&desc);
				wgpu::TextureViewDescriptor vd = {};
				vd.dimension = textureViewDimension(config.flags);
				wgpu::TextureView view = tex.CreateView(&vd);
				wgpu::SamplerDescriptor sd = {};
				if (config.samplerVariant)
				{
					sd.addressModeU = sd.addressModeV = sd.addressModeW = wgpu::AddressMode::ClampToEdge;
					sd.magFilter = sd.minFilter = wgpu::FilterMode::Linear;
					sd.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
					sd.label = config.name.c_str();
				}
				wgpu::Sampler samp = device->nativeDevice()->CreateSampler(&sd);
				Holder<Texture> t = newTexture(tex, view, samp, config.name);
				t->flags = config.flags;
				return t;
			}

			Holder<Texture> getTexture(const TransientTextureCreateConfig &config)
			{
				//return createTexture(config);

				const Key key(config);

				{
					ScopeLock lock(mutex, ReadLockTag());
					const auto it = cache.find(key);
					if (it != cache.end())
					{
						it->second.lastUsedFrame = currentFrame;
						return it->second.texture.share();
					}
				}

				{
					ScopeLock lock(mutex, WriteLockTag());
					auto &it = cache[key];
					it.lastUsedFrame = currentFrame;
					if (!it.texture) // must check again after relocking
						it.texture = createTexture(config);
					return it.texture.share();
				}
			}

			void nextFrame()
			{
				ScopeLock lock(mutex, WriteLockTag());
				currentFrame++;
				std::erase_if(cache, [&](const auto &it) { return it.second.lastUsedFrame + 2 < currentFrame; });
			}
		};

		Holder<DeviceTexturesCache> newDeviceTexturesCache(GraphicsDevice *device)
		{
			return systemMemory().createHolder<DeviceTexturesCache>(device);
		}

		void deviceCacheNextFrame(DeviceTexturesCache *cache)
		{
			cache->nextFrame();
		}

		DeviceTexturesCache *getDeviceTexturesCache(GraphicsDevice *device);

		Texture *getTextureDummy2d(GraphicsDevice *device)
		{
			return +getDeviceTexturesCache(device)->dummy2d;
		}

		Texture *getTextureDummyArray(GraphicsDevice *device)
		{
			return +getDeviceTexturesCache(device)->dummyArray;
		}

		Texture *getTextureDummyCube(GraphicsDevice *device)
		{
			return +getDeviceTexturesCache(device)->dummyCube;
		}

		Texture *getTextureShadowsSampler(GraphicsDevice *device)
		{
			return +getDeviceTexturesCache(device)->shadowsSampler;
		}
	}

	namespace
	{
		wgpu::TextureFormat findFormat(ImageFormatEnum format, uint32 channels, bool srgb)
		{
			using F = wgpu::TextureFormat;

			if (srgb)
			{
				if (format == ImageFormatEnum::U8)
				{
					switch (channels)
					{
						case 3:
						case 4:
							return F::RGBA8UnormSrgb;
					}
				}
			}
			else
			{
				switch (format)
				{
					case ImageFormatEnum::U8:
					{
						switch (channels)
						{
							case 1:
								return F::R8Unorm;
							case 2:
								return F::RG8Unorm;
							case 3:
							case 4:
								return F::RGBA8Unorm;
						}
						break;
					}
					case ImageFormatEnum::U16:
					{
						switch (channels)
						{
							case 1:
								return F::R16Unorm;
							case 2:
								return F::RG16Unorm;
							case 3:
							case 4:
								return F::RGBA16Unorm;
						}
						break;
					}
					case ImageFormatEnum::Float:
					{
						switch (channels)
						{
							case 1:
								return F::R32Float;
							case 2:
								return F::RG32Float;
							case 3:
							case 4:
								return F::RGBA32Float;
						}
						break;
					}
					case ImageFormatEnum::Default:
						break;
				}
			}

			CAGE_THROW_ERROR(Exception, "image format/channels/srgb combination not supported as a texture");
		}

		class TextureImpl : public Texture
		{
		public:
			wgpu::Texture texture;
			wgpu::TextureView view;
			wgpu::Sampler sampler;

			TextureImpl(GraphicsDevice *device, const ColorTextureCreateConfig &config, const AssetLabel &label_)
			{
				this->label = label_;
				this->flags = config.flags;
				wgpu::TextureDescriptor desc = {};
				desc.usage = wgpu::TextureUsage::CopyDst;
				if (config.sampling)
					desc.usage |= wgpu::TextureUsage::TextureBinding;
				if (config.renderable)
					desc.usage |= wgpu::TextureUsage::RenderAttachment;
				desc.dimension = any(config.flags & TextureFlags::Volume3D) ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D;
				desc.size.width = config.resolution[0];
				desc.size.height = config.resolution[1];
				desc.size.depthOrArrayLayers = config.resolution[2];
				desc.format = findFormat(ImageFormatEnum::U8, config.channels, any(config.flags & TextureFlags::Srgb));
				desc.mipLevelCount = config.mipLevels;
				desc.label = label.c_str();
				Holder<wgpu::Device> dev = device->nativeDevice();
				texture = dev->CreateTexture(&desc);
				wgpu::TextureViewDescriptor twd = {};
				twd.dimension = privat::textureViewDimension(config.flags);
				view = texture.CreateView(&twd);
				sampler = dev->CreateSampler();
			}

			TextureImpl(wgpu::Texture texture, wgpu::TextureView view, wgpu::Sampler sampler, const AssetLabel &label_) : texture(texture), view(view), sampler(sampler) { this->label = label_; }

			Vec3i mipRes(uint32 mip) const
			{
				CAGE_ASSERT(mip < texture.GetMipLevelCount());
				const Vec3i r = resolution3();
				return Vec3i(max(r[0] >> mip, 1), max(r[1] >> mip, 1), max(r[2] >> (texture.GetDimension() == wgpu::TextureDimension::e3D ? mip : 0), 1));
			}
		};
	}

	Vec2i Texture::resolution() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return Vec2i(impl->texture.GetWidth(), impl->texture.GetHeight());
	}

	Vec3i Texture::resolution3() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return Vec3i(impl->texture.GetWidth(), impl->texture.GetHeight(), impl->texture.GetDepthOrArrayLayers());
	}

	uint32 Texture::mipLevels() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return impl->texture.GetMipLevelCount();
	}

	Vec2i Texture::mipResolution(uint32 mipmapLevel) const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return Vec2i(impl->mipRes(mipmapLevel));
	}

	Vec3i Texture::mipResolution3(uint32 mipmapLevel) const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return impl->mipRes(mipmapLevel);
	}

	const wgpu::Texture &Texture::nativeTexture()
	{
		TextureImpl *impl = (TextureImpl *)this;
		return impl->texture;
	}

	const wgpu::TextureView &Texture::nativeView()
	{
		TextureImpl *impl = (TextureImpl *)this;
		return impl->view;
	}

	const wgpu::Sampler &Texture::nativeSampler()
	{
		TextureImpl *impl = (TextureImpl *)this;
		return impl->sampler;
	}

	Holder<Texture> newTexture(GraphicsDevice *device, const ColorTextureCreateConfig &config, const AssetLabel &label)
	{
		return systemMemory().createImpl<Texture, TextureImpl>(device, config, label);
	}

	Holder<Texture> newTexture(GraphicsDevice *device, const TransientTextureCreateConfig &config)
	{
		return privat::getDeviceTexturesCache(device)->getTexture(config);
	}

	Holder<Texture> newTexture(GraphicsDevice *device, const Image *image, const AssetLabel &label)
	{
		if (image->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for texture (not U8)");
		ColorTextureCreateConfig conf;
		conf.resolution = Vec3i(image->resolution(), 1);
		conf.channels = image->channels();
		if (image->colorConfig.gammaSpace == GammaSpaceEnum::Gamma)
			conf.flags = TextureFlags::Srgb;
		Holder<Texture> tex = newTexture(device, conf, label);
		wgpu::TexelCopyTextureInfo dest = {};
		dest.texture = tex->nativeTexture();
		dest.aspect = wgpu::TextureAspect::All;
		wgpu::TexelCopyBufferLayout layout = {};
		layout.bytesPerRow = image->width() * image->channels();
		layout.rowsPerImage = image->height();
		const wgpu::Extent3D extents = { image->width(), image->height(), 1 };
		const auto data = image->rawViewU8();
		device->nativeQueue()->WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
		return tex;
	}

	Holder<Texture> newTexture(wgpu::Texture texture, wgpu::TextureView view, wgpu::Sampler sampler, const AssetLabel &label)
	{
		return systemMemory().createImpl<Texture, TextureImpl>(texture, view, sampler, label);
	}
}
