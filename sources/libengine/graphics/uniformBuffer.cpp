#include <cage-engine/opengl.h>
#include <cage-engine/uniformBuffer.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class UniformBufferImpl : public UniformBuffer
		{
		public:
			const UniformBufferCreateConfig config;
			char *mapped = nullptr;
			uint32 size = 0;
			uint32 id = 0;

			explicit UniformBufferImpl(const UniformBufferCreateConfig &config) : config(config)
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
						mapped = (char*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, accessFlags);
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

	void UniformBuffer::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		UniformBufferImpl *impl = (UniformBufferImpl *)this;
		CAGE_ASSERT(impl->id);
		glObjectLabel(GL_BUFFER, impl->id, name.length(), name.c_str());
	}

	uint32 UniformBuffer::id() const
	{
		return ((UniformBufferImpl *)this)->id;
	}

	void UniformBuffer::bind() const
	{
		const UniformBufferImpl *impl = (const UniformBufferImpl *)this;
		glBindBuffer(GL_UNIFORM_BUFFER, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		setCurrentObject<UniformBuffer>(impl->id);
	}

	void UniformBuffer::bind(uint32 bindingPoint) const
	{
		const UniformBufferImpl *impl = (const UniformBufferImpl *)this;
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void UniformBuffer::bind(uint32 bindingPoint, uint32 offset, uint32 size) const
	{
		const UniformBufferImpl *impl = (const UniformBufferImpl *)this;
		CAGE_ASSERT(offset + size <= impl->size);
		CAGE_ASSERT((offset % alignmentRequirement()) == 0);
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, impl->id, offset, size);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void UniformBuffer::writeWhole(PointerRange<const char> buffer, uint32 usage)
	{
		UniformBufferImpl *impl = (UniformBufferImpl *)this;
		if (impl->mapped || impl->config.size)
		{
			CAGE_ASSERT(usage == 0);
			CAGE_ASSERT(buffer.size() == impl->size);
			writeRange(buffer, 0);
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
			if (usage == 0)
				usage = GL_STATIC_DRAW;
			glBufferData(GL_UNIFORM_BUFFER, buffer.size(), buffer.data(), usage);
			CAGE_CHECK_GL_ERROR_DEBUG();
			impl->size = numeric_cast<uint32>(buffer.size());
		}
	}

	void UniformBuffer::writeRange(PointerRange<const char> buffer, uint32 offset)
	{
		UniformBufferImpl *impl = (UniformBufferImpl *)this;
		CAGE_ASSERT(offset + buffer.size() <= impl->size);
		if (impl->mapped)
		{
			detail::memcpy(impl->mapped + offset, buffer.data(), buffer.size());
			if (impl->config.explicitFlush)
			{
				CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
				glFlushMappedBufferRange(GL_UNIFORM_BUFFER, offset, buffer.size());
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		}
		else
		{
			CAGE_ASSERT(graphicsPrivat::getCurrentObject<UniformBuffer>() == impl->id);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, buffer.size(), buffer.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
	}

	uint32 UniformBuffer::size() const
	{
		const UniformBufferImpl *impl = (const UniformBufferImpl *)this;
		return impl->size;
	}

	namespace
	{
		uint32 alignmentRequirementImpl()
		{
			GLint alignment = 256;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
			return numeric_cast<uint32>(alignment);
		}
	}

	uint32 UniformBuffer::alignmentRequirement()
	{
		static const uint32 alignment = alignmentRequirementImpl();
		return alignment;
	}

	Holder<UniformBuffer> newUniformBuffer(const UniformBufferCreateConfig &config)
	{
		return systemMemory().createImpl<UniformBuffer, UniformBufferImpl>(config);
	}
}
