#include <cage-core/geometry.h>
#include <cage-core/memory.h> // addToAlign

#include <cage-engine/shaderConventions.h>
#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include "private.h"

namespace cage
{
	MeshHeader::MaterialData::MaterialData()
	{
		albedoBase[3] = 1;
		albedoMult = specialMult = vec4(1);
	}

	namespace
	{
		class MeshImpl : public Mesh
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
			MeshRenderFlags flags;

			static constexpr MeshRenderFlags defaultFlags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::VelocityWrite | MeshRenderFlags::Lighting | MeshRenderFlags::ShadowCast;

			MeshImpl() : box(aabb::Universe()), id(0), vbo(0), verticesCount(0), verticesOffset(0), indicesCount(0), indicesOffset(0), materialSize(0), materialOffset(0), primitiveType(GL_TRIANGLES), primitivesCount(0), skeletonName(0), skeletonBones(0), instancesLimitHint(1), flags(defaultFlags)
			{
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					textures[i] = 0;
				glGenVertexArrays(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~MeshImpl()
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

	void Mesh::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		MeshImpl *impl = (MeshImpl*)this;
		glObjectLabel(GL_VERTEX_ARRAY, impl->id, name.length(), name.c_str());
	}

	uint32 Mesh::getId() const
	{
		return ((MeshImpl*)this)->id;
	}

	void Mesh::bind() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		glBindVertexArray(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (impl->materialSize)
		{
			glBindBufferRange(GL_UNIFORM_BUFFER, CAGE_SHADER_UNIBLOCK_MATERIAL, impl->vbo, impl->materialOffset, impl->materialSize);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		setCurrentObject<Mesh>(impl->id);
		setCurrentObject<UniformBuffer>(impl->id);
	}

	void Mesh::setFlags(MeshRenderFlags flags)
	{
		MeshImpl *impl = (MeshImpl*)this;
		impl->flags = flags;
	}

	void Mesh::setPrimitiveType(uint32 type)
	{
		MeshImpl *impl = (MeshImpl*)this;
		impl->primitiveType = type;
		impl->updatePrimitivesCount();
	}

	void Mesh::setBoundingBox(const aabb &box)
	{
		MeshImpl *impl = (MeshImpl*)this;
		impl->box = box;
	}

	void Mesh::setTextureNames(const uint32 *textureNames)
	{
		MeshImpl *impl = (MeshImpl*)this;
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
			impl->textures[i] = textureNames[i];
	}

	void Mesh::setTextureName(uint32 texIdx, uint32 name)
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(texIdx < MaxTexturesCountPerMaterial);
		impl->textures[texIdx] = name;
	}

	void Mesh::setBuffers(uint32 verticesCount, uint32 vertexSize, const void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, const void *MaterialData)
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<Mesh>() == impl->id);
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
			glBufferSubData(GL_UNIFORM_BUFFER, offset, materialSize, MaterialData);
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += materialSize + numeric_cast<uint32>(detail::addToAlign(materialSize, BufferAlignment));
		}

		impl->updatePrimitivesCount();
	}

	void Mesh::setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset)
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<Mesh>() == impl->id);
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

	void Mesh::setSkeleton(uint32 name, uint32 bones)
	{
		MeshImpl *impl = (MeshImpl*)this;
		impl->skeletonName = name;
		impl->skeletonBones = bones;
	}

	void Mesh::setInstancesLimitHint(uint32 limit)
	{
		MeshImpl *impl = (MeshImpl*)this;
		impl->instancesLimitHint = limit;
	}

	uint32 Mesh::getVerticesCount() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->verticesCount;
	}

	uint32 Mesh::getIndicesCount() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->indicesCount;
	}

	uint32 Mesh::getPrimitivesCount() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->primitivesCount;
	}

	MeshRenderFlags Mesh::getFlags() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->flags;
	}

	aabb Mesh::getBoundingBox() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->box;
	}

	const uint32 *Mesh::getTextureNames() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->textures;
	}

	uint32 Mesh::getTextureName(uint32 texIdx) const
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(texIdx < MaxTexturesCountPerMaterial);
		return impl->textures[texIdx];
	}

	uint32 Mesh::getSkeletonName() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->skeletonName;
	}

	uint32 Mesh::getSkeletonBones() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->skeletonBones;
	}

	uint32 Mesh::getInstancesLimitHint() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		return impl->instancesLimitHint;
	}

	void Mesh::dispatch() const
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<Mesh>() == impl->id);
		if (impl->indicesCount)
			glDrawElements(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset);
		else
			glDrawArrays(impl->primitiveType, 0, impl->verticesCount);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Mesh::dispatch(uint32 instances) const
	{
		MeshImpl *impl = (MeshImpl*)this;
		CAGE_ASSERT(graphicsPrivat::getCurrentObject<Mesh>() == impl->id);
		if (impl->indicesCount)
			glDrawElementsInstanced(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void*)(uintPtr)impl->indicesOffset, instances);
		else
			glDrawArraysInstanced(impl->primitiveType, 0, impl->verticesCount, instances);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	Holder<Mesh> newMesh()
	{
		return detail::systemArena().createImpl<Mesh, MeshImpl>();
	}
}
