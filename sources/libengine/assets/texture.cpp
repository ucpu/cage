#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/config.h>
#include <cage-core/typeIndex.h>
#include <cage-core/imageKtx.h>

#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		uint32 convertSwizzle(TextureSwizzleEnum s)
		{
			switch (s)
			{
			case TextureSwizzleEnum::Zero: return GL_ZERO;
			case TextureSwizzleEnum::One: return GL_ONE;
			case TextureSwizzleEnum::R: return GL_RED;
			case TextureSwizzleEnum::G: return GL_GREEN;
			case TextureSwizzleEnum::B: return GL_BLUE;
			case TextureSwizzleEnum::A: return GL_ALPHA;
			default: CAGE_THROW_ERROR(Exception, "invalid TextureSwizzleEnum value");
			}
		}

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			TextureHeader data;
			des >> data;

			Holder<Texture> tex = newTexture(data.target);
			tex->setDebugName(context->textName);

			PointerRange<const char> values = des.read(des.available());

			if (any(data.flags & TextureFlags::Ktx))
			{
				const auto res = imageKtxDecompressBlocks(values);
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
				{
					const uint32 c = res.size();
					const uint32 s = res[0].data.size();
					MemoryBuffer buff;
					buff.resize(c * s);
					for (uint32 i = 0; i < c; i++)
					{
						CAGE_ASSERT(res[i].data.size() == s);
						detail::memcpy(buff.data() + i * s, res[i].data.data(), s);
					}
					tex->image3dCompressed(data.resolution, data.internalFormat, buff);
				}
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					CAGE_ASSERT(res.size() == 6);
					const uint32 s = res[0].data.size();
					MemoryBuffer buff;
					buff.resize(s * 6);
					for (uint32 f = 0; f < 6; f++)
					{
						CAGE_ASSERT(res[f].data.size() == s);
						detail::memcpy(buff.data() + s * f, res[f].data.data(), s);
					}
					tex->imageCubeCompressed(Vec2i(data.resolution), data.internalFormat, buff, data.stride);
				}
				else
				{
					CAGE_ASSERT(res.size() == 1);
					tex->image2dCompressed(Vec2i(data.resolution), data.internalFormat, res[0].data);
				}
			}
			else if (any(data.flags & TextureFlags::Astc))
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3dCompressed(data.resolution, data.internalFormat, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
					tex->imageCubeCompressed(Vec2i(data.resolution), data.internalFormat, values, data.stride);
				else
					tex->image2dCompressed(Vec2i(data.resolution), data.internalFormat, values);
			}
			else
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3d(data.resolution, data.internalFormat, data.copyFormat, data.copyType, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
					tex->imageCube(Vec2i(data.resolution), data.internalFormat, data.copyFormat, data.copyType, values, data.stride);
				else
					tex->image2d(Vec2i(data.resolution), data.internalFormat, data.copyFormat, data.copyType, values);
			}

			tex->filters(data.filterMin, data.filterMag, data.filterAniso);
			tex->wraps(data.wrapX, data.wrapY, data.wrapZ);
			{
				uint32 s[4] = {};
				for (uint32 i = 0; i < 4; i++)
					s[i] = convertSwizzle(data.swizzle[i]);
				tex->swizzle(s);
			}
			if (any(data.flags & TextureFlags::GenerateMipmaps))
				tex->generateMipmaps();

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
		s.typeIndex = detail::typeIndex<Texture>();
		return s;
	}
}
