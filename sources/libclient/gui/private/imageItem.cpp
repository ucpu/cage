#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

#include <unordered_map>

namespace cage
{
	void imageCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->image);
		item->image = item->impl->itemsMemory.createObject<imageItemStruct>(item);
	}

	imageItemStruct::imageItemStruct(hierarchyItemStruct *hierarchy) : hierarchy(hierarchy), skipInitialize(false)
	{}

	void imageItemStruct::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = hierarchy->impl;
		if (GUI_HAS_COMPONENT(imageFormat, hierarchy->entity))
		{
			GUI_GET_COMPONENT(imageFormat, f, hierarchy->entity);
			apply(f);
		}
		assign();
	}

	void imageItemStruct::assign()
	{
		auto *impl = hierarchy->impl;
		GUI_GET_COMPONENT(image, i, hierarchy->entity);
		assign(i);
	}

	void imageItemStruct::assign(const imageComponent &value)
	{
		image = value;
		texture = hierarchy->impl->assetManager->tryGet<assetSchemeIndexTexture, textureClass>(value.textureName);
	}

	void imageItemStruct::apply(const imageFormatComponent &f)
	{
		format = f;
		// todo inherit only
	}

	vec2 imageItemStruct::findRequestedSize()
	{
		if (texture)
		{
			uint32 w, h;
			texture->getResolution(w, h);
			return vec2(w, h);
		}
		return vec2();
	}

	renderableImageStruct *imageItemStruct::emit(vec2 position, vec2 size) const
	{
		if (!texture)
			return nullptr;
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<renderableImageStruct>();
		t->setClip(hierarchy);
		t->data.texture = texture;
		t->data.ndcPos = hierarchy->impl->pointsToNdc(position, size);
		t->data.uvClip = vec4(image.textureUvOffset, image.textureUvOffset + image.textureUvSize);
		// todo format mode
		t->data.aniTexFrames = detail::evalSamplesForTextureAnimation(texture, getApplicationTime(), image.animationStart, format.animationSpeed, format.animationOffset);
		e->last->next = t;
		e->last = t;
		return t;
	}
}
