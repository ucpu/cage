#include <cage-core/core.h>
#include <cage-core/math.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class DrawMark;
		class ReadMark;

		class FrameBufferImpl : public FrameBuffer
		{
		public:
			uint32 id;
			uint32 target;

			FrameBufferImpl(uint32 target) : id(0), target(target)
			{
				glGenFramebuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~FrameBufferImpl()
			{
				glDeleteFramebuffers(1, &id);
			}

			void setBinded()
			{
				switch (target)
				{
				case GL_DRAW_FRAMEBUFFER:
					setCurrentObject<DrawMark>(id);
					break;
				case GL_READ_FRAMEBUFFER:
					setCurrentObject<ReadMark>(id);
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid frame buffer target");
				}
			}

			void checkBinded()
			{
				switch (target)
				{
				case GL_DRAW_FRAMEBUFFER:
					CAGE_ASSERT(getCurrentObject<DrawMark>() == id);
					break;
				case GL_READ_FRAMEBUFFER:
					CAGE_ASSERT(getCurrentObject<ReadMark>() == id);
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid frame buffer target");
				}
			}
		};
	}

	void FrameBuffer::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		glObjectLabel(GL_FRAMEBUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 FrameBuffer::getId() const
	{
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		return impl->id;
	}

	uint32 FrameBuffer::getTarget() const
	{
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		return impl->target;
	}

	void FrameBuffer::bind() const
	{
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		glBindFramebuffer(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		impl->setBinded();
	}

	void FrameBuffer::depthTexture(Texture *tex)
	{
		FrameBufferImpl *impl = (FrameBufferImpl*)this;
		impl->checkBinded();
		glFramebufferTexture(impl->target, GL_DEPTH_ATTACHMENT, tex ? tex->getId() : 0, 0);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::colorTexture(uint32 index, Texture *tex, uint32 mipmapLevel)
	{
		FrameBufferImpl *impl = (FrameBufferImpl*)this;
		impl->checkBinded();
		glFramebufferTexture(impl->target, GL_COLOR_ATTACHMENT0 + index, tex ? tex->getId() : 0, mipmapLevel);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::activeAttachments(uint32 mask)
	{
		FrameBufferImpl *impl = (FrameBufferImpl*)this;
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
			CAGE_THROW_CRITICAL(Exception, "invalid frame buffer target");
		}
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::clear()
	{
		depthTexture(nullptr);
		for (uint32 i = 0; i < 8; i++)
			colorTexture(i, nullptr);
		activeAttachments(0);
	}

	void FrameBuffer::checkStatus()
	{
		FrameBufferImpl *impl = (FrameBufferImpl*)this;
		impl->checkBinded();
		GLenum result = glCheckFramebufferStatus(impl->target);
		CAGE_CHECK_GL_ERROR_DEBUG();
		switch (result)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			return;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: attachment", numeric_cast<uint32>(result));
		//case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: // this is defined for ES only
		//	CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: dimensions", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: missing attachment", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_UNSUPPORTED:
			CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: unsupported", numeric_cast<uint32>(result));
		default:
			CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: unknown error", numeric_cast<uint32>(result));
		}
	}

	Holder<FrameBuffer> newFrameBufferDraw()
	{
		return detail::systemArena().createImpl<FrameBuffer, FrameBufferImpl>(GL_DRAW_FRAMEBUFFER);
	}

	Holder<FrameBuffer> newFrameBufferRead()
	{
		return detail::systemArena().createImpl<FrameBuffer, FrameBufferImpl>(GL_READ_FRAMEBUFFER);
	}
}
