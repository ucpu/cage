#include <webgpu/webgpu_cpp.h>

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
