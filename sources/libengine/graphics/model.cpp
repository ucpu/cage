#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h> // addToAlign
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>

#include <cage-engine/assetStructs.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/uniformBuffer.h>

#include <vector>

namespace cage
{
	namespace
	{
#ifdef CAGE_ASSERT_ENABLED
		thread_local uint32 boundModel = 0;
#endif // CAGE_ASSERT_ENABLED

		class ModelImpl : public Model
		{
		public:
			Aabb box = Aabb::Universe();
			uint32 id = 0;
			uint32 vbo = 0;
			uint32 verticesCount = 0;
			uint32 indicesCount = 0;
			uint32 indicesOffset = 0;
			uint32 materialSize = 0;
			uint32 materialOffset = 0;
			uint32 primitiveType = GL_TRIANGLES;
			uint32 primitivesCount = 0;

			ModelImpl()
			{
				flags = MeshRenderFlags::Default;
				glGenVertexArrays(1, &id);
				CAGE_CHECK_GL_ERROR_DEBUG();
				bind();
			}

			~ModelImpl()
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

	void Model::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		ModelImpl *impl = (ModelImpl *)this;
		CAGE_ASSERT(impl->id);
		glObjectLabel(GL_VERTEX_ARRAY, impl->id, name.length(), name.c_str());
		if (impl->vbo)
			glObjectLabel(GL_BUFFER, impl->vbo, name.length(), name.c_str());
	}

	uint32 Model::id() const
	{
		return ((ModelImpl *)this)->id;
	}

	void Model::bind() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		glBindVertexArray(impl->id);
		CAGE_CHECK_GL_ERROR_DEBUG();
		if (impl->materialSize)
		{
			glBindBufferRange(GL_UNIFORM_BUFFER, CAGE_SHADER_UNIBLOCK_MATERIAL, impl->vbo, impl->materialOffset, impl->materialSize);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
#ifdef CAGE_ASSERT_ENABLED
		boundModel = impl->id;
#endif // CAGE_ASSERT_ENABLED
	}

	void Model::importMesh(const Mesh *poly, PointerRange<const char> materialBuffer)
	{
		const uint32 verticesCount = poly->verticesCount();
		uint32 vertexSize = 0;
		uint32 offset = 0;
		MemoryBuffer vts;
		vts.reserve(verticesCount * 32);

		struct Attr
		{
			uint32 index;
			uint32 count;
			uint32 type;
			uint32 stride;
			uint32 offset;
		};
		std::vector<Attr> attrs;

		if (!poly->positions().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->positions().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_POSITION, 3, GL_FLOAT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->normals().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->normals().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_NORMAL, 3, GL_FLOAT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->uvs().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec2);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->uvs().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_UV, 2, GL_FLOAT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->uvs3().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec3);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->uvs3().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_UV, 3, GL_FLOAT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->boneIndices().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec4i);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->boneIndices().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_BONEINDEX, 4, GL_UNSIGNED_INT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		if (!poly->boneWeights().empty())
		{
			static constexpr uint32 attrSize = sizeof(Vec4);
			const uint32 bufSize = attrSize * verticesCount;
			vts.resize(offset + bufSize);
			detail::memcpy(vts.data() + offset, poly->boneWeights().data(), bufSize);
			attrs.push_back({ CAGE_SHADER_ATTRIB_IN_BONEWEIGHT, 4, GL_FLOAT, attrSize, offset });
			vertexSize += attrSize;
			offset += bufSize;
		}

		CAGE_ASSERT(vts.size() == verticesCount * vertexSize);
		setBuffers(vertexSize, vts, poly->indices(), materialBuffer);

		switch (poly->type())
		{
			case MeshTypeEnum::Points:
				setPrimitiveType(GL_POINTS);
				break;
			case MeshTypeEnum::Lines:
				setPrimitiveType(GL_LINES);
				break;
			case MeshTypeEnum::Triangles:
				setPrimitiveType(GL_TRIANGLES);
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
		}

		for (const Attr &a : attrs)
			setAttribute(a.index, a.count, a.type, a.stride, a.offset);

		setBoundingBox(poly->boundingBox());
	}

	void Model::setPrimitiveType(uint32 type)
	{
		ModelImpl *impl = (ModelImpl *)this;
		impl->primitiveType = type;
		impl->updatePrimitivesCount();
	}

	void Model::setBoundingBox(const Aabb &box)
	{
		ModelImpl *impl = (ModelImpl *)this;
		impl->box = box;
	}

	void Model::setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer)
	{
		ModelImpl *impl = (ModelImpl *)this;
		{
			if (impl->vbo)
			{
				glDeleteBuffers(1, &impl->vbo);
				impl->vbo = 0;
			}
			glCreateBuffers(1, &impl->vbo);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		impl->verticesCount = numeric_cast<uint32>(vertexData.size() / vertexSize);
		impl->indicesCount = numeric_cast<uint32>(indexData.size());
		impl->materialSize = numeric_cast<uint32>(materialBuffer.size());
		CAGE_ASSERT(vertexData.size() == vertexSize * impl->verticesCount);

		const GLint bufferAlignment = UniformBuffer::alignmentRequirement();

		const uint32 bufferSize = numeric_cast<uint32>(impl->verticesCount * vertexSize + detail::addToAlign(impl->verticesCount * vertexSize, bufferAlignment) + impl->indicesCount * sizeof(uint32) + detail::addToAlign(impl->indicesCount * sizeof(uint32), bufferAlignment) + impl->materialSize + detail::addToAlign(materialBuffer.size(), bufferAlignment));
		glNamedBufferData(impl->vbo, bufferSize, nullptr, GL_STATIC_DRAW);
		CAGE_CHECK_GL_ERROR_DEBUG();

		uint32 offset = 0;

		{ // vertices
			glNamedBufferSubData(impl->vbo, offset, impl->verticesCount * vertexSize, vertexData.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += impl->verticesCount * vertexSize + numeric_cast<uint32>(detail::addToAlign(impl->verticesCount * vertexSize, bufferAlignment));
		}

		if (impl->indicesCount)
		{ // indices
			impl->indicesOffset = offset;
			glNamedBufferSubData(impl->vbo, offset, impl->indicesCount * sizeof(uint32), indexData.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += impl->indicesCount * sizeof(uint32) + numeric_cast<uint32>(detail::addToAlign(impl->indicesCount * sizeof(uint32), bufferAlignment));
			glVertexArrayElementBuffer(impl->id, impl->vbo);
		}
		else
			glVertexArrayElementBuffer(impl->id, 0);

		if (impl->materialSize)
		{ // material
			impl->materialOffset = offset;
			glNamedBufferSubData(impl->vbo, offset, materialBuffer.size(), materialBuffer.data());
			CAGE_CHECK_GL_ERROR_DEBUG();
			offset += numeric_cast<uint32>(materialBuffer.size() + detail::addToAlign(materialBuffer.size(), bufferAlignment));
		}

		impl->updatePrimitivesCount();
	}

	void Model::setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset)
	{
		ModelImpl *impl = (ModelImpl *)this;
		if (type == 0)
			glDisableVertexArrayAttrib(impl->id, index);
		else
		{
			glEnableVertexArrayAttrib(impl->id, index);
			glVertexArrayVertexBuffer(impl->id, index, impl->vbo, startOffset, stride);
			glVertexArrayAttribBinding(impl->id, index, index);
			switch (type)
			{
				case GL_BYTE:
				case GL_UNSIGNED_BYTE:
				case GL_SHORT:
				case GL_UNSIGNED_SHORT:
				case GL_INT:
				case GL_UNSIGNED_INT:
					glVertexArrayAttribIFormat(impl->id, index, size, type, 0);
					break;
				case GL_DOUBLE:
					glVertexArrayAttribLFormat(impl->id, index, size, type, 0);
					break;
				default:
					glVertexArrayAttribFormat(impl->id, index, size, type, GL_FALSE, 0);
			}
		}
	}

	uint32 Model::verticesCount() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		return impl->verticesCount;
	}

	uint32 Model::indicesCount() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		return impl->indicesCount;
	}

	uint32 Model::primitivesCount() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		return impl->primitivesCount;
	}

	Aabb Model::boundingBox() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
		return impl->box;
	}

	void Model::dispatch() const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
#ifdef CAGE_ASSERT_ENABLED
		CAGE_ASSERT(boundModel == impl->id);
#endif
		if (impl->indicesCount)
			glDrawElements(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void *)(uintPtr)impl->indicesOffset);
		else
			glDrawArrays(impl->primitiveType, 0, impl->verticesCount);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	void Model::dispatch(uint32 instances) const
	{
		const ModelImpl *impl = (const ModelImpl *)this;
#ifdef CAGE_ASSERT_ENABLED
		CAGE_ASSERT(boundModel == impl->id);
#endif
		if (impl->indicesCount)
			glDrawElementsInstanced(impl->primitiveType, impl->indicesCount, GL_UNSIGNED_INT, (void *)(uintPtr)impl->indicesOffset, instances);
		else
			glDrawArraysInstanced(impl->primitiveType, 0, impl->verticesCount, instances);
		CAGE_CHECK_GL_ERROR_DEBUG();
	}

	Holder<Model> newModel()
	{
		return systemMemory().createImpl<Model, ModelImpl>();
	}
}
