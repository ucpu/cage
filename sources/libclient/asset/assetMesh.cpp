#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include <cage-core/color.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include <cage-client/assetStructs.h>
#include <cage-client/graphics/shaderConventions.h>

namespace cage
{
	namespace
	{
		constexpr meshDataFlags auxFlag(uint32 i)
		{
			return i == 0 ? meshDataFlags::Aux0 : i == 1 ? meshDataFlags::Aux1 : i == 2 ? meshDataFlags::Aux2 : i == 3 ? meshDataFlags::Aux3 : meshDataFlags::None;
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
				context->assetHolder = newMesh().cast<void>();
				msh = static_cast<meshClass*>(context->assetHolder.get());
				msh->setDebugName(context->textName);
			}
			context->returnData = msh;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			meshHeaderStruct data;
			des >> data;

			msh->setFlags(data.renderFlags);
			msh->setPrimitiveType(data.primitiveType);
			msh->setBoundingBox(data.box);
			msh->setTextureNames(data.textureNames);
			msh->setSkeleton(data.skeletonName, data.skeletonBones);
			msh->setInstancesLimitHint(data.instancesLimitHint);

			const void *verticesData = des.advance(data.verticesCount * data.vertexSize());
			uint32 *indicesData = (uint32*)des.advance(data.indicesCount * sizeof(uint32));
			const void *materialData = des.advance(data.materialSize);
			CAGE_ASSERT_RUNTIME(des.available() == 0);

			msh->setBuffers(
				data.verticesCount, data.vertexSize(), verticesData,
				data.indicesCount, indicesData,
				data.materialSize, materialData
			);

			uint32 ptr = 0;
			msh->setAttribute(CAGE_SHADER_ATTRIB_IN_POSITION, 3, GL_FLOAT, 0, ptr);
			ptr += data.verticesCount * sizeof(vec3);

			// normals
			if (data.normals())
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_NORMAL, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
			}
			else
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_NORMAL, 0, 0, 0, 0);

			// tangents
			if (data.tangents())
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_TANGENT, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BITANGENT, 3, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec3);
			}
			else
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_TANGENT, 0, 0, 0, 0);
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BITANGENT, 0, 0, 0, 0);
			}

			// bones
			if (data.bones())
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BONEINDEX, 4, GL_UNSIGNED_SHORT, 0, ptr);
				ptr += data.verticesCount * sizeof(uint16) * 4;
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BONEWEIGHT, 4, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(vec4);
			}
			else
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BONEINDEX, 0, 0, 0, 0);
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_BONEWEIGHT, 0, 0, 0, 0);
			}

			// uvs
			if (data.uvs())
			{
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_UV, data.uvDimension, GL_FLOAT, 0, ptr);
				ptr += data.verticesCount * sizeof(float) * data.uvDimension;
			}
			else
				msh->setAttribute(CAGE_SHADER_ATTRIB_IN_UV, 0, 0, 0, 0);

			// aux
			for (uint32 i = 0; i < 4; i++)
			{
				meshDataFlags f = auxFlag(i);
				if ((data.flags & f) == f)
				{
					msh->setAttribute(CAGE_SHADER_ATTRIB_IN_AUX0 + i, data.auxDimensions[i], GL_FLOAT, 0, ptr);
					ptr += data.verticesCount * sizeof(float) * data.auxDimensions[i];
				}
				else
				{
					msh->setAttribute(CAGE_SHADER_ATTRIB_IN_AUX0 + i, 0, 0, 0, 0);
				}
			}
		}
	}

	assetSchemeStruct genAssetSchemeMesh(uint32 threadIndex, windowClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}

	bool meshHeaderStruct::normals() const
	{
		return (flags & meshDataFlags::Normals) == meshDataFlags::Normals;
	}

	bool meshHeaderStruct::tangents() const
	{
		return (flags & meshDataFlags::Tangents) == meshDataFlags::Tangents;
	}

	bool meshHeaderStruct::bones() const
	{
		return (flags & meshDataFlags::Bones) == meshDataFlags::Bones;
	}

	bool meshHeaderStruct::uvs() const
	{
		return (flags & meshDataFlags::Uvs) == meshDataFlags::Uvs;
	}

	uint32 meshHeaderStruct::vertexSize() const
	{
		uint32 p = sizeof(float) * 3;
		uint32 u = sizeof(float) * (int)uvs() * uvDimension;
		uint32 n = sizeof(float) * (int)normals() * 3;
		uint32 t = sizeof(float) * (int)tangents() * 6;
		uint32 b = (int)bones() * (sizeof(uint16) + sizeof(float)) * 4;
		uint32 a = 0;
		for (uint32 i = 0; i < 4; i++)
		{
			meshDataFlags f = auxFlag(i);
			a += sizeof(float) * (int)((flags & f) == f) * auxDimensions[i];
		}
		return p + u + n + t + b + a;
	}
}
