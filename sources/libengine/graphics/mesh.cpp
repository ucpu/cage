#include <cage-core/geometry.h>
#include <cage-core/polyhedron.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memory.h> // addToAlign

#include <cage-engine/shaderConventions.h>
#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include "private.h"

#include <vector>

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
			static constexpr MeshRenderFlags defaultFlags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::VelocityWrite | MeshRenderFlags::Lighting | MeshRenderFlags::ShadowCast;

		public:
			aabb box = aabb::Universe();
			uint32 textures[MaxTexturesCountPerMaterial];
			uint32 id = 0;
			uint32 vbo = 0;
			uint32 verticesCount = 0;
			uint32 indicesCount = 0;
			uint32 indicesOffset = 0;
			uint32 materialSize = 0;
			uint32 materialOffset = 0;
			uint32 primitiveType = GL_TRIANGLES;
			uint32 primitivesCount = 0;
			uint32 skeletonName = 0;
			uint32 skeletonBones = 0;
			uint32 instancesLimitHint = 1;
			MeshRenderFlags flags = defaultFlags;

			MeshImpl()
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
				case GL_LINE_LOOP:
					primitivesCount = cnt;
					break;
				case GL_LINE_STRIP:
					primitivesCount = cnt - 1;
					break;
				case GL_LINES:
					primitivesCount = cnt / 2;
					break;
				case GL_TRIANGLE_STRIP:
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

	void Mesh::importPolyhedron(const Polyhedron *poly, PointerRange<const char> materialBuffer)
	{
		const uint32 verticesCount = poly->verticesCount();
		uint32 vertexSize = 0;
		uint32 offset = 0;
		MemoryBuffer vts;
		vts.reserve(verticesCount * 32);

		struct Attr
		{
			uint32 i;
			uint32 c;
			uint32 t;
			uint32 s;
			uint32 o;

			Attr(uint32 i, uint32 c, uint32 t, uint32 s, uint32 o) : i(i), c(c), t(t), s(s), o(o)
			{}
		};
		std::vector<Attr> attrs;

		if (!poly->positions().empty())
		{
			constexpr uint32 attrSize = sizeof(vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->positions().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_POSITION, 3, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->normals().empty())
		{
			constexpr uint32 attrSize = sizeof(vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->normals().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_NORMAL, 3, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->uvs().empty())
		{
			constexpr uint32 attrSize = sizeof(vec2);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->uvs().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_UV, 2, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->uvs3().empty())
		{
			constexpr uint32 attrSize = sizeof(vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->uvs3().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_UV, 3, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->tangents().empty())
		{
			constexpr uint32 attrSize = sizeof(vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->tangents().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_TANGENT, 3, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->bitangents().empty())
		{
			constexpr uint32 attrSize = sizeof(vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->bitangents().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_BITANGENT, 3, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->boneIndices().empty())
		{
			constexpr uint32 attrSize = sizeof(ivec4);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->boneIndices().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_BONEINDEX, 4, GL_UNSIGNED_INT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->boneWeights().empty())
		{
			constexpr uint32 attrSize = sizeof(vec4);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->boneWeights().data(), bufSize);
			attrs.emplace_back(CAGE_SHADER_ATTRIB_IN_BONEWEIGHT, 4, GL_FLOAT, attrSize, offset);
			vertexSize += attrSize;
			offset += bufSize;
		}

		CAGE_ASSERT(vts.size() == verticesCount * vertexSize);
		setBuffers(vertexSize, vts, poly->indices(), materialBuffer);

		switch (poly->type())
		{
		case PolyhedronTypeEnum::Points: setPrimitiveType(GL_POINTS); break;
		case PolyhedronTypeEnum::Lines: setPrimitiveType(GL_LINES); break;
		case PolyhedronTypeEnum::Triangles: setPrimitiveType(GL_TRIANGLES); break;
		default: CAGE_THROW_CRITICAL(Exception, "invalid polyhedron type");
		}

		for (const Attr &a : attrs)
			setAttribute(a.i, a.c, a.t, a.s, a.o);

		setBoundingBox(poly->boundingBox());
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

	void Mesh::setTextureNames(PointerRange<const uint32> textureNames)
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

	void Mesh::setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer)
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
		impl->verticesCount = numeric_cast<uint32>(vertexData.size() / vertexSize);
		impl->indicesCount = numeric_cast<uint32>(indexData.size());
		impl->materialSize = numeric_cast<uint32>(materialBuffer.size());
		CAGE_ASSERT(vertexData.size() == vertexSize * impl->verticesCount);

		uint32 bufferAlignment = 256;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&bufferAlignment);

		const uint32 bufferSize = numeric_cast<uint32>(
			impl->verticesCount * vertexSize + detail::addToAlign(impl->verticesCount * vertexSize, bufferAlignment) +
			impl->indicesCount * sizeof(uint32) + detail::addToAlign(impl->indicesCount * sizeof(uint32), bufferAlignment) +
			impl->materialSize + detail::addToAlign(materialBuffer.size(), bufferAlignment)
			);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
		CAGE_CHECK_GL_ERROR_DEBUG();

		uint32 offset = 0;

		{ // vertices
			glBufferSubData(GL_ARRAY_BUFFER, offset, impl->verticesCount * vertexSize, vertexData.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += impl->verticesCount * vertexSize + numeric_cast<uint32>(detail::addToAlign(impl->verticesCount * vertexSize, bufferAlignment));
		}

		{ // indices
			impl->indicesOffset = offset;
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, impl->indicesCount * sizeof(uint32), indexData.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += impl->indicesCount * sizeof(uint32) + numeric_cast<uint32>(detail::addToAlign(impl->indicesCount * sizeof(uint32), bufferAlignment));
		}

		{ // material
			impl->materialOffset = offset;
			glBufferSubData(GL_UNIFORM_BUFFER, offset, materialBuffer.size(), materialBuffer.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += numeric_cast<uint32>(materialBuffer.size() + detail::addToAlign(materialBuffer.size(), bufferAlignment));
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
			void *data = (void*)(uintPtr)startOffset;
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

	PointerRange<const uint32> Mesh::getTextureNames() const
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
