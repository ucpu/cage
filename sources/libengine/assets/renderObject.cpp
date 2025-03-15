#include <cage-core/assetContext.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/renderObject.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<RenderObject> obj = newRenderObject();
			obj->setDebugName(context->textId);

			Deserializer des(context->originalData);
			RenderObjectHeader h;
			des >> h;
			obj->color = h.color;
			obj->intensity = h.intensity;
			obj->opacity = h.opacity;
			obj->animSpeed = h.animSpeed;
			obj->animOffset = h.animOffset;
			obj->skelAnimId = h.skelAnimId;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;

			PointerRange<const Real> thresholds = bufferCast<const Real>(des.read(h.lodsCount * sizeof(Real)));
			PointerRange<const uint32> indices = bufferCast<const uint32>(des.read((h.lodsCount + 1) * sizeof(uint32)));
			PointerRange<const uint32> names = bufferCast<const uint32>(des.read(h.modelsCount * sizeof(uint32)));
			obj->setLods(thresholds, indices, names);

			context->assetHolder = std::move(obj).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeRenderObject()
	{
		AssetsScheme s;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<RenderObject>();
		return s;
	}
}
