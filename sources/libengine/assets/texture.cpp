#include <webgpu/webgpu_cpp.h>

#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/assetsStructs.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace privat
	{
		wgpu::TextureViewDimension textureViewDimension(TextureFlags flags);
	}

	namespace detail
	{
		CAGE_API_EXPORT void textureLoadLevel(wgpu::Queue &queue, wgpu::Texture &tex, const TextureHeader &header, const Vec3i &resolution, const uint32 mipLevel, PointerRange<const char> data)
		{
			CAGE_ASSERT(mipLevel > 0 || Vec2i(resolution) == Vec2i(header.resolution));
			CAGE_ASSERT(header.channels == 1 || header.channels == 2 || header.channels == 4);

			wgpu::TexelCopyTextureInfo dest = {};
			dest.texture = tex;
			dest.mipLevel = mipLevel;
			dest.aspect = wgpu::TextureAspect::All;

			uint32 blockWidth = 1;
			uint32 blockBytes = header.channels; // for uncompressed formats
			switch ((wgpu::TextureFormat)header.format)
			{
				case wgpu::TextureFormat::BC1RGBAUnorm:
				case wgpu::TextureFormat::BC1RGBAUnormSrgb:
				case wgpu::TextureFormat::BC4RUnorm:
				case wgpu::TextureFormat::BC4RSnorm:
					blockWidth = 4;
					blockBytes = 8;
					break;
				case wgpu::TextureFormat::BC2RGBAUnorm:
				case wgpu::TextureFormat::BC2RGBAUnormSrgb:
				case wgpu::TextureFormat::BC3RGBAUnorm:
				case wgpu::TextureFormat::BC3RGBAUnormSrgb:
				case wgpu::TextureFormat::BC5RGUnorm:
				case wgpu::TextureFormat::BC5RGSnorm:
				case wgpu::TextureFormat::BC6HRGBUfloat:
				case wgpu::TextureFormat::BC6HRGBFloat:
				case wgpu::TextureFormat::BC7RGBAUnorm:
				case wgpu::TextureFormat::BC7RGBAUnormSrgb:
					blockWidth = 4;
					blockBytes = 16;
					break;
				default:
					break;
			}
			CAGE_ASSERT((resolution[0] % blockWidth) == 0);
			CAGE_ASSERT((resolution[1] % blockWidth) == 0);

			wgpu::TexelCopyBufferLayout layout = {};
			layout.bytesPerRow = ((resolution[0] + blockWidth - 1) / blockWidth) * blockBytes;
			layout.rowsPerImage = (resolution[1] + blockWidth - 1) / blockWidth;

			const uint32 copyWidth = max(blockWidth, (uint32)resolution[0]);
			const uint32 copyHeight = max(blockWidth, (uint32)resolution[1]);
			wgpu::Extent3D extents = { copyWidth, copyHeight, numeric_cast<uint32>(resolution[2]) };

			queue.WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
		}
	}

	namespace
	{
		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader header;
			des >> header;

			Holder<wgpu::Device> dev = ((GraphicsDevice *)context->device)->nativeDevice();

			wgpu::TextureDescriptor desc = {};
			desc.dimension = any(header.flags & TextureFlags::Volume3D) ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D;
			static_assert(sizeof(desc.usage) == sizeof(header.usage));
			desc.usage = (wgpu::TextureUsage)header.usage;
			static_assert(sizeof(desc.format) == sizeof(header.format));
			desc.format = (wgpu::TextureFormat)header.format;
			desc.mipLevelCount = header.mipLevels;
			desc.size = { numeric_cast<uint32>(header.resolution[0]), numeric_cast<uint32>(header.resolution[1]), numeric_cast<uint32>(header.resolution[2]) };
			desc.label = context->textId.c_str();
			wgpu::Texture wtex = dev->CreateTexture(&desc);

			wgpu::TextureViewDescriptor twd = {};
			twd.dimension = privat::textureViewDimension(header.flags);
			twd.label = context->textId.c_str();
			wgpu::TextureView view = wtex.CreateView(&twd);

			wgpu::SamplerDescriptor sd = {};
			static_assert(sizeof(sd.magFilter) == sizeof(header.sampleFilter));
			sd.magFilter = (wgpu::FilterMode)header.sampleFilter;
			sd.minFilter = (wgpu::FilterMode)header.sampleFilter;
			static_assert(sizeof(sd.mipmapFilter) == sizeof(header.mipmapFilter));
			sd.mipmapFilter = (wgpu::MipmapFilterMode)header.mipmapFilter;
			sd.maxAnisotropy = header.anisoFilter;
			static_assert(sizeof(sd.addressModeU) == sizeof(header.wrapX));
			sd.addressModeU = (wgpu::AddressMode)header.wrapX;
			sd.addressModeV = (wgpu::AddressMode)header.wrapY;
			sd.addressModeW = (wgpu::AddressMode)header.wrapZ;
			sd.label = context->textId.c_str();
			wgpu::Sampler samp = dev->CreateSampler(&sd);

			dev.clear();
			Holder<Texture> tex = newTexture(wtex, view, samp, context->textId);
			tex->flags = header.flags;

			Holder<wgpu::Queue> queue = ((GraphicsDevice *)context->device)->nativeQueue();
			CAGE_ASSERT(header.mipLevels > 0);
			for (uint32 mip = 0; mip < header.mipLevels; mip++)
			{
				Vec3i resolution;
				uint32 size = 0;
				des >> resolution >> size;
				detail::textureLoadLevel(*queue, wtex, header, resolution, mip, des.read(size));
			}

			CAGE_ASSERT(des.available() == 0);

			context->assetHolder = std::move(tex).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeTexture(GraphicsDevice *device)
	{
		AssetsScheme s;
		s.device = device;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Texture>();
		return s;
	}
}
