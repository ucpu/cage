#include <cage-engine/frameBuffer.h>
#include <cage-engine/texture.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/opengl.h>

namespace cage
{
	namespace
	{
		class FrameBufferImpl : public FrameBuffer
		{
		public:
			uint32 id = 0;
			uint32 target = 0;

			FrameBufferImpl(uint32 target) : target(target)
			{
				glGenFramebuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~FrameBufferImpl()
			{
				glDeleteFramebuffers(1, &id);
			}
		};
	}

	void FrameBuffer::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		CAGE_ASSERT(impl->id);
		glObjectLabel(GL_FRAMEBUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 FrameBuffer::id() const
	{
		const FrameBufferImpl *impl = (const FrameBufferImpl *)this;
		return impl->id;
	}

	uint32 FrameBuffer::target() const
	{
		const FrameBufferImpl *impl = (const FrameBufferImpl *)this;
		return impl->target;
	}

	void FrameBuffer::bind() const
	{
		const FrameBufferImpl *impl = (const FrameBufferImpl *)this;
		glBindFramebuffer(impl->target, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::depthTexture(Texture *tex)
	{
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		glNamedFramebufferTexture(impl->id, GL_DEPTH_ATTACHMENT, tex ? tex->id() : 0, 0);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::colorTexture(uint32 index, Texture *tex, uint32 mipmapLevel)
	{
		CAGE_ASSERT(!tex || mipmapLevel < tex->mipmapLevels());
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		glNamedFramebufferTexture(impl->id, GL_COLOR_ATTACHMENT0 + index, tex ? tex->id() : 0, mipmapLevel);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void FrameBuffer::activeAttachments(uint32 mask)
	{
		FrameBufferImpl *impl = (FrameBufferImpl *)this;
		uint32 count = 0;
		GLenum bufs[32];
		for (uint32 i = 0; i < 32; i++)
		{
			if (mask & (uint32(1) << i))
				bufs[count++] = GL_COLOR_ATTACHMENT0 + i;
		}
		switch (impl->target)
		{
		case GL_DRAW_FRAMEBUFFER:
			glNamedFramebufferDrawBuffers(impl->id, count, bufs);
			break;
		case GL_READ_FRAMEBUFFER:
			CAGE_ASSERT(count == 1);
			glNamedFramebufferReadBuffer(impl->id, *bufs);
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

	void FrameBuffer::checkStatus() const
	{
		const FrameBufferImpl *impl = (const FrameBufferImpl *)this;
		GLenum result = glCheckNamedFramebufferStatus(impl->id, impl->target);
		CAGE_CHECK_GL_ERROR_DEBUG();
		switch (result)
		{
		case GL_FRAMEBUFFER_COMPLETE: return;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: attachment", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: missing attachment", numeric_cast<uint32>(result));
		case GL_FRAMEBUFFER_UNSUPPORTED: CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: unsupported", numeric_cast<uint32>(result));
		default: CAGE_THROW_ERROR(GraphicsError, "incomplete frame buffer: unknown error", numeric_cast<uint32>(result));
		}
	}

	Holder<FrameBuffer> newFrameBufferDraw()
	{
		return systemMemory().createImpl<FrameBuffer, FrameBufferImpl>(GL_DRAW_FRAMEBUFFER);
	}

	Holder<FrameBuffer> newFrameBufferRead()
	{
		return systemMemory().createImpl<FrameBuffer, FrameBufferImpl>(GL_READ_FRAMEBUFFER);
	}
}
