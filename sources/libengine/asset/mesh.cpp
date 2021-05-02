#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/mesh.h>
#include <cage-core/color.h>
#include <cage-core/typeIndex.h>

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
			Holder<Model> msh = newModel();
			msh->setDebugName(context->textName);

			Deserializer des(context->originalData);
			ModelHeader data;
			des >> data;

			MemoryBuffer mat;
			mat.resize(data.materialSize);
			des.read(mat);

			Holder<Mesh> poly = newMesh();
			poly->deserialize(des.advance(des.available()));

			msh->importMesh(poly.get(), mat);

			msh->setFlags(data.renderFlags);
			msh->setBoundingBox(data.box);
			msh->setTextureNames(data.textureNames);
			msh->setSkeleton(data.skeletonName, data.skeletonBones);
			msh->setInstancesLimitHint(data.instancesLimitHint);

			context->assetHolder = std::move(msh).cast<void>();
		}
	}

	AssetScheme genAssetSchemeModel(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeIndex = detail::typeIndex<Model>();
		return s;
	}
}
