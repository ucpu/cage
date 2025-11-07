#include <cage-core/assetContext.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/mesh.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/assetsStructs.h>
#include <cage-engine/graphicsBindings.h>
#include <cage-engine/model.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData);
			ModelHeader header;
			des >> header;

			// todo handle mesh name
			CAGE_ASSERT(header.meshName == 0); // do not use for now

			Holder<Mesh> mesh = newMesh();
			mesh->importBuffer(des.read(header.meshSize));

			MemoryBuffer mat;
			mat.resize(header.materialSize);
			des.read(mat);

			Holder<Model> model = newModel((GraphicsDevice *)context->device, +mesh, mat, context->textId);

			if (header.colliderSize)
			{
				Holder<Collider> col = newCollider();
				col->importBuffer(des.read(header.colliderSize));
				col->rebuild();
				model->collider = std::move(col);
			}

			CAGE_ASSERT(des.available() == 0);

			model->importTransform = header.importTransform;
			model->boundingBox = header.box;
			model->textureNames = header.textureNames;
			model->shaderName = header.shaderName;
			model->flags = header.renderFlags;
			model->layer = header.renderLayer;
			model->bonesCount = header.skeletonBones;

			//prepareModelBindings((GraphicsDevice *)context->device, context->assetsManager, +model);

			context->assetHolder = std::move(model).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeModel(GraphicsDevice *device)
	{
		AssetsScheme s;
		s.device = device;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Model>();
		return s;
	}
}
