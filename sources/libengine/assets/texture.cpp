#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/assetsStructs.h>
#include <cage-engine/gpuCore.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace privat
	{
		gpu::TextureViewDimension textureViewDimension(TextureFlags flags);
	}

	namespace detail
	{
		CAGE_API_EXPORT void textureLoadLevel(gpu::Queue &queue, gpu::Texture &tex, const TextureHeader &header, const Vec3i &resolution, const uint32 mipLevel, PointerRange<const char> data)
		{
			CAGE_ASSERT(mipLevel > 0 || Vec2i(resolution) == Vec2i(header.resolution));
			CAGE_ASSERT(header.channels == 1 || header.channels == 2 || header.channels == 4);

			gpu::TexelCopyTextureInfo dest = {};
			dest.texture = tex;
			dest.mipLevel = mipLevel;
			dest.aspect = gpu::TextureAspect::All;

			uint32 blockWidth = 1;
			uint32 blockBytes = header.channels; // for uncompressed formats
			switch ((gpu::TextureFormat)header.format)
			{
				case gpu::TextureFormat::BC1RGBAUnorm:
				case gpu::TextureFormat::BC1RGBAUnormSrgb:
				case gpu::TextureFormat::BC4RUnorm:
				case gpu::TextureFormat::BC4RSnorm:
					blockWidth = 4;
					blockBytes = 8;
					break;
				case gpu::TextureFormat::BC2RGBAUnorm:
				case gpu::TextureFormat::BC2RGBAUnormSrgb:
				case gpu::TextureFormat::BC3RGBAUnorm:
				case gpu::TextureFormat::BC3RGBAUnormSrgb:
				case gpu::TextureFormat::BC5RGUnorm:
				case gpu::TextureFormat::BC5RGSnorm:
				case gpu::TextureFormat::BC6HRGBUfloat:
				case gpu::TextureFormat::BC6HRGBFloat:
				case gpu::TextureFormat::BC7RGBAUnorm:
				case gpu::TextureFormat::BC7RGBAUnormSrgb:
					blockWidth = 4;
					blockBytes = 16;
					break;
				default:
					break;
			}
			CAGE_ASSERT((resolution[0] % blockWidth) == 0);
			CAGE_ASSERT((resolution[1] % blockWidth) == 0);

			gpu::TexelCopyBufferLayout layout = {};
			layout.bytesPerRow = ((resolution[0] + blockWidth - 1) / blockWidth) * blockBytes;
			layout.rowsPerImage = (resolution[1] + blockWidth - 1) / blockWidth;

			const uint32 copyWidth = max(blockWidth, (uint32)resolution[0]);
			const uint32 copyHeight = max(blockWidth, (uint32)resolution[1]);
			gpu::Extent3D extents = { copyWidth, copyHeight, numeric_cast<uint32>(resolution[2]) };

			queue.WriteTexture(dest, data, layout, extents);
		}
	}

	namespace
	{
		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader header;
			des >> header;

			Holder<gpu::Device> dev = ((GraphicsDevice *)context->device)->nativeDevice();

			gpu::TextureDescriptor desc = {};
			desc.dimension = any(header.flags & TextureFlags::Volume3D) ? gpu::TextureDimension::e3D : gpu::TextureDimension::e2D;
			static_assert(sizeof(desc.usage) == sizeof(header.usage));
			desc.usage = (gpu::TextureUsage)header.usage;
			static_assert(sizeof(desc.format) == sizeof(header.format));
			desc.format = (gpu::TextureFormat)header.format;
			desc.mipLevelCount = header.mipLevels;
			desc.size = { numeric_cast<uint32>(header.resolution[0]), numeric_cast<uint32>(header.resolution[1]), numeric_cast<uint32>(header.resolution[2]) };
			desc.label = context->textId.c_str();
			gpu::Texture wtex = dev->CreateTexture(desc);

			gpu::TextureViewDescriptor twd = {};
			twd.dimension = privat::textureViewDimension(header.flags);
			twd.label = context->textId.c_str();
			gpu::TextureView view = wtex.CreateView(twd);

			gpu::SamplerDescriptor sd = {};
			static_assert(sizeof(sd.magFilter) == sizeof(header.sampleFilter));
			sd.magFilter = (gpu::FilterMode)header.sampleFilter;
			sd.minFilter = (gpu::FilterMode)header.sampleFilter;
			static_assert(sizeof(sd.mipmapFilter) == sizeof(header.mipmapFilter));
			sd.mipmapFilter = (gpu::MipmapFilterMode)header.mipmapFilter;
			sd.maxAnisotropy = header.anisoFilter;
			static_assert(sizeof(sd.addressModeU) == sizeof(header.wrapX));
			sd.addressModeU = (gpu::AddressMode)header.wrapX;
			sd.addressModeV = (gpu::AddressMode)header.wrapY;
			sd.addressModeW = (gpu::AddressMode)header.wrapZ;
			sd.label = context->textId.c_str();
			gpu::Sampler samp = dev->CreateSampler(sd);

			dev.clear();
			Holder<Texture> tex = newTexture(wtex, view, samp, context->textId);
			tex->flags = header.flags;

			Holder<gpu::Queue> queue = ((GraphicsDevice *)context->device)->nativeQueue();
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
