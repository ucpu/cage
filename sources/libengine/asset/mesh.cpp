#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include <cage-core/color.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/graphics/shaderConventions.h>

namespace cage
{
	namespace
	{
		constexpr MeshDataFlags auxFlag(uint32 i)
		{
			return i == 0 ? MeshDataFlags::Aux0 : i == 1 ? MeshDataFlags::Aux1 : i == 2 ? MeshDataFlags::Aux2 : i == 3 ? MeshDataFlags::Aux3 : MeshDataFlags::None;
		}

		void processLoad(const AssetContext *context, void *schemePointer)
		{
			Mesh *msh = nullptr;
			if (context->assetHolder)
			{
				msh = static_cast<Mesh*>(context->assetHolder.get());
				msh->bind();
			}
			else
			{
				context->assetHolder = newRenderMesh().cast<void>();
				msh = static_cast<Mesh*>(context->assetHolder.get());
				msh->setDebugName(context->textName);
			}
			context->returnData = msh;

			Deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			MeshHeader data;
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
			CAGE_ASSERT(des.available() == 0);

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
				MeshDataFlags f = auxFlag(i);
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

	AssetScheme genAssetSchemeRenderMesh(uint32 threadIndex, Window *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}

	bool MeshHeader::normals() const
	{
		return (flags & MeshDataFlags::Normals) == MeshDataFlags::Normals;
	}

	bool MeshHeader::tangents() const
	{
		return (flags & MeshDataFlags::Tangents) == MeshDataFlags::Tangents;
	}

	bool MeshHeader::bones() const
	{
		return (flags & MeshDataFlags::Bones) == MeshDataFlags::Bones;
	}

	bool MeshHeader::uvs() const
	{
		return (flags & MeshDataFlags::Uvs) == MeshDataFlags::Uvs;
	}

	uint32 MeshHeader::vertexSize() const
	{
		uint32 p = sizeof(float) * 3;
		uint32 u = sizeof(float) * (int)uvs() * uvDimension;
		uint32 n = sizeof(float) * (int)normals() * 3;
		uint32 t = sizeof(float) * (int)tangents() * 6;
		uint32 b = (int)bones() * (sizeof(uint16) + sizeof(float)) * 4;
		uint32 a = 0;
		for (uint32 i = 0; i < 4; i++)
		{
			MeshDataFlags f = auxFlag(i);
			a += sizeof(float) * (int)((flags & f) == f) * auxDimensions[i];
		}
		return p + u + n + t + b + a;
	}
}
