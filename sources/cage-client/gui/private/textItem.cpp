#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/textPack.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include <cage-client/assets.h>
#include "../private.h"

#include <unordered_map>

namespace cage
{
	void textCreate(guiItemStruct *item)
	{
		item->text = item->impl->itemsMemory.createObject<textItemStruct>(item);
	}

	textItemStruct::textItemStruct(guiItemStruct *base) : base(base)
	{}

	namespace
	{
		const string loadInternationalizedText(guiImpl *impl, uint32 asset, uint32 text, string params)
		{
			auto a = impl->assetManager->tryGet<assetSchemeIndexTextPackage, textPackClass>(asset);
			if (a)
			{
				std::vector<string> ps;
				while (!params.empty())
					ps.push_back(params.split("|"));
				return a->format(text, numeric_cast<uint32>(ps.size()), ps.data());
			}
			else
				return "";
		}
	}

	void textItemStruct::initialize()
	{
		auto *impl = base->impl;
		string value;
		{ // load text
			GUI_GET_COMPONENT(text, t, base->entity);
			if (t.assetName && t.textName)
				value = loadInternationalizedText(impl, t.assetName, t.textName, t.value);
			else
				value = t.value;
		}
		{ // load format
			if (GUI_HAS_COMPONENT(textFormat, base->entity))
			{
				GUI_GET_COMPONENT(textFormat, f, base->entity);
				if (f.align != (textAlignEnum)-1)
					text.format.align = f.align;
				if (f.color.valid())
					text.color = f.color;
				if (f.lineSpacing != detail::numeric_limits<sint16>::min())
					text.format.lineSpacing = f.lineSpacing;
				if (f.fontName)
					text.font = impl->assetManager->tryGet<assetSchemeIndexFont, fontClass>(f.fontName);
			}
		}
		if (text.font)
		{
			text.font->transcript(value, nullptr, text.count);
			text.glyphs = (uint32*)impl->itemsMemory.allocate(text.count * sizeof(uint32));
			text.font->transcript(value, text.glyphs, text.count);
		}
	}

	void textItemStruct::updateRequestedSize()
	{
		if (text.font)
		{
			uint32 w, h;
			text.font->size(text.glyphs, text.count, text.format, w, h);
			base->requestedSize = vec2(w, h) / base->impl->pointsScale;
		}
		else
			base->requestedSize = vec2();
	}

	renderableTextStruct *textItemStruct::emit() const
	{
		if (!base->text->text.font || !base->text->text.count)
			return nullptr; // early exit
		CAGE_ASSERT_RUNTIME(base->text->text.color.valid());
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableTextStruct>();
		t->data = base->text->text;
		t->data.glyphs = (uint32*)e->memory.allocate(t->data.count * sizeof(uint32));
		detail::memcpy(t->data.glyphs, base->text->text.glyphs, t->data.count * sizeof(uint32));
		t->data.posX = numeric_cast<sint16>(base->contentPosition[0] * base->impl->pointsScale);
		t->data.posY = numeric_cast<sint16>(base->contentPosition[1] * base->impl->pointsScale);
		t->data.format.wrapWidth = numeric_cast<uint16>(base->contentSize[0] * base->impl->pointsScale);
		e->last->next = t;
		e->last = t;
		return t;
	}
}
