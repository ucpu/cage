#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/mesh.h>
#include <cage-core/typeIndex.h>

#include <cage-engine/assetStructs.h>
#include <cage-engine/model.h>

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

			msh->importMesh(+poly, mat);

			msh->setBoundingBox(data.box);
			msh->setTextureNames(data.textureNames);

			msh->flags = data.renderFlags;
			msh->skeletonName = data.skeletonName;
			msh->skeletonBones = data.skeletonBones;
			msh->instancesLimitHint = data.instancesLimitHint;

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
