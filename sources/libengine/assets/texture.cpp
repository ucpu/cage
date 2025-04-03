#include <cage-core/assetContext.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/opengl.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		uint32 convertSwizzle(TextureSwizzleEnum s)
		{
			switch (s)
			{
				case TextureSwizzleEnum::Zero:
					return GL_ZERO;
				case TextureSwizzleEnum::One:
					return GL_ONE;
				case TextureSwizzleEnum::R:
					return GL_RED;
				case TextureSwizzleEnum::G:
					return GL_GREEN;
				case TextureSwizzleEnum::B:
					return GL_BLUE;
				case TextureSwizzleEnum::A:
					return GL_ALPHA;
				default:
					CAGE_THROW_ERROR(Exception, "invalid TextureSwizzleEnum value");
			}
		}

		void textureLoadLevel(Texture *tex, const TextureHeader &header, uint32 mipmapLevel, PointerRange<const char> values)
		{
			if (any(header.flags & TextureFlags::Compressed))
			{
				if (header.target == GL_TEXTURE_3D || header.target == GL_TEXTURE_2D_ARRAY)
					tex->image3dCompressed(mipmapLevel, values);
				else if (header.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->image2dSliceCompressed(mipmapLevel, f, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
				else
					tex->image2dCompressed(mipmapLevel, values);
			}
			else
			{
				if (header.target == GL_TEXTURE_3D || header.target == GL_TEXTURE_2D_ARRAY)
					tex->image3d(mipmapLevel, header.copyFormat, header.copyType, values);
				else if (header.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->image2dSlice(mipmapLevel, f, header.copyFormat, header.copyType, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
				else
					tex->image2d(mipmapLevel, header.copyFormat, header.copyType, values);
			}
		}

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader header;
			des >> header;

			CAGE_ASSERT(detail::internalFormatIsSrgb(header.internalFormat) == any(header.flags & TextureFlags::Srgb));

			Holder<Texture> tex = newTexture(header.target);
			tex->setDebugName(context->textId);

			tex->filters(header.filterMin, header.filterMag, header.filterAniso);
			tex->wraps(header.wrapX, header.wrapY, header.wrapZ);
			{
				uint32 s[4] = {};
				for (uint32 i = 0; i < 4; i++)
					s[i] = convertSwizzle(header.swizzle[i]);
				tex->swizzle(s);
			}

			if (header.target == GL_TEXTURE_3D || header.target == GL_TEXTURE_2D_ARRAY)
				tex->initialize(header.resolution, header.mipmapLevels, header.internalFormat);
			else
				tex->initialize(Vec2i(header.resolution[0], header.resolution[1]), header.mipmapLevels, header.internalFormat);

			CAGE_ASSERT(header.mipmapLevels >= header.containedLevels);
			CAGE_ASSERT(header.containedLevels > 0);
			for (uint32 mip = 0; mip < header.containedLevels; mip++)
			{
				Vec3i resolution;
				uint32 size = 0;
				des >> resolution >> size;
				CAGE_ASSERT(mip > 0 || Vec2i(resolution) == Vec2i(header.resolution));
				textureLoadLevel(+tex, header, mip, des.read(size));
			}

			if (any(header.flags & TextureFlags::GenerateMipmaps))
			{
				CAGE_ASSERT(header.containedLevels == 1);
				CAGE_ASSERT(header.mipmapLevels > 1);
				tex->generateMipmaps();
			}
			else
			{
				CAGE_ASSERT(header.containedLevels == header.mipmapLevels);
			}

			context->assetHolder = std::move(tex).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeTexture(uint32 threadIndex)
	{
		AssetsScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Texture>();
		return s;
	}
}
