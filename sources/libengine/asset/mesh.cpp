#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/polyhedron.h>
#include <cage-core/color.h>

#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/shaderConventions.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<Mesh> msh = newMesh();
			msh->setDebugName(context->textName);

			Deserializer des(context->originalData);
			MeshHeader data;
			des >> data;

			MemoryBuffer mat;
			mat.resize(data.materialSize);
			des.read(mat);

			Holder<Polyhedron> poly = newPolyhedron();
			poly->deserialize(des.advance(des.available()));

			msh->importPolyhedron(poly.get(), mat);

			msh->setFlags(data.renderFlags);
			msh->setBoundingBox(data.box);
			msh->setTextureNames(data.textureNames);
			msh->setSkeleton(data.skeletonName, data.skeletonBones);
			msh->setInstancesLimitHint(data.instancesLimitHint);

			context->assetHolder = templates::move(msh).cast<void>();
		}
	}

	AssetScheme genAssetSchemeMesh(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
