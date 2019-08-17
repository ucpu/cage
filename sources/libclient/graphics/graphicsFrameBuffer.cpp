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
		class drawMark;
		class readMark;

		class frameBufferImpl : public frameBuffer
		{
		public:
			uint32 id;
			uint32 target;

			frameBufferImpl(uint32 target) : id(0), target(target)
			{
				glGenFramebuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~frameBufferImpl()
			{
				glDeleteFramebuffers(1, &id);
			}

			void setBinded()
			{
				switch (target)
				{
				case GL_DRAW_FRAMEBUFFER:
					setCurrentObject<drawMark>(id);
					break;
				case GL_READ_FRAMEBUFFER:
					setCurrentObject<readMark>(id);
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid frame buffer target");
				}
			}

			void checkBinded()
			{
				switch (target)
				{
				case GL_DRAW_FRAMEBUFFER:
					CAGE_ASSERT(getCurrentObject<drawMark>() == id);
					break;
				case GL_READ_FRAMEBUFFER:
					CAGE_ASSERT(getCurrentObject<readMark>() == id);
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid frame buffer target");
				}
			}
		};
	}

	void frameBuffer::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		frameBufferImpl *impl = (frameBufferImpl *)this;
		glObjectLabel(GL_FRAMEBUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 frameBuffer::getId() const
	{
		frameBufferImpl *impl = (frameBufferImpl *)this;
		return impl->id;
	}

	uint32 frameBuffer::getTarget() const
	{
		frameBufferImpl *impl = (frameBufferImpl *)this;
		return impl->target;
	}

	void frameBuffer::bind() const
	{
		frameBufferImpl *impl = (frameBufferImpl *)this;
		glBindFramebuffer(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		impl->setBinded();
	}

	void frameBuffer::depthTexture(renderTexture *tex)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		impl->checkBinded();
		glFramebufferTexture(impl->target, GL_DEPTH_ATTACHMENT, tex ? tex->getId() : 0, 0);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBuffer::colorTexture(uint32 index, renderTexture *tex, uint32 mipmapLevel)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		impl->checkBinded();
		glFramebufferTexture(impl->target, GL_COLOR_ATTACHMENT0 + index, tex ? tex->getId() : 0, mipmapLevel);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBuffer::activeAttachments(uint32 mask)
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		impl->checkBinded();
		uint32 count = 0;
		GLenum bufs[32];
		for (uint32 i = 0, bit = 1; i < 32; i++, bit *= 2)
		{
			if (mask & bit)
				bufs[count++] = GL_COLOR_ATTACHMENT0 + i;
		}
		switch (impl->target)
		{
		case GL_DRAW_FRAMEBUFFER:
			glDrawBuffers(count, bufs);
			break;
		case GL_READ_FRAMEBUFFER:
			CAGE_ASSERT(count == 1);
			glReadBuffer(*bufs);
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid frame buffer target");
		}
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void frameBuffer::clear()
	{
		depthTexture(nullptr);
		for (uint32 i = 0; i < 8; i++)
			colorTexture(i, nullptr);
		activeAttachments(0);
	}

	void frameBuffer::checkStatus()
	{
		frameBufferImpl *impl = (frameBufferImpl*)this;
		impl->checkBinded();
		GLenum result = glCheckFramebufferStatus(impl->target);
		CAGE_CHECK_GL_ERROR_DEBUG();
		switch (result)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			return;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			CAGE_THROW_ERROR(graphicsError, "incomplete frame buffer: attachment", numeric_cast<uint32>(result));
		//case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: // this is defined for ES only
		//	CAGE_THROW_ERROR(graphicsError, "incomplete frame buffer: dimensions", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			CAGE_THROW_ERROR(graphicsError, "incomplete frame buffer: missing attachment", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_UNSUPPORTED:
			CAGE_THROW_ERROR(graphicsError, "incomplete frame buffer: unsupported", numeric_cast<uint32>(result));
		default:
			CAGE_THROW_ERROR(graphicsError, "incomplete frame buffer: unknown error", numeric_cast<uint32>(result));
		}
	}

	holder<frameBuffer> newFrameBufferDraw()
	{
		return detail::systemArena().createImpl<frameBuffer, frameBufferImpl>(GL_DRAW_FRAMEBUFFER);
	}

	holder<frameBuffer> newFrameBufferRead()
	{
		return detail::systemArena().createImpl<frameBuffer, frameBufferImpl>(GL_READ_FRAMEBUFFER);
	}
}
