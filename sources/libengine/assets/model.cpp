#include <cage-core/assetContext.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/mesh.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/model.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<Model> model = newModel();

			Deserializer des(context->originalData);
			ModelHeader data;
			des >> data;

			MemoryBuffer mat;
			mat.resize(data.materialSize);
			des.read(mat);

			Holder<Mesh> mesh = newMesh();
			mesh->importBuffer(des.read(des.available()));

			model->importMesh(+mesh, mat);
			model->setBoundingBox(data.box);

			model->importTransform = data.importTransform;
			for (int i = 0; i < MaxTexturesCountPerMaterial; i++)
				model->textureNames[i] = data.textureNames[i];
			model->shaderName = data.shaderName;
			model->flags = data.renderFlags;
			model->layer = data.renderLayer;
			model->bones = data.skeletonBones;

			model->setDebugName(context->textId); // last command to apply it to all subresources

			context->assetHolder = std::move(model).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeModel(uint32 threadIndex)
	{
		AssetsScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Model>();
		return s;
	}
}
