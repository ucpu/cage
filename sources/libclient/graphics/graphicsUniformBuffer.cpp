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
		class uniformBufferImpl : public uniformBufferClass
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

	uint32 uniformBufferClass::getId() const
	{
		return ((uniformBufferImpl*)this)->id;
	}

	void uniformBufferClass::bind() const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		glBindBuffer(GL_UNIFORM_BUFFER, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<uniformBufferClass>(impl->id);
	}

	void uniformBufferClass::bind(uint32 bindingPoint) const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void uniformBufferClass::bind(uint32 bindingPoint, uint32 offset, uint32 size) const
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, impl->id, offset, size);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void uniformBufferClass::writeWhole(void *data, uint32 size, uint32 usage)
	{
		if (usage == 0)
			usage = GL_STATIC_DRAW;
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<uniformBufferClass>() == impl->id);
		glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
		CAGE_CHECK_GL_ERROR_DEBUG();
		impl->size = size;
	}

	void uniformBufferClass::writeRange(void *data, uint32 offset, uint32 size)
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentObject<uniformBufferClass>() == impl->id);
		CAGE_ASSERT_RUNTIME(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	holder<uniformBufferClass> newUniformBuffer(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext() == context);
		return detail::systemArena().createImpl <uniformBufferClass, uniformBufferImpl>();
	}
}