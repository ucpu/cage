#include <cage-core/macros.h>
#include <cage-core/image.h>
#include <cage-core/serialization.h>

#include <cage-engine/texture.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/opengl.h>

namespace cage
{
	namespace
	{
		class TextureImpl : public Texture
		{
		public:
			const uint32 target = 0;
			uint32 id = 0;
			const bool owning = true;

			Vec3i resolution;
			uint32 mipmapLevels = 0;
			uint32 internalFormat = 0;
			sint32 residentCounter = 0;

			TextureImpl(uint32 target) : target(target)
			{
				glCreateTextures(target, 1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind(0);
			}

			// createTextureForOpenXr
			TextureImpl(uint32 id, uint32 internalFormat, Vec2i resolution) : target(GL_TEXTURE_2D), id(id), owning(false), resolution(resolution, 1), mipmapLevels(1), internalFormat(internalFormat)
			{}

			~TextureImpl()
			{
				try
				{
					CAGE_ASSERT(residentCounter == 0);
				}
				catch (...)
				{}
				if (owning)
					glDeleteTextures(1, &id);
			}

			Vec3i mipRes(uint32 mip) const
			{
				CAGE_ASSERT(mip < mipmapLevels);
				return Vec3i(
					max(resolution[0] >> mip, 1),
					max(resolution[1] >> mip, 1),
					max(resolution[2] >> (target == GL_TEXTURE_2D_ARRAY ? 0 : mip), 1)
				);
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
		CAGE_ASSERT(impl->id);
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

	uint32 Texture::internalFormat() const
	{
		return ((TextureImpl *)this)->internalFormat;
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
		return impl->mipmapLevels;
	}

	void Texture::bind(uint32 bindingPoint) const
	{
		const TextureImpl *impl = (const TextureImpl *)this;
		glActiveTexture(GL_TEXTURE0 + bindingPoint);
		glBindTexture(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::bindImage(uint32 bindingPoint, bool read, bool write) const
	{
		CAGE_ASSERT(read || write);
		const TextureImpl *impl = (const TextureImpl *)this;
		const uint32 access = read && write ? GL_READ_WRITE : write ? GL_WRITE_ONLY : GL_READ_ONLY;
		glBindImageTexture(bindingPoint, impl->id, 0, GL_FALSE, 0, access, impl->internalFormat);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::filters(uint32 mig, uint32 mag, uint32 aniso)
	{
		TextureImpl *impl = (TextureImpl *)this;
		glTextureParameteri(impl->id, GL_TEXTURE_MIN_FILTER, mig);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTextureParameteri(impl->id, GL_TEXTURE_MAG_FILTER, mag);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTextureParameterf(impl->id, GL_TEXTURE_MAX_ANISOTROPY_EXT, float(max(aniso, 1u)));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::wraps(uint32 s, uint32 t)
	{
		wraps(s, t, GL_REPEAT);
	}

	void Texture::wraps(uint32 s, uint32 t, uint32 r)
	{
		TextureImpl *impl = (TextureImpl *)this;
		glTextureParameteri(impl->id, GL_TEXTURE_WRAP_S, s);
		glTextureParameteri(impl->id, GL_TEXTURE_WRAP_T, t);
		glTextureParameteri(impl->id, GL_TEXTURE_WRAP_R, r);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::swizzle(const uint32 values[4])
	{
		TextureImpl *impl = (TextureImpl *)this;
		static_assert(sizeof(uint32) == sizeof(GLint));
		glTextureParameteriv(impl->id, GL_TEXTURE_SWIZZLE_RGBA, (const GLint *)values);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::initialize(Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_2D || impl->target == GL_TEXTURE_RECTANGLE || impl->target == GL_TEXTURE_CUBE_MAP);
		CAGE_ASSERT(mipmapLevels > 0);
		CAGE_ASSERT(internalFormat != 0);
		CAGE_ASSERT(impl->internalFormat == 0); // cannot reinitialize
		impl->resolution = Vec3i(resolution, 1);
		impl->mipmapLevels = mipmapLevels;
		impl->internalFormat = internalFormat;
		glTextureStorage2D(impl->id, mipmapLevels, internalFormat, resolution[0], resolution[1]);
		glTextureParameteri(impl->id, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::initialize(Vec3i resolution, uint32 mipmapLevels, uint32 internalFormat)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		CAGE_ASSERT(mipmapLevels > 0);
		CAGE_ASSERT(internalFormat != 0);
		CAGE_ASSERT(impl->internalFormat == 0); // cannot reinitialize
		impl->resolution = resolution;
		impl->mipmapLevels = mipmapLevels;
		impl->internalFormat = internalFormat;
		glTextureStorage3D(impl->id, mipmapLevels, internalFormat, resolution[0], resolution[1], resolution[2]);
		glTextureParameteri(impl->id, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
		CAGE_CHECK_GL_ERROR_DEBUG();
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
				case 3: initialize(res, 1, GL_SRGB8); image2d(0, GL_RGB, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
				case 4: initialize(res, 1, GL_SRGB8_ALPHA8); image2d(0, GL_RGBA, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
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
				case 1: initialize(res, 1, GL_R8); image2d(0, GL_RED, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
				case 2: initialize(res, 1, GL_RG8); image2d(0, GL_RG, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
				case 3: initialize(res, 1, GL_RGB8); image2d(0, GL_RGB, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
				case 4: initialize(res, 1, GL_RGBA8); image2d(0, GL_RGBA, GL_UNSIGNED_BYTE, bufferCast<const char>(img->rawViewU8())); return;
				}
			} break;
			case ImageFormatEnum::U16:
			{
				switch (img->channels())
				{
				case 1: initialize(res, 1, GL_R16); image2d(0, GL_RED, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16())); return;
				case 2: initialize(res, 1, GL_RG16); image2d(0, GL_RG, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16())); return;
				case 3: initialize(res, 1, GL_RGB16); image2d(0, GL_RGB, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16())); return;
				case 4: initialize(res, 1, GL_RGBA16); image2d(0, GL_RGBA, GL_UNSIGNED_SHORT, bufferCast<const char>(img->rawViewU16())); return;
				}
			} break;
			case ImageFormatEnum::Float:
			{
				switch (img->channels())
				{
				case 1: initialize(res, 1, GL_R32F); image2d(0, GL_RED, GL_FLOAT, bufferCast<const char>(img->rawViewFloat())); return;
				case 2: initialize(res, 1, GL_RG32F); image2d(0, GL_RG, GL_FLOAT, bufferCast<const char>(img->rawViewFloat())); return;
				case 3: initialize(res, 1, GL_RGB32F); image2d(0, GL_RGB, GL_FLOAT, bufferCast<const char>(img->rawViewFloat())); return;
				case 4: initialize(res, 1, GL_RGBA32F); image2d(0, GL_RGBA, GL_FLOAT, bufferCast<const char>(img->rawViewFloat())); return;
				}
			} break;
			default:
				// pass
				break;
			}
		}
		CAGE_THROW_ERROR(Exception, "image has a combination of format, channels count and color configuration that cannot be imported into texture");
	}

	void Texture::image2d(uint32 mipmapLevel, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_2D || impl->target == GL_TEXTURE_RECTANGLE);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glTextureSubImage2D(impl->id, mipmapLevel, 0, 0, res[0], res[1], format, type, buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image2dCompressed(uint32 mipmapLevel, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_2D);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glCompressedTextureSubImage2D(impl->id, mipmapLevel, 0, 0, res[0], res[1], impl->internalFormat, buffer.size(), buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::imageCube(uint32 mipmapLevel, uint32 faceIndex, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_CUBE_MAP);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glTextureSubImage3D(impl->id, mipmapLevel, 0, 0, faceIndex, res[0], res[1], 1, format, type, buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::imageCubeCompressed(uint32 mipmapLevel, uint32 faceIndex, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_CUBE_MAP);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glCompressedTextureSubImage3D(impl->id, mipmapLevel, 0, 0, faceIndex, res[0], res[1], 1, impl->internalFormat, buffer.size(), buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image3d(uint32 mipmapLevel, uint32 format, uint32 type, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glTextureSubImage3D(impl->id, mipmapLevel, 0, 0, 0, res[0], res[1], res[2], format, type, buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::image3dCompressed(uint32 mipmapLevel, PointerRange<const char> buffer)
	{
		TextureImpl *impl = (TextureImpl *)this;
		CAGE_ASSERT(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		CAGE_ASSERT(impl->internalFormat != 0);
		CAGE_ASSERT(!buffer.empty());
		const Vec3i res = impl->mipRes(mipmapLevel);
		glCompressedTextureSubImage3D(impl->id, mipmapLevel, 0, 0, 0, res[0], res[1], res[2], impl->internalFormat, buffer.size(), buffer.data());
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Texture::generateMipmaps()
	{
		TextureImpl *impl = (TextureImpl *)this;
		glGenerateTextureMipmap(impl->id);
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
			double sample = (((double)((sint64)currentTime - (sint64)startTime) * (double)animationSpeed.value) / (double)texture->animationDuration + (double)animationOffset.value) * (double)frames;
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

	namespace privat
	{
		Holder<Texture> createTextureForOpenXr(uint32 id, uint32 internalFormat, Vec2i resolution)
		{
			return systemMemory().createImpl<Texture, TextureImpl>(id, internalFormat, resolution);
		}
	}
}
