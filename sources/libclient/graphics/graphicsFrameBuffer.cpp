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
	namespace
	{
		class frameBufferImpl : public frameBufferClass
		{
		public:
			uint32 id;

			frameBufferImpl()
			{
				glGenFramebuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~frameBufferImpl()
			{
				glDeleteFramebuffers(1, &id);
			}
		};
	}

	const uint32 frameBufferClass::getId() const
	{
		frameBufferImpl *impl = (frameBufferImpl *)this;
		return impl->id;
	}

	void frameBufferClass::bind() const
	{
		bind(true, true);
	}

	void frameBufferClass::bind(bool draw, bool read) const
	{
		CAGE_ASSERT_RUNTIME(draw || read, draw, read, "at least one must be specified");
		frameBufferImpl *impl = (frameBufferImpl *)this;
		uint32 target = 0;
		if (draw) target |= GL_DRAW_FRAMEBUFFER;
		if (read) target |= GL_READ_FRAMEBUFFER;
		glBindFramebuffer(target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<frameBufferClass>(impl->id);
	}

	void frameBufferClass::depthTexture(textureClass *tex)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(getCurrentObject<frameBufferClass>() == impl->id);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex ? tex->getId() : 0, 0);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBufferClass::colorTexture(uint32 index, textureClass *tex)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(getCurrentObject<frameBufferClass>() == impl->id);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, tex ? tex->getId() : 0, 0);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBufferClass::drawAttachments(uint32 mask)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(getCurrentObject<frameBufferClass>() == impl->id);
		uint32 count = 0;
		GLenum bufs[32];
		for (uint32 i = 0, bit = 1; i < 32; i++, bit *= 2)
		{
			if (mask & bit)
				bufs[count++] = GL_COLOR_ATTACHMENT0 + i;
		}
		glDrawBuffers(count, bufs);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBufferClass::checkStatus()
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(getCurrentObject<frameBufferClass>() == impl->id);
		GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (result != GL_FRAMEBUFFER_COMPLETE)
			CAGE_THROW_ERROR(graphicsException, "frameBufferClass::checkStatus: frame buffer check failed", numeric_cast<uint32>(result));
	}

	holder<frameBufferClass> newFrameBuffer(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(getCurrentContext() == context);
		return detail::systemArena().createImpl <frameBufferClass, frameBufferImpl>();
	}
}