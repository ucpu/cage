#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/memory/memoryDetail.h> // addToAlign
#include <cage-core/utility/pointer.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/graphic/shaderConventions.h>
#include <cage-client/opengl.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class meshImpl : public meshClass
		{
		public:
			uint32 id;
			uint32 vbo;
			uint32 verticesCount;
			uint32 verticesOffset;
			uint32 indicesCount;
			uint32 indicesOffset;
			uint32 materialSize;
			uint32 materialOffset;
			meshFlags flags;
			uint32 primitiveType;
			aabb box;
			uint32 textures[MaxTexturesCountPerMaterial];

			meshImpl() : id(0), vbo(0), verticesCount(0), verticesOffset(0), indicesCount(0), indicesOffset(0), materialSize(0), materialOffset(0), flags(meshFlags::None), primitiveType(GL_TRIANGLES)
			{
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					textures[i] = 0;
				glGenVertexArrays(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~meshImpl()
			{
				glDeleteVertexArrays(1, &id);
				if (vbo)
					glDeleteBuffers(1, &vbo);
			}
		};
	}

	uint32 meshClass::getId() const
	{
		return ((meshImpl*)this)->id;
	}

	void meshClass::bind() const
	{
		meshImpl *impl = (meshImpl*)this;
		glBindVertexArray(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (impl->materialSize)
		{
			glBindBufferRange(GL_UNIFORM_BUFFER, CAGE_SHADER_UNIBLOCK_MATERIAL, impl->vbo, impl->materialOffset, impl->materialSize);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		setCurrentObject<meshClass>(impl->id);
		setCurrentObject<uniformBufferClass>(impl->id);
	}

	void meshClass::setFlags(meshFlags flags)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->flags = flags;
	}

	void meshClass::setPrimitiveType(uint32 type)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->primitiveType = type;
	}

	void meshClass::setBoundingBox(const aabb &box)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->box = box;
	}

	void meshClass::setTextures(const uint32 *textureNames)
	{
		meshImpl *impl = (meshImpl*)this;
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
			impl->textures[i] = textureNames[i];
	}

	void meshClass::setBuffers(uint32 verticesCount, uint32 vertexSize, void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, void *materialData)
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicPrivat::getCurrentObject<meshClass>() == impl->id);
		{
			if (impl->vbo)
			{
				glDeleteBuffers(1, &impl->vbo);
				impl->vbo = 0;
			}
			glGenBuffers(1, &impl->vbo);
			glBindBuffer(GL_UNIFORM_BUFFER, impl->vbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, impl->vbo);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		impl->verticesCount = verticesCount;
		impl->indicesCount = indicesCount;
		impl->materialSize = materialSize;

		uint32 BufferAlignment = 256;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&BufferAlignment);

		uint32 bufferSize = numeric_cast<uint32>(
			verticesCount * vertexSize + detail::addToAlign(verticesCount * vertexSize, BufferAlignment) +
			indicesCount * sizeof(uint32) + detail::addToAlign(indicesCount * sizeof(uint32), BufferAlignment) +
			materialSize + detail::addToAlign(materialSize, BufferAlignment)
			);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
		CAGE_CHECK_GL_ERROR_DEBUG();

		uint32 offset = 0;

		{ // vertices
			impl->verticesOffset = offset;
			glBufferSubData(GL_ARRAY_BUFFER, offset, verticesCount * vertexSize, vertexData);
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += verticesCount * vertexSize + numeric_cast<uint32>(detail::addToAlign(verticesCount * vertexSize, BufferAlignment));
		}

		{ // indices
			impl->indicesOffset = offset;
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, indicesCount * sizeof(uint32), indexData);
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += indicesCount * sizeof(uint32) + numeric_cast<uint32>(detail::addToAlign(indicesCount * sizeof(uint32), BufferAlignment));
		}

		{ // material
			impl->materialOffset = offset;
			glBufferSubData(GL_UNIFORM_BUFFER, offset, materialSize, materialData);
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += materialSize + numeric_cast<uint32>(detail::addToAlign(materialSize, BufferAlignment));
		}
	}

	void meshClass::setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, void *data)
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicPrivat::getCurrentObject<meshClass>() == impl->id);
		if (type == 0)
			glDisableVertexAttribArray(index);
		else
		{
			glEnableVertexAttribArray(index);
			data = pointer(data) + numeric_cast<uintPtr>(impl->verticesOffset);
			switch (type)
			{
			case GL_BYTE:
			case GL_UNSIGNED_BYTE:
			case GL_SHORT:
			case GL_UNSIGNED_SHORT:
			case GL_INT:
			case GL_UNSIGNED_INT:
				glVertexAttribIPointer(index, size, type, stride, data);
				break;
			case GL_DOUBLE:
				glVertexAttribLPointer(index, size, type, stride, data);
				break;
			default:
				glVertexAttribPointer(index, size, type, GL_FALSE, stride, data);
			}
		}
	}

	meshFlags meshClass::getFlags() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->flags;
	}

	aabb meshClass::getBoundingBox() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->box;
	}

	uint32 meshClass::textureName(uint32 texIdx) const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT_RUNTIME(texIdx < MaxTexturesCountPerMaterial, texIdx, MaxTexturesCountPerMaterial);
		return impl->textures[texIdx];
	}

	void meshClass::dispatch() const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicPrivat::getCurrentObject<meshClass>() == impl->id);
		if (impl->indicesCount)
			glDrawElements(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset);
		else
			glDrawArrays(impl->primitiveType, 0, impl->verticesCount);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void meshClass::dispatch(uint32 instances) const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT_RUNTIME(graphicPrivat::getCurrentObject<meshClass>() == impl->id);
		if (impl->indicesCount)
			glDrawElementsInstanced(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset, instances);
		else
			glDrawArraysInstanced(impl->primitiveType, 0, impl->verticesCount, instances);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	holder<meshClass> newMesh(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(graphicPrivat::getCurrentContext() == context);
		return detail::systemArena().createImpl <meshClass, meshImpl>();
	}
}