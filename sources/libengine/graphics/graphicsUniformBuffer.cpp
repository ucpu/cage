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
			const uniformBufferCreateConfig config;
			void *mapped;
			uint32 id;
			uint32 size;

			uniformBufferImpl(const uniformBufferCreateConfig &config) : config(config), mapped(nullptr), id(0), size(0)
			{
				CAGE_ASSERT(!config.explicitFlush || config.persistentMapped);
				CAGE_ASSERT(!config.coherentMapped || config.persistentMapped);
				CAGE_ASSERT(!config.persistentMapped || config.size > 0)
				glGenBuffers(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
				if (config.size > 0)
				{
					{
						uint32 storageFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT;
						if (config.persistentMapped)
							storageFlags |= GL_MAP_PERSISTENT_BIT;
						if (config.coherentMapped)
							storageFlags |= GL_MAP_COHERENT_BIT;
						glBufferStorage(GL_UNIFORM_BUFFER, config.size, nullptr, storageFlags);
					}
					CAGE_CHECK_GL_ERROR_DEBUG();
					size = config.size;
					if (config.persistentMapped)
					{
						uint32 accessFlags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
						if (config.persistentMapped)
							accessFlags |= GL_MAP_PERSISTENT_BIT;
						if (config.coherentMapped)
							accessFlags |= GL_MAP_COHERENT_BIT;
						if (config.explicitFlush)
							accessFlags |= GL_MAP_FLUSH_EXPLICIT_BIT;
						mapped = glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, accessFlags);
						CAGE_CHECK_GL_ERROR_DEBUG();
						CAGE_ASSERT(mapped);
					}
				}
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
		CAGE_ASSERT((offset % getAlignmentRequirement()) == 0, "the offset must be aligned", offset, getAlignmentRequirement());
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, impl->id, offset, size);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void uniformBuffer::writeWhole(const void *data, uint32 size, uint32 usage)
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		if (impl->mapped || impl->config.size)
		{
			CAGE_ASSERT(usage == 0, "buffer storage is immutable");
			CAGE_ASSERT(size == impl->size, "buffer storage is immutable");
			writeRange(data, 0, size);
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<uniformBuffer>() == impl->id);
			if (usage == 0)
				usage = GL_STATIC_DRAW;
			glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
			CAGE_CHECK_GL_ERROR_DEBUG();
			impl->size = size;
		}
	}

	void uniformBuffer::writeRange(const void *data, uint32 offset, uint32 size)
	{
		uniformBufferImpl *impl = (uniformBufferImpl*)this;
		CAGE_ASSERT(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		if (impl->mapped)
		{
			detail::memcpy((char*)impl->mapped + offset, data, size);
			if (impl->config.explicitFlush)
			{
				CAGE_ASSERT(graphicsPrivat::getCurrentObject<uniformBuffer>() == impl->id);
				glFlushMappedBufferRange(GL_UNIFORM_BUFFER, offset, size);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<uniformBuffer>() == impl->id);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
	}

	uint32 uniformBuffer::getSize() const
	{
		const uniformBufferImpl *impl = (const uniformBufferImpl*)this;
		return impl->size;
	}

	namespace
	{
		uint32 getAlignmentRequirementImpl()
		{
			uint32 alignment = 256;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&alignment);
			return alignment;
		}
	}

	uint32 uniformBuffer::getAlignmentRequirement()
	{
		static uint32 alignment = getAlignmentRequirementImpl();
		return alignment;
	}

	uniformBufferCreateConfig::uniformBufferCreateConfig() : size(0), persistentMapped(false), coherentMapped(false), explicitFlush(false)
	{}

	Holder<uniformBuffer> newUniformBuffer(const uniformBufferCreateConfig &config)
	{
		return detail::systemArena().createImpl<uniformBuffer, uniformBufferImpl>(config);
	}
}
