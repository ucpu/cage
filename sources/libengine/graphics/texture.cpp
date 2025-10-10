#include <webgpu/webgpu_cpp.h>

#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		class TextureImpl : public Texture
		{
		public:
			wgpu::Texture texture;
			wgpu::TextureView view;

			TextureImpl(wgpu::Texture texture) : texture(texture) { view = texture.CreateView(); }

			TextureImpl(GraphicsDevice *device, const TextureCreateConfig &config)
			{
				wgpu::TextureDescriptor desc;
				desc.dimension = config.volume3D ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D;
				desc.mipLevelCount = config.mipLevels;
				desc.size.width = config.resolution[0];
				desc.size.height = config.resolution[1];
				desc.size.depthOrArrayLayers = config.resolution[2];
				texture = device->nativeDevice().CreateTexture(&desc);
				view = texture.CreateView();
			}

			Vec3i mipRes(uint32 mip) const
			{
				CAGE_ASSERT(mip < texture.GetMipLevelCount());
				const Vec3i r = resolution3();
				return Vec3i(max(r[0] >> mip, 1), max(r[1] >> mip, 1), max(r[2] >> (texture.GetDimension() == wgpu::TextureDimension::e3D ? mip : 0), 1));
			}
		};
	}

	Holder<Texture> adaptWgpuTexture(wgpu::Texture texture)
	{
		return systemMemory().createImpl<Texture, TextureImpl>(texture);
	}

	void Texture::setLabel(const String &name)
	{
		label = name;
		TextureImpl *impl = (TextureImpl *)this;
		impl->texture.SetLabel(name.c_str());
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

	wgpu::Texture Texture::nativeTexture()
	{
		TextureImpl *impl = (TextureImpl *)this;
		return impl->texture;
	}

	wgpu::TextureView Texture::nativeView()
	{
		TextureImpl *impl = (TextureImpl *)this;
		return impl->view;
	}

	Holder<Texture> newGraphicsTexture(GraphicsDevice *device, const TextureCreateConfig &config)
	{
		return systemMemory().createImpl<Texture, TextureImpl>(device, config);
	}
}
