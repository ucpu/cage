#include <cage-core/macros.h>
#include <cage-core/image.h>
#include <cage-core/serialization.h>

#include <cage-engine/opengl.h>
#include <cage-engine/texture.h>
#include "private.h"

namespace cage
{
	namespace privat
	{
		namespace
		{
			template<uint32 N>
			class NumberedTexture {};

			sint32 activeTexture()
			{
				sint32 i = -1;
				glGetIntegerv(GL_ACTIVE_TEXTURE, &i);
				CAGE_CHECK_GL_ERROR_DEBUG();
				i -= GL_TEXTURE0;
				CAGE_ASSERT(i >= 0 && i < 32);
				return i;
			}

			void setSpecificTexture(uint32 index, uint32 id)
			{
				switch (index)
				{
#define GCHL_GENERATE(I) case I: return setCurrentObject<NumberedTexture<I>>(id);
					GCHL_GENERATE(0);
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(31, GCHL_GENERATE));
#undef GCHL_GENERATE
				}
			}

			void setCurrentTexture(uint32 id)
			{
				setSpecificTexture(activeTexture(), id);
			}

			uint32 getCurrentTexture()
			{
				switch (activeTexture())
				{
#define GCHL_GENERATE(I) case I: return getCurrentObject<NumberedTexture<I>>();
					GCHL_GENERATE(0);
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(31, GCHL_GENERATE));
#undef GCHL_GENERATE
				default: CAGE_THROW_CRITICAL(Exception, "active texture index out of range");
				}
			}
		}
	}

	namespace
	{
		class TextureImpl : public Texture
		{
		public:
			Vec3i resolution;
			uint32 levels = 1000;
			const uint32 target = 0;
			uint32 id = 0;

			TextureImpl(uint32 target) : target(target)
			{
				glGenTextures(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~TextureImpl()
			{
				glDeleteTextures(1, &id);
			}
		};

		uint32 textureFormat(uint32 internalFormat)
		{
			switch (internalFormat)
			{
			case GL_R8:
			case GL_R16:
			case GL_R16F:
			case GL_R16I:
			case GL_R16UI:
			case GL_R32F:
			case GL_R32I:
			case GL_R32UI:
			case GL_R8_SNORM:
			case GL_R16_SNORM:
				return GL_RED;
			case GL_RG8:
			case GL_RG16:
			case GL_RG16F:
			case GL_RG16I:
			case GL_RG16UI:
			case GL_RG32F:
			case GL_RG32I:
			case GL_RG32UI:
			case GL_RG8_SNORM:
			case GL_RG16_SNORM:
				return GL_RG;
			case GL_RGB8:
			case GL_RGB16:
			case GL_RGB16F:
			case GL_RGB16I:
			case GL_RGB16UI:
			case GL_RGB32F:
			case GL_RGB32I:
			case GL_RGB32UI:
			case GL_RGB8_SNORM:
			case GL_RGB16_SNORM:
			case GL_RGB10:
			case GL_RGB4:
			case GL_RGB5:
			case GL_RGB565:
			case GL_RGB12:
			case GL_R11F_G11F_B10F:
			case GL_RGB9_E5:
			case GL_R3_G3_B2:
			case GL_SRGB8:
				return GL_RGB;
			case GL_RGBA8:
			case GL_RGBA16:
			case GL_RGBA16F:
			case GL_RGBA16I:
			case GL_RGBA16UI:
			case GL_RGBA32F:
			case GL_RGBA32I:
			case GL_RGBA32UI:
			case GL_RGBA8_SNORM:
			case GL_RGBA16_SNORM:
			case GL_RGBA2:
			case GL_RGBA4:
			case GL_RGBA12:
			case GL_RGB5_A1:
			case GL_RGB10_A2:
			case GL_RGB10_A2UI:
			case GL_SRGB8_ALPHA8:
				return GL_RGBA;
			case GL_DEPTH_COMPONENT16:
			case GL_DEPTH_COMPONENT24:
			case GL_DEPTH_COMPONENT32:
			case GL_DEPTH_COMPONENT32F:
				return GL_DEPTH_COMPONENT;
			case GL_DEPTH24_STENCIL8:
			case GL_DEPTH32F_STENCIL8:
				return GL_DEPTH_STENCIL;
			default: CAGE_THROW_CRITICAL(Exception, "unknown texture internal format");
			}
		}

		uint32 textureType(uint32 internalFormat)
		{
			return GL_FLOAT;
		}
	}

	void Texture::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		TextureImpl *impl = (TextureImpl*)this;
		glObjectLabel(GL_TEXTURE, impl->id, name.length(), name.c_str());
	}

	uint32 Texture::id() const
	{
		return ((TextureImpl *)this)->id;
	}

	uint32 Texture::target() const
	{
		return ((TextureImpl *)this)->target;
	}

	Vec2i Texture::resolution() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		CAGE_ASSERT(impl->resolution[2] <= 1);
		return Vec2i(impl->resolution);
	}

	Vec3i Texture::resolution3() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return impl->resolution;
	}

	uint32 Texture::mipmapLevels() const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		return impl->levels;
	}

	void Texture::bind() const
	{
		CAGE_ASSERT(graphicsPrivat::getCurrentContext());
		const TextureImpl *impl = (const TextureImpl *)this;
		glBindTexture(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
#ifdef CAGE_ASSERT_ENABLED
		privat::setCurrentTexture(impl->id);
#endif // CAGE_ASSERT_ENABLED
	}

	void Texture::importImage(const Image *img)
	{
		const uint32 w = img->width();
		const uint32 h = img->height();
		const Vec2i res = Vec2i(w, h);
		if (img->colorConfig.gammaSpace == GammaSpaceEnum::Gamma)
		{
			switch (img->format())
			{
			case ImageFormatEnum::U8:
			{
				switch (img->channels())
				{
				case 3: return image2d(res, GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				case 4: return image2d(res, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				}
			} break;
			default:
				// pass
				break;
			}
		}
		else
		{
			switch (img->format())
			{
			case ImageFormatEnum::U8:
			{
				switch (img->channels())
				{
				case 1: return image2d(res, GL_R8, GL_RED, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				case 2: return image2d(res, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				case 3: return image2d(res, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				case 4: return image2d(res, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8()));
				}
			} break;
			case ImageFormatEnum::U16:
			{
				switch (img->channels())
				{
				case 1: return image2d(res, GL_R16, GL_RED, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16()));
				case 2: return image2d(res, GL_RG16, GL_RG, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16()));
				case 3: return image2d(res, GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16()));
				case 4: return image2d(res, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16()));
				}
			} break;
			case ImageFormatEnum::Float:
			{
				switch (img->channels())
				{
				case 1: return image2d(res, GL_R32F, GL_RED, GL_FLOAT, bufferCast<const char>(img->rawViewFloat()));
				case 2: return image2d(res, GL_RG32F, GL_RG, GL_FLOAT, bufferCast<const char>(img->rawViewFloat()));
				case 3: return image2d(res, GL_RGB32F, GL_RGB, GL_FLOAT, bufferCast<const char>(img->rawViewFloat()));
				case 4: return image2d(res, GL_RGBA32F, GL_RGBA, GL_FLOAT, bufferCast<const char>(img->rawViewFloat()));
				}
			} break;
			default:
				// pass
				break;
			}
		}
		CAGE_THROW_ERROR(Exception, "image has a combination of format, channels count and color configuration that cannot be imported into texture");
	}

	void Texture::image2d(Vec2i resolution, uint32 internalFormat)
	{
		image2d(resolution, internalFormat, textureFormat(internalFormat), textureType(internalFormat), {});
	}

	void Texture::image2d(Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		image2d(0, resolution, internalFormat, format, type, buffer);
	}

	void Texture::image2d(uint32 mipmapLevel, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_2D || impl->target == GL_TEXTURE_RECTANGLE);
		glTexImage2D(impl->target, mipmapLevel, internalFormat, resolution[0], resolution[1], 0, format, type, buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = Vec3i(resolution, 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image2dCompressed(Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		image2dCompressed(0, resolution, internalFormat, buffer);
	}

	void Texture::image2dCompressed(uint32 mipmapLevel, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_2D);
		glCompressedTexImage2D(impl->target, mipmapLevel, internalFormat, resolution[0], resolution[1], 0, buffer.size(), buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = Vec3i(resolution, 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::imageCube(Vec2i resolution, uint32 internalFormat)
	{
		for (uint32 f = 0; f < 6; f++)
			imageCube(f, resolution, internalFormat, textureFormat(internalFormat), textureType(internalFormat), {});
	}

	void Texture::imageCube(uint32 faceIndex, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		imageCube(0, faceIndex, resolution, internalFormat, format, type, buffer);
	}

	void Texture::imageCube(uint32 mipmapLevel, uint32 faceIndex, Vec2i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_CUBE_MAP);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, mipmapLevel, internalFormat, resolution[0], resolution[1], 0, format, type, buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = Vec3i(resolution, 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::imageCubeCompressed(uint32 faceIndex, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		imageCubeCompressed(0, faceIndex, resolution, internalFormat, buffer);
	}

	void Texture::imageCubeCompressed(uint32 mipmapLevel, uint32 faceIndex, Vec2i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_CUBE_MAP);
		glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, mipmapLevel, internalFormat, resolution[0], resolution[1], 0, buffer.size(), buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = Vec3i(resolution, 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image3d(Vec3i resolution, uint32 internalFormat)
	{
		image3d(resolution, internalFormat, textureFormat(internalFormat), textureType(internalFormat), {});
	}

	void Texture::image3d(Vec3i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		image3d(0, resolution, internalFormat, format, type, buffer);
	}

	void Texture::image3d(uint32 mipmapLevel, Vec3i resolution, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		glTexImage3D(impl->target, mipmapLevel, internalFormat, resolution[0], resolution[1], resolution[2], 0, format, type, buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = resolution;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image3dCompressed(Vec3i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		image3dCompressed(0, resolution, internalFormat, buffer);
	}

	void Texture::image3dCompressed(uint32 mipmapLevel, Vec3i resolution, uint32 internalFormat, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		glCompressedTexImage3D(impl->target, mipmapLevel, internalFormat, resolution[0], resolution[1], resolution[2], 0, buffer.size(), buffer.data());
		if (mipmapLevel == 0)
			impl->resolution = resolution;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::filters(uint32 mig, uint32 mag, uint32 aniso)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		glTexParameteri(impl->target, GL_TEXTURE_MIN_FILTER, mig);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTexParameteri(impl->target, GL_TEXTURE_MAG_FILTER, mag);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTexParameterf(impl->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, float(max(aniso, 1u)));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::wraps(uint32 s, uint32 t)
	{
		wraps(s, t, GL_REPEAT);
	}

	void Texture::wraps(uint32 s, uint32 t, uint32 r)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_S, s);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_T, t);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_R, r);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::swizzle(const uint32 values[4])
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		static_assert(sizeof(uint32) == sizeof(GLint));
		glTexParameteriv(impl->target, GL_TEXTURE_SWIZZLE_RGBA, (const GLint *)values);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::mipmapLevels(uint32 levels)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		glTexParameteri(impl->target, GL_TEXTURE_MAX_LEVEL, levels);
		impl->levels = levels;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::generateMipmaps()
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(privat::getCurrentTexture() == impl->id);
		glGenerateMipmap(impl->target);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	Holder<Texture> newTexture()
	{
		return newTexture(GL_TEXTURE_2D);
	}

	Holder<Texture> newTexture(uint32 target)
	{
		CAGE_ASSERT(target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_RECTANGLE || target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP);
		return systemMemory().createImpl<Texture, TextureImpl>(target);
	}

	namespace detail
	{
		Vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset)
		{
			if (!texture)
				return Vec4();
			const uint32 frames = numeric_cast<uint32>(texture->resolution3()[2]);
			if (frames <= 1)
				return Vec4();
			double sample = ((double)((sint64)currentTime - (sint64)startTime) * (double)animationSpeed.value + (double)animationOffset.value) * (double)frames / (double)texture->animationDuration;
			if (!texture->animationLoop)
				sample = clamp(sample, 0.0, (double)(frames - 1));
			else
				sample += (-(sint64)sample / frames + 1) * frames;
			const uint32 i = (uint32)sample % frames;
			const Real f = sample - (sint32)sample;
			CAGE_ASSERT(i < frames);
			CAGE_ASSERT(f >= 0 && f <= 1);
			return Vec4(i, (i + 1) % frames, f, 0);
		}
	}
}
