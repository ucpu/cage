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
		class uniformBufferImpl : public uniformBuffer
		{
		public:
			uint32 id;
			uint32 size;

			uniformBufferImpl() : id(0), size(0)
			{
				glGenBuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~uniformBufferImpl()
			{
				glDeleteBuffers(1, &id);
			}
		};
	}

	void uniformBuffer::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		glObjectLabel(GL_BUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 uniformBuffer::getId() const
	{
		return ((uniformBufferImpl*)this)->id;
	}

	void uniformBuffer::bind() const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		glBindBuffer(GL_UNIFORM_BUFFER, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<uniformBuffer>(impl->id);
	}

	void uniformBuffer::bind(uint32 bindingPoint) const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void uniformBuffer::bind(uint32 bindingPoint, uint32 offset, uint32 size) const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, impl->id, offset, size);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void uniformBuffer::writeWhole(void *data, uint32 size, uint32 usage)
	{
		if (usage == 0)
			usage = GL_STATIC_DRAW;
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<uniformBuffer>() == impl->id);
		glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
		CAGE_CHECK_GL_ERROR_DEBUG();
		impl->size = size;
	}

	void uniformBuffer::writeRange(void *data, uint32 offset, uint32 size)
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<uniformBuffer>() == impl->id);
		CAGE_ASSERT(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	holder<uniformBuffer> newUniformBuffer()
	{
		return detail::systemArena().createImpl<uniformBuffer, uniformBufferImpl>();
	}
}
