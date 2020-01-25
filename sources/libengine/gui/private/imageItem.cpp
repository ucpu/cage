#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

#include <unordered_map>

namespace cage
{
	void imageCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->Image);
		item->Image = item->impl->itemsMemory.createObject<ImageItem>(item);
	}

	ImageItem::ImageItem(HierarchyItem *hierarchy) : hierarchy(hierarchy), skipInitialize(false)
	{}

	void ImageItem::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = hierarchy->impl;
		if (GUI_HAS_COMPONENT(ImageFormat, hierarchy->ent))
		{
			CAGE_COMPONENT_GUI(ImageFormat, f, hierarchy->ent);
			apply(f);
		}
		assign();
	}

	void ImageItem::assign()
	{
		auto *impl = hierarchy->impl;
		CAGE_COMPONENT_GUI(Image, i, hierarchy->ent);
		assign(i);
	}

	void ImageItem::assign(const GuiImageComponent &value)
	{
		Image = value;
		texture = hierarchy->impl->assetMgr->tryGetRaw<AssetSchemeIndexTexture, Texture>(value.textureName);
	}

	void ImageItem::apply(const GuiImageFormatComponent &f)
	{
		format = f;
		// todo inherit only
	}

	vec2 ImageItem::findRequestedSize()
	{
		if (texture)
		{
			uint32 w, h;
			texture->getResolution(w, h);
			return vec2(w, h);
		}
		return vec2();
	}

	RenderableImage *ImageItem::emit(vec2 position, vec2 size) const
	{
		if (!texture)
			return nullptr;
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<RenderableImage>();
		t->setClip(hierarchy);
		t->data.texture = texture;
		t->data.ndcPos = hierarchy->impl->pointsToNdc(position, size);
		t->data.uvClip = vec4(Image.textureUvOffset, Image.textureUvOffset + Image.textureUvSize);
		// todo format mode
		t->data.aniTexFrames = detail::evalSamplesForTextureAnimation(texture, getApplicationTime(), Image.animationStart, format.animationSpeed, format.animationOffset);
		e->last->next = t;
		e->last = t;
		return t;
	}
}
