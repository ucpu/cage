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
	namespace
	{
		void textureLoadLevel(wgpu::Queue &queue, wgpu::Texture &tex, const TextureHeader &header, const Vec3i &resolution, const uint32 mipLevel, PointerRange<const char> data)
		{
			CAGE_ASSERT(mipLevel > 0 || Vec2i(resolution) == Vec2i(header.resolution));
			wgpu::TexelCopyTextureInfo dest = {};
			dest.texture = tex;
			dest.mipLevel = mipLevel;
			dest.aspect = wgpu::TextureAspect::All;
			wgpu::TexelCopyBufferLayout layout = {};
			layout.bytesPerRow = resolution[0] * header.channels;
			layout.rowsPerImage = resolution[1];
			const wgpu::Extent3D extents = { numeric_cast<uint32>(resolution[0]), numeric_cast<uint32>(resolution[1]), numeric_cast<uint32>(resolution[2]) };
			queue.WriteTexture(&dest, data.data(), data.size(), &layout, &extents);
		}

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader header;
			des >> header;

			wgpu::TextureDescriptor desc = {};
			desc.dimension = any(header.flags & TextureFlags::Volume3D) ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D;
			static_assert(sizeof(desc.usage) == sizeof(header.usage));
			desc.usage = (wgpu::TextureUsage)header.usage;
			static_assert(sizeof(desc.format) == sizeof(header.format));
			desc.format = (wgpu::TextureFormat)header.format;
			desc.mipLevelCount = header.mipLevels;
			desc.size = { numeric_cast<uint32>(header.resolution[0]), numeric_cast<uint32>(header.resolution[1]), numeric_cast<uint32>(header.resolution[2]) };
			wgpu::Texture wtex = ((GraphicsDevice *)context->device)->nativeDevice().CreateTexture(&desc);
			Holder<Texture> tex = newTexture(wtex);

			Holder<wgpu::Queue> queue = ((GraphicsDevice *)context->device)->nativeQueue();
			CAGE_ASSERT(header.mipLevels > 0);
			for (uint32 mip = 0; mip < header.mipLevels; mip++)
			{
				Vec3i resolution;
				uint32 size = 0;
				des >> resolution >> size;
				textureLoadLevel(*queue, wtex, header, resolution, mip, des.read(size));
			}

			tex->setLabel(context->textId);

			context->assetHolder = std::move(tex).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeTexture(GraphicsDevice *device, uint32 threadIndex)
	{
		AssetsScheme s;
		s.device = device;
		s.threadIndex = threadIndex;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Texture>();
		return s;
	}
}
