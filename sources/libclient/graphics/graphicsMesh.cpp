#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/memory.h> // addToAlign

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/opengl.h>
#include <cage-client/assetStructs.h>
#include "private.h"

namespace cage
{
	renderMeshHeader::materialData::materialData()
	{
		albedoBase[3] = 1;
		albedoMult = specialMult = vec4(1);
	}

	namespace
	{
		class meshImpl : public renderMesh
		{
		public:
			aabb box;
			uint32 textures[MaxTexturesCountPerMaterial];
			uint32 id;
			uint32 vbo;
			uint32 verticesCount;
			uint32 verticesOffset;
			uint32 indicesCount;
			uint32 indicesOffset;
			uint32 materialSize;
			uint32 materialOffset;
			uint32 primitiveType;
			uint32 primitivesCount;
			uint32 skeletonName;
			uint32 skeletonBones;
			uint32 instancesLimitHint;
			meshRenderFlags flags;

			static const meshRenderFlags defaultFlags = meshRenderFlags::DepthTest | meshRenderFlags::DepthWrite | meshRenderFlags::VelocityWrite | meshRenderFlags::Lighting | meshRenderFlags::ShadowCast;

			meshImpl() : box(aabb::Universe()), id(0), vbo(0), verticesCount(0), verticesOffset(0), indicesCount(0), indicesOffset(0), materialSize(0), materialOffset(0), primitiveType(GL_TRIANGLES), primitivesCount(0), skeletonName(0), skeletonBones(0), instancesLimitHint(1), flags(defaultFlags)
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

			void updatePrimitivesCount()
			{
				uint32 cnt = indicesCount ? indicesCount : verticesCount;
				switch (primitiveType)
				{
				case GL_POINTS:
					primitivesCount = cnt;
					break;
				case GL_LINE_STRIP:
					primitivesCount = cnt - 1;
					break;
				case GL_LINE_LOOP:
					primitivesCount = cnt;
					break;
				case GL_LINES:
					primitivesCount = cnt / 2;
					break;
				case GL_TRIANGLE_STRIP:
					primitivesCount = cnt - 2;
					break;
				case GL_TRIANGLE_FAN:
					primitivesCount = cnt - 2;
					break;
				case GL_TRIANGLES:
					primitivesCount = cnt / 3;
					break;
				default:
					primitivesCount = cnt / 4; // we do not want to fail, so just return an estimate
					break;
				}
			}
		};
	}

	void renderMesh::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		meshImpl *impl = (meshImpl*)this;
		glObjectLabel(GL_VERTEX_ARRAY, impl->id, name.length(), name.c_str());
	}

	uint32 renderMesh::getId() const
	{
		return ((meshImpl*)this)->id;
	}

	void renderMesh::bind() const
	{
		meshImpl *impl = (meshImpl*)this;
		glBindVertexArray(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (impl->materialSize)
		{
			glBindBufferRange(GL_UNIFORM_BUFFER, CAGE_SHADER_UNIBLOCK_MATERIAL, impl->vbo, impl->materialOffset, impl->materialSize);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		setCurrentObject<renderMesh>(impl->id);
		setCurrentObject<uniformBuffer>(impl->id);
	}

	void renderMesh::setFlags(meshRenderFlags flags)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->flags = flags;
	}

	void renderMesh::setPrimitiveType(uint32 type)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->primitiveType = type;
		impl->updatePrimitivesCount();
	}

	void renderMesh::setBoundingBox(const aabb &box)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->box = box;
	}

	void renderMesh::setTextureNames(const uint32 *textureNames)
	{
		meshImpl *impl = (meshImpl*)this;
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
			impl->textures[i] = textureNames[i];
	}

	void renderMesh::setTextureName(uint32 texIdx, uint32 name)
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(texIdx < MaxTexturesCountPerMaterial, texIdx, MaxTexturesCountPerMaterial);
		impl->textures[texIdx] = name;
	}

	void renderMesh::setBuffers(uint32 verticesCount, uint32 vertexSize, const void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, const void *materialData)
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<renderMesh>() == impl->id);
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

		impl->updatePrimitivesCount();
	}

	void renderMesh::setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset)
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<renderMesh>() == impl->id);
		if (type == 0)
			glDisableVertexAttribArray(index);
		else
		{
			glEnableVertexAttribArray(index);
			void *data = (void*)(uintPtr)(startOffset + numeric_cast<uint32>(impl->verticesOffset));
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

	void renderMesh::setSkeleton(uint32 name, uint32 bones)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->skeletonName = name;
		impl->skeletonBones = bones;
	}

	void renderMesh::setInstancesLimitHint(uint32 limit)
	{
		meshImpl *impl = (meshImpl*)this;
		impl->instancesLimitHint = limit;
	}

	uint32 renderMesh::getVerticesCount() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->verticesCount;
	}

	uint32 renderMesh::getIndicesCount() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->indicesCount;
	}

	uint32 renderMesh::getPrimitivesCount() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->primitivesCount;
	}

	meshRenderFlags renderMesh::getFlags() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->flags;
	}

	aabb renderMesh::getBoundingBox() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->box;
	}

	const uint32 *renderMesh::getTextureNames() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->textures;
	}

	uint32 renderMesh::getTextureName(uint32 texIdx) const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(texIdx < MaxTexturesCountPerMaterial, texIdx, MaxTexturesCountPerMaterial);
		return impl->textures[texIdx];
	}

	uint32 renderMesh::getSkeletonName() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->skeletonName;
	}

	uint32 renderMesh::getSkeletonBones() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->skeletonBones;
	}

	uint32 renderMesh::getInstancesLimitHint() const
	{
		meshImpl *impl = (meshImpl*)this;
		return impl->instancesLimitHint;
	}

	void renderMesh::dispatch() const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<renderMesh>() == impl->id);
		if (impl->indicesCount)
			glDrawElements(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset);
		else
			glDrawArrays(impl->primitiveType, 0, impl->verticesCount);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void renderMesh::dispatch(uint32 instances) const
	{
		meshImpl *impl = (meshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<renderMesh>() == impl->id);
		if (impl->indicesCount)
			glDrawElementsInstanced(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset, instances);
		else
			glDrawArraysInstanced(impl->primitiveType, 0, impl->verticesCount, instances);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	holder<renderMesh> newRenderMesh()
	{
		return detail::systemArena().createImpl<renderMesh, meshImpl>();
	}
}
