#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/config.h>
#include <cage-core/typeIndex.h>
#include <cage-core/imageBlocks.h>

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

		uint32 findInternalFormatForKtx(const TextureHeader &data)
		{
			if (any(data.flags & TextureFlags::Srgb))
			{
				switch (data.channels)
				{
				case 3: return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
				case 4: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
				}
			}
			else
			{
				switch (data.channels)
				{
				case 1: return GL_COMPRESSED_RED_RGTC1;
				case 2: return GL_COMPRESSED_RG_RGTC2;
				case 3: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				case 4: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				}
			}
			CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
		}

		ImageKtxTranscodeFormatEnum findBlockFormatForKtx(const TextureHeader &data)
		{
			switch (data.channels)
			{
			case 1: return ImageKtxTranscodeFormatEnum::Bc4;
			case 2: return ImageKtxTranscodeFormatEnum::Bc5;
			case 3: return ImageKtxTranscodeFormatEnum::Bc1;
			case 4: return ImageKtxTranscodeFormatEnum::Bc3;
			}
			CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
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
				ImageKtxTranscodeConfig config;
				config.format = findBlockFormatForKtx(data);
				const uint32 internalFormat = findInternalFormatForKtx(data);
				const auto res = imageKtxTranscode(values, config);
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
					tex->image3dCompressed(data.resolution, internalFormat, buff);
				}
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					CAGE_ASSERT(res.size() == 6);
					for (uint32 f = 0; f < 6; f++)
						tex->imageCubeCompressed(f, Vec2i(data.resolution), internalFormat, res[f].data);
				}
				else
				{
					CAGE_ASSERT(res.size() == 1);
					tex->image2dCompressed(Vec2i(data.resolution), internalFormat, res[0].data);
				}
			}
			else if (any(data.flags & TextureFlags::Compressed))
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3dCompressed(data.resolution, data.internalFormat, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->imageCubeCompressed(f, Vec2i(data.resolution), data.internalFormat, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
				else
					tex->image2dCompressed(Vec2i(data.resolution), data.internalFormat, values);
			}
			else
			{
				if (data.target == GL_TEXTURE_3D || data.target == GL_TEXTURE_2D_ARRAY)
					tex->image3d(data.resolution, data.internalFormat, data.copyFormat, data.copyType, values);
				else if (data.target == GL_TEXTURE_CUBE_MAP)
				{
					const uint32 stride = values.size() / 6;
					for (uint32 f = 0; f < 6; f++)
						tex->imageCube(f, Vec2i(data.resolution), data.internalFormat, data.copyFormat, data.copyType, { values.data() + f * stride, values.data() + (f + 1) * stride });
				}
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
		s.typeHash = detail::typeHash<Texture>();
		return s;
	}
}
