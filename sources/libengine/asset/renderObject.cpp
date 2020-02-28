#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context, void *schemePointer)
		{
			Holder<RenderObject> obj = newRenderObject();
			obj->setDebugName(context->textName);

			Deserializer des(context->originalData());
			RenderObjectHeader h;
			des >> h;
			obj->color = h.color;
			obj->intensity = h.intensity;
			obj->opacity = h.opacity;
			obj->texAnimSpeed = h.texAnimSpeed;
			obj->texAnimOffset = h.texAnimOffset;
			obj->skelAnimName = h.skelAnimName;
			obj->skelAnimSpeed = h.skelAnimSpeed;
			obj->skelAnimOffset = h.skelAnimOffset;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;

			float *thresholds = (float*)des.advance(h.lodsCount * sizeof(uint32));
			uint32 *indices = (uint32*)des.advance((h.lodsCount + 1) * sizeof(uint32));
			uint32 *names = (uint32*)des.advance(h.meshesCount * sizeof(uint32));
			obj->setLods(h.lodsCount, h.meshesCount, thresholds, indices, names);

			context->assetHolder = templates::move(obj).cast<void>();
		}
	}

	AssetScheme genAssetSchemeRenderObject()
	{
		AssetScheme s;
		s.load.bind<&processLoad>();
		return s;
	}
}
