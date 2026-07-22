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
		gpu::TextureDimensionEnum textureViewDimension(TextureFlags flags);
	}

	namespace detail
	{
		CAGE_API_EXPORT void textureLoadLevel(gpu::Device &device, gpu::Texture &tex, const TextureHeader &header, const Vec3i &resolution, const uint32 arrayLayersOffset, const uint32 arrayLayersCount, const uint32 mipLevel, PointerRange<const char> data)
		{
			CAGE_ASSERT(mipLevel > 0 || Vec2i(resolution) == Vec2i(header.resolution));
			CAGE_ASSERT(header.channels == 1 || header.channels == 2 || header.channels == 4);

			gpu::TexelCopyTextureInfo dest;
			dest.texture = tex;
			dest.mipLevel = mipLevel;
			dest.arrayLayersOffset = arrayLayersOffset;
			dest.arrayLayersCount = arrayLayersCount;

			uint32 blockWidth = 1;
			uint32 blockBytes = header.channels; // for uncompressed formats
			switch ((gpu::TextureFormatEnum)header.format)
			{
				case gpu::TextureFormatEnum::BC1RGBAUnorm:
				case gpu::TextureFormatEnum::BC1RGBAUnormSrgb:
				case gpu::TextureFormatEnum::BC4RUnorm:
				case gpu::TextureFormatEnum::BC4RSnorm:
					blockWidth = 4;
					blockBytes = 8;
					break;
				case gpu::TextureFormatEnum::BC2RGBAUnorm:
				case gpu::TextureFormatEnum::BC2RGBAUnormSrgb:
				case gpu::TextureFormatEnum::BC3RGBAUnorm:
				case gpu::TextureFormatEnum::BC3RGBAUnormSrgb:
				case gpu::TextureFormatEnum::BC5RGUnorm:
				case gpu::TextureFormatEnum::BC5RGSnorm:
				case gpu::TextureFormatEnum::BC6HRGBUfloat:
				case gpu::TextureFormatEnum::BC6HRGBFloat:
				case gpu::TextureFormatEnum::BC7RGBAUnorm:
				case gpu::TextureFormatEnum::BC7RGBAUnormSrgb:
					blockWidth = 4;
					blockBytes = 16;
					break;
				default:
					break;
			}
			CAGE_ASSERT((resolution[0] % blockWidth) == 0);
			CAGE_ASSERT((resolution[1] % blockWidth) == 0);

			gpu::TexelCopyBufferLayout layout;
			layout.bytesPerRow = ((resolution[0] + blockWidth - 1) / blockWidth) * blockBytes;
			layout.rowsPerImage = (resolution[1] + blockWidth - 1) / blockWidth;

			const uint32 copyWidth = max(blockWidth, (uint32)resolution[0]);
			const uint32 copyHeight = max(blockWidth, (uint32)resolution[1]);
			const Vec3i extents = Vec3i(copyWidth, copyHeight, numeric_cast<uint32>(resolution[2]));

			device.writeTexture(dest, data, layout, extents);
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

			gpu::TextureDescriptor desc;
			desc.label = context->textId;
			desc.resolution = header.resolution;
			desc.arrayLayersCount = header.arrayLayersCount;
			desc.mipLevelsCount = header.mipLevelsCount;
			desc.dimension = any(header.flags & TextureFlags::Volume3D) ? gpu::TextureDimensionEnum::e3D : gpu::TextureDimensionEnum::e2D;
			desc.format = header.format;
			desc.usage = header.usage;
			gpu::Texture wtex = dev->createTexture(desc);

			gpu::TextureViewDescriptor twd;
			twd.label = context->textId;
			twd.dimension = privat::textureViewDimension(header.flags);
			gpu::TextureView view = wtex.createView(twd);

			gpu::SamplerDescriptor sd;
			sd.label = context->textId;
			sd.magFilter = header.sampleFilter;
			sd.minFilter = header.sampleFilter;
			sd.mipmapFilter = header.mipmapFilter;
			sd.maxAnisotropy = header.anisoFilter;
			sd.addressModeU = header.wrapX;
			sd.addressModeV = header.wrapY;
			sd.addressModeW = header.wrapZ;
			gpu::Sampler samp = dev->createSampler(sd);

			Holder<Texture> tex = newTexture(wtex, view, samp, context->textId);
			tex->flags = header.flags;

			CAGE_ASSERT(header.mipLevelsCount > 0);
			for (uint32 mip = 0; mip < header.mipLevelsCount; mip++)
			{
				Vec3i resolution;
				uint32 arrayLayersCount = 0;
				uint32 size = 0;
				des >> resolution >> arrayLayersCount >> size;
				detail::textureLoadLevel(*dev, wtex, header, resolution, 0, arrayLayersCount, mip, des.read(size));
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
