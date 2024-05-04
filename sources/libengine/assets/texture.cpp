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

		void textureLoadLevel(Texture *tex, const TextureHeader &data, uint32 mipmapLevel, PointerRange<const char> values)
		{
			if (any(data.flags & TextureFlags::Compressed))
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3dCompressed(mipmapLevel, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->imageCubeCompressed(mipmapLevel, f, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
				else
					tex->image2dCompressed(mipmapLevel, values);
			}
			else
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3d(mipmapLevel, data.copyFormat, data.copyType, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->imageCube(mipmapLevel, f, data.copyFormat, data.copyType, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
				else
					tex->image2d(mipmapLevel, data.copyFormat, data.copyType, values);
			}
		}

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader data;
			des >> data;

			Holder<Texture> tex = newTexture(data.target);
			tex->setDebugName(context->textId);

			tex->filters(data.filterMin, data.filterMag, data.filterAniso);
			tex->wraps(data.wrapX, data.wrapY, data.wrapZ);
			{
				uint32 s[4] = {};
				for (uint32 i = 0; i < 4; i++)
					s[i] = convertSwizzle(data.swizzle[i]);
				tex->swizzle(s);
			}

			if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
				tex->initialize(data.resolution, data.mipmapLevels, data.internalFormat);
			else
				tex->initialize(Vec2i(data.resolution[0], data.resolution[1]), data.mipmapLevels, data.internalFormat);

			CAGE_ASSERT(data.mipmapLevels >= data.containedLevels);
			CAGE_ASSERT(data.containedLevels > 0);
			for (uint32 mip = 0; mip < data.containedLevels; mip++)
			{
				Vec3i resolution;
				uint32 size = 0;
				des >> resolution >> size;
				CAGE_ASSERT(mip > 0 || Vec2i(resolution) == Vec2i(data.resolution));
				PointerRange<const char> values = des.read(size);
				textureLoadLevel(+tex, data, mip, values);
			}

			if (any(data.flags & TextureFlags::GenerateMipmaps))
			{
				CAGE_ASSERT(data.containedLevels == 1);
				CAGE_ASSERT(data.mipmapLevels > 1);
				tex->generateMipmaps();
			}
			else
			{
				CAGE_ASSERT(data.containedLevels == data.mipmapLevels);
			}

			tex->animationDuration = data.animationDuration;
			tex->animationLoop = any(data.flags & TextureFlags::AnimationLoop);

			context->assetHolder = std::move(tex).cast<void>();
		}
	}

	AssetScheme genAssetSchemeTexture(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeHash = detail::typeHash<Texture>();
		return s;
	}
}
