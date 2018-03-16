#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

#include <unordered_map>

namespace cage
{
	void imageCreate(guiItemStruct *item)
	{
		item->image = item->impl->itemsMemory.createObject<imageItemStruct>(item);
	}

	imageItemStruct::imageItemStruct(guiItemStruct *base) : base(base), skipInitialize(false)
	{}

	void imageItemStruct::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = base->impl;
		if (GUI_HAS_COMPONENT(imageFormat, base->entity))
		{
			GUI_GET_COMPONENT(imageFormat, f, base->entity);
			image.apply(f, impl);
		}
		assign();
	}

	void imageItemStruct::assign()
	{
		auto *impl = base->impl;
		GUI_GET_COMPONENT(image, i, base->entity);
		assign(i);
	}

	void imageItemStruct::assign(const imageComponent &value)
	{
		image.texture = base->impl->assetManager->tryGet<assetSchemeIndexTexture, textureClass>(value.textureName);
		image.uvClip = vec4(value.textureUvOffset, value.textureUvOffset + value.textureUvSize);
		image.aniTexFrames = detail::evalSamplesForTextureAnimation(image.texture, 0, 0, 0, 0); // todo
	}

	vec2 imageItemStruct::updateRequestedSize()
	{
		if (image.texture)
		{
			uint32 w, h;
			image.texture->getResolution(w, h);
			return vec2(w, h);
		}
		return vec2();
	}

	renderableImageStruct *imageItemStruct::emit() const
	{
		return emit(base->contentPosition, base->contentSize);
	}

	renderableImageStruct *imageItemStruct::emit(vec2 position, vec2 size) const
	{
		if (!image.texture)
			return nullptr;
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableImageStruct>();
		t->data = image;
		t->data.ndcPos = base->impl->pointsToNdc(position, size);
		e->last->next = t;
		e->last = t;
		return t;
	}
}
