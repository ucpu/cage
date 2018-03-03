#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/textPack.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

#include <unordered_map>

namespace cage
{
	void textCreate(guiItemStruct *item)
	{
		item->text = item->impl->itemsMemory.createObject<textItemStruct>(item);
	}

	textItemStruct::textItemStruct(guiItemStruct *base) : base(base), skipInitialize(false)
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
		if (skipInitialize)
			return;
		auto *impl = base->impl;
		if (GUI_HAS_COMPONENT(textFormat, base->entity))
		{
			GUI_GET_COMPONENT(textFormat, f, base->entity);
			text.apply(f, base->impl);
		}
		transcript();
	}

	void textItemStruct::transcript()
	{
		auto *impl = base->impl;
		string value;
		{
			GUI_GET_COMPONENT(text, t, base->entity);
			if (t.assetName && t.textName)
				value = loadInternationalizedText(impl, t.assetName, t.textName, t.value);
			else
				value = t.value;
			text.count = value.length();
		}
		transcript(value);
	}

	void textItemStruct::transcript(const string &value)
	{
		transcript(value.c_str());
	}

	void textItemStruct::transcript(const char *value)
	{
		if (text.font)
		{
			text.font->transcript(value, nullptr, text.count);
			text.glyphs = (uint32*)base->impl->itemsMemory.allocate(text.count * sizeof(uint32));
			text.font->transcript(value, text.glyphs, text.count);
		}
	}

	void textItemStruct::updateRequestedSize(vec2 &size)
	{
		if (text.font)
		{
			uint32 w, h;
			text.font->size(text.glyphs, text.count, text.format, w, h);
			size = vec2(w, h) / base->impl->pointsScale;
		}
		else
			size = vec2();
	}

	renderableTextStruct *textItemStruct::emit() const
	{
		return emit(base->contentPosition, base->contentSize);
	}

	renderableTextStruct *textItemStruct::emit(vec2 position, vec2 size) const
	{
		if (!text.font)
			return nullptr;
		if (text.count == 0 && text.cursor > 0)
			return nullptr; // early exit
		CAGE_ASSERT_RUNTIME(text.color.valid());
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableTextStruct>();
		t->data = text;
		t->data.glyphs = (uint32*)e->memory.allocate(t->data.count * sizeof(uint32));
		detail::memcpy(t->data.glyphs, text.glyphs, t->data.count * sizeof(uint32));
		t->data.posX = numeric_cast<sint16>(position[0] * base->impl->pointsScale);
		t->data.posY = numeric_cast<sint16>(position[1] * base->impl->pointsScale);
		t->data.format.wrapWidth = numeric_cast<uint16>(size[0] * base->impl->pointsScale + 1);
		e->last->next = t;
		e->last = t;
		return t;
	}

	void textItemStruct::updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor)
	{
		if (!text.font)
			return;
		// todo assert that size == text.format.size
		uint32 dummy;
		text.font->size(text.glyphs, text.count, text.format, dummy, dummy, point - position, cursor);
		text.cursor = cursor;
	}
}
