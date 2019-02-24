#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>
#include <cage-core/color.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include <cage-client/assetStructs.h>

namespace cage
{
	namespace
	{
		void processDecompress(const assetContextStruct *context, void *schemePointer)
		{
			meshHeaderStruct *hdr = (meshHeaderStruct*)context->compressedData;
			uintPtr sth = sizeof(meshHeaderStruct) + hdr->materialSize;
			detail::memcpy(context->originalData, context->compressedData, sth);
			uintPtr res = detail::decompress((char*)context->compressedData + sth, numeric_cast<uintPtr>(context->compressedSize - sth), (char*)context->originalData + sth, numeric_cast<uintPtr>(context->originalSize - sth));
			CAGE_ASSERT_RUNTIME(res == context->originalSize - sth, res, context->originalSize, sth);
		}

		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			meshClass *msh = nullptr;
			if (context->assetHolder)
			{
				msh = static_cast<meshClass*>(context->assetHolder.get());
				msh->bind();
			}
			else
			{
				context->assetHolder = newMesh().transfev();
				msh = static_cast<meshClass*>(context->assetHolder.get());
				msh->setDebugName(context->textName);
			}
			context->returnData = msh;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			meshHeaderStruct data;
			des >> data;
			const void *materialData = des.access(data.materialSize);
			const void *verticesData = des.access(data.verticesCount * data.vertexSize());
			uint32 *indicesData = (uint32*)des.access(data.indicesCount * sizeof(uint32));

			msh->setFlags(data.flags);
			msh->setPrimitiveType(data.primitiveType);
			msh->setBoundingBox(data.box);
			msh->setTextures(data.textureNames);
			msh->setSkeleton(data.skeletonName, data.skeletonBones);
			msh->setInstancesLimitHint(data.instancesLimitHint);

			msh->setBuffers(
				data.verticesCount, data.vertexSize(), verticesData,
				data.indicesCount, indicesData,
				data.materialSize, materialData
			);

			uint32 ptr = 0;
			msh->setAttribute(0, 3, GL_FLOAT, 0, ptr);
			ptr += data.verticesCount * sizeof(vec3);

			if (data.uvs())
			{
				msh->setAttribute(1, 2, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec2);
			}
			else
				msh->setAttribute(1, 0, 0, 0, 0);

			if (data.normals())
			{
				msh->setAttribute(2, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
			}
			else
				msh->setAttribute(2, 0, 0, 0, 0);

			if (data.tangents())
			{
				msh->setAttribute(3, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
				msh->setAttribute(4, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
			}
			else
			{
				msh->setAttribute(3, 0, 0, 0, 0);
				msh->setAttribute(4, 0, 0, 0, 0);
			}

			if (data.bones())
			{
				msh->setAttribute(5, 4, GL_UNSIGNED_SHORT, 0, ptr);
				ptr += data.verticesCount * sizeof(uint16) * 4;
				msh->setAttribute(6, 4, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec4);
			}
			else
			{
				msh->setAttribute(5, 0, 0, 0, 0);
				msh->setAttribute(6, 0, 0, 0, 0);
			}
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeMesh(uint32 threadIndex, windowClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		s.decompress.bind<&processDecompress>();
		return s;
	}

	bool meshHeaderStruct::uvs() const
	{
		return (flags & meshFlags::Uvs) == meshFlags::Uvs;
	}

	bool meshHeaderStruct::normals() const
	{
		return (flags & meshFlags::Normals) == meshFlags::Normals;
	}

	bool meshHeaderStruct::tangents() const
	{
		return (flags & meshFlags::Tangents) == meshFlags::Tangents;
	}

	bool meshHeaderStruct::bones() const
	{
		return (flags & meshFlags::Bones) == meshFlags::Bones;
	}

	uint32 meshHeaderStruct::vertexSize() const
	{
		return sizeof(float) * (3 + (int)uvs() * 2 + (int)normals() * 3 + (int)tangents() * 6) + (int)bones() * (sizeof(uint16) + sizeof(float)) * 4;
	}
}
