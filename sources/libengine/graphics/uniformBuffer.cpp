#include <cage-engine/opengl.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class UniformBufferImpl : public UniformBuffer
		{
		public:
			const UniformBufferCreateConfig config;
			void *mapped;
			uint32 id;
			uint32 size;

			explicit UniformBufferImpl(const UniformBufferCreateConfig &config) : config(config), mapped(nullptr), id(0), size(0)
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

			~UniformBufferImpl()
			{
				glDeleteBuffers(1, &id);
			}
		};
	}

	void UniformBuffer::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		glObjectLabel(GL_BUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 UniformBuffer::getId() const
	{
		return ((UniformBufferImpl*)this)->id;
	}

	void UniformBuffer::bind() const
	{
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		glBindBuffer(GL_UNIFORM_BUFFER, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<UniformBuffer>(impl->id);
	}

	void UniformBuffer::bind(uint32 bindingPoint) const
	{
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void UniformBuffer::bind(uint32 bindingPoint, uint32 offset, uint32 size) const
	{
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		CAGE_ASSERT(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		CAGE_ASSERT((offset % getAlignmentRequirement()) == 0, "the offset must be aligned", offset, getAlignmentRequirement());
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, impl->id, offset, size);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void UniformBuffer::writeWhole(const void *data, uint32 size, uint32 usage)
	{
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		if (impl->mapped || impl->config.size)
		{
			CAGE_ASSERT(usage == 0, "buffer storage is immutable");
			CAGE_ASSERT(size == impl->size, "buffer storage is immutable");
			writeRange(data, 0, size);
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
			if (usage == 0)
				usage = GL_STATIC_DRAW;
			glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
			CAGE_CHECK_GL_ERROR_DEBUG();
			impl->size = size;
		}
	}

	void UniformBuffer::writeRange(const void *data, uint32 offset, uint32 size)
	{
		UniformBufferImpl *impl = (UniformBufferImpl*)this;
		CAGE_ASSERT(offset + size <= impl->size, "insufficient buffer size", offset, size, impl->size);
		if (impl->mapped)
		{
			detail::memcpy((char*)impl->mapped + offset, data, size);
			if (impl->config.explicitFlush)
			{
				CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
				glFlushMappedBufferRange(GL_UNIFORM_BUFFER, offset, size);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
	}

	uint32 UniformBuffer::getSize() const
	{
		const UniformBufferImpl *impl = (const UniformBufferImpl*)this;
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

	uint32 UniformBuffer::getAlignmentRequirement()
	{
		static uint32 alignment = getAlignmentRequirementImpl();
		return alignment;
	}

	UniformBufferCreateConfig::UniformBufferCreateConfig() : size(0), persistentMapped(false), coherentMapped(false), explicitFlush(false)
	{}

	Holder<UniformBuffer> newUniformBuffer(const UniformBufferCreateConfig &config)
	{
		return detail::systemArena().createImpl<UniformBuffer, UniformBufferImpl>(config);
	}
}
