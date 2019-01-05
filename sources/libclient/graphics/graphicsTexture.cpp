#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include "private.h"

namespace cage
{
	namespace privat
	{
		namespace
		{
			template<uint32 N> class numberedTextureClass {};

			const sint32 activeTexture()
			{
				sint32 i = -1;
				glGetIntegerv(GL_ACTIVE_TEXTURE, &i);
				CAGE_CHECK_GL_ERROR_DEBUG();
				i -= GL_TEXTURE0;
				CAGE_ASSERT_RUNTIME(i >= 0 && i < 32, "GL_ACTIVE_TEXTURE out of range", i);
				return i;
			}

			void setSpecificTexture(uint32 index, uint32 id)
			{
				switch (index)
				{
#define GCHL_GENERATE(I) case I: return setCurrentObject<numberedTextureClass<I> >(id);
					GCHL_GENERATE(0);
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(31, GCHL_GENERATE));
#undef GCHL_GENERATE
				}
			}

			void setCurrentTexture(uint32 id)
			{
				setSpecificTexture(activeTexture(), id);
			}

			const uint32 getCurrentTexture()
			{
				switch (activeTexture())
				{
#define GCHL_GENERATE(I) case I: return getCurrentObject<numberedTextureClass<I> >();
					GCHL_GENERATE(0);
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(31, GCHL_GENERATE));
#undef GCHL_GENERATE
				default: CAGE_THROW_CRITICAL(exception, "active texture index out of range");
				}
			}
		}
	}

	namespace
	{
		class textureImpl : public textureClass
		{
		public:
			const uint32 target;
			uint32 id;
			uint32 width, height, depth;

			textureImpl(uint32 target) : id(0), target(target), width(0), height(0), depth(0)
			{
				animationDuration = 0;
				animationLoop = false;
				glGenTextures(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~textureImpl()
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
				return GL_RGBA;
			case GL_DEPTH_COMPONENT16:
			case GL_DEPTH_COMPONENT24:
			case GL_DEPTH_COMPONENT32:
			case GL_DEPTH_COMPONENT32F:
				return GL_DEPTH_COMPONENT;
			case GL_DEPTH24_STENCIL8:
			case GL_DEPTH32F_STENCIL8:
				return GL_DEPTH_STENCIL;
			default: CAGE_THROW_CRITICAL(exception, "unknown texture internal format");
			}
		}

		uint32 textureType(uint32 internalFormat)
		{
			return GL_FLOAT;
		}
	}

	uint32 textureClass::getId() const
	{
		return ((textureImpl*)this)->id;
	}

	uint32 textureClass::getTarget() const
	{
		return ((textureImpl*)this)->target;
	}

	void textureClass::getResolution(uint32 &width, uint32 &height) const
	{
		textureImpl *impl = (textureImpl*)this;
		width = impl->width;
		height = impl->height;
	}

	void textureClass::getResolution(uint32 &width, uint32 &height, uint32 &depth) const
	{
		textureImpl *impl = (textureImpl*)this;
		width = impl->width;
		height = impl->height;
		depth = impl->depth;
	}

	void textureClass::bind() const
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext());
		textureImpl *impl = (textureImpl*)this;
		glBindTexture(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		privat::setCurrentTexture(impl->id);
	}

	void textureClass::image2d(uint32 w, uint32 h, uint32 internalFormat)
	{
		image2d(w, h, internalFormat, textureFormat(internalFormat), textureType(internalFormat), nullptr);
	}

	void textureClass::image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data)
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT_RUNTIME(impl->target == GL_TEXTURE_2D || impl->target == GL_TEXTURE_RECTANGLE);
		glTexImage2D(impl->target, 0, internalFormat, w, h, 0, format, type, data);
		impl->width = w;
		impl->height = h;
		impl->depth = 1;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::imageCube(uint32 w, uint32 h, uint32 internalFormat)
	{
		imageCube(w, h, internalFormat, textureFormat(internalFormat), textureType(internalFormat), nullptr, 0);
	}

	void textureClass::imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data, uintPtr stride)
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT_RUNTIME(impl->target == GL_TEXTURE_CUBE_MAP);
		for (uint32 i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, w, h, 0, format, type, (char*)data + i * stride);
		impl->width = w;
		impl->height = h;
		impl->depth = 1;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat)
	{
		image3d(w, h, d, internalFormat, textureFormat(internalFormat), textureType(internalFormat), nullptr);
	}

	void textureClass::image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, const void *data)
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		CAGE_ASSERT_RUNTIME(impl->target == GL_TEXTURE_3D || impl->target == GL_TEXTURE_2D_ARRAY);
		glTexImage3D(impl->target, 0, internalFormat, w, h, d, 0, format, type, data);
		impl->width = w;
		impl->height = h;
		impl->depth = d;
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::filters(uint32 mig, uint32 mag, uint32 aniso)
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		glTexParameteri(impl->target, GL_TEXTURE_MIN_FILTER, mig);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTexParameteri(impl->target, GL_TEXTURE_MAG_FILTER, mag);
		CAGE_CHECK_GL_ERROR_DEBUG();
		glTexParameterf(impl->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, float(max(aniso, 1u)));
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::wraps(uint32 s, uint32 t)
	{
		wraps(s, t, GL_REPEAT);
	}

	void textureClass::wraps(uint32 s, uint32 t, uint32 r)
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_S, s);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_T, t);
		glTexParameteri(impl->target, GL_TEXTURE_WRAP_R, r);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::generateMipmaps()
	{
		textureImpl *impl = (textureImpl*)this;
		CAGE_ASSERT_RUNTIME(privat::getCurrentTexture() == impl->id);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glGenerateMipmap(impl->target);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void textureClass::multiBind(uint32 count, const uint32 tius[], const textureClass *const texs[])
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext());
		GLint active = 0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
		for (uint32 i = 0; i < count; i++)
		{
			glActiveTexture(GL_TEXTURE0 + tius[i]);
			texs[i] ? texs[i]->bind() : void();
		}
		glActiveTexture(active);
		CAGE_CHECK_GL_ERROR_DEBUG();
		/*
		CAGE_ASSERT_RUNTIME(count <= 32);
		uint32 textures[32];
		detail::memset(textures, 0, sizeof(textures));
		for (uint32 i = 0; i < count; i++)
		{
			if (texs[i])
			{
				CAGE_ASSERT_RUNTIME(textures[i] == 0, "cannot bind multiple textures to same texture unit", textures[i], i);
				textures[tius[i]] = ((textureImpl*)texs[i])->id;
			}
		}
		glBindTextures(0, 32, textures);
		CAGE_CHECK_GL_ERROR_DEBUG();
		for (uint32 i = 0; i < 32; i++)
			privat::setSpecificTexture(i, textures[i]);
		*/
	}

	holder<textureClass> newTexture(windowClass *context)
	{
		return newTexture(context, GL_TEXTURE_2D);
	}

	holder<textureClass> newTexture(windowClass *context, uint32 target)
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext() == context);
		CAGE_ASSERT_RUNTIME(target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_RECTANGLE || target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP);
		return detail::systemArena().createImpl <textureClass, textureImpl>(target);
	}

	namespace detail
	{
		vec4 evalSamplesForTextureAnimation(const textureClass *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset)
		{
			if (!texture)
				return vec4();
			uint32 dummy, frames;
			texture->getResolution(dummy, dummy, frames);
			if (frames <= 1)
				return vec4();
			double sample = ((double)((sint64)emitTime - (sint64)animationStart) * (double)animationSpeed.value + (double)animationOffset.value) * (double)frames / (double)texture->animationDuration;
			if (!texture->animationLoop)
				sample = sample < 0 ? 0 : sample > frames - 1 ? frames - 1 : sample;
			else
			{
				if (sample < 0)
				{
					uint32 n = numeric_cast<uint32>(-sample / frames);
					sample += (n + 1) * frames;
				}
				else if (sample >= frames)
				{
					uint32 n = numeric_cast<uint32>(sample / frames);
					sample -= n * frames;
				}
			}
			CAGE_ASSERT_RUNTIME(sample >= 0 && sample < frames, sample, frames);
			real s = (float)sample;
			real f = floor(s);
			if (s < frames - 1)
				return vec4(f, f + 1, s - f, 0);
			return vec4(frames - 1, 0, s - f, 0);
		}
	}
}
