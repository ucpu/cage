#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/textPack.h>

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
	void textCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->text);
		item->text = item->impl->itemsMemory.createObject<TextItem>(item);
	}

	TextItem::TextItem(HierarchyItem *hierarchy) : hierarchy(hierarchy), skipInitialize(false)
	{}

	// this is also used in engine
	string loadInternationalizedText(AssetManager *assets, uint32 asset, uint32 text, string params)
	{
		if (asset == 0 || text == 0)
			return params;
		auto a = assets->tryGet<AssetSchemeIndexTextPack, TextPack>(asset);
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

	void TextItem::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = hierarchy->impl;
		if (GUI_HAS_COMPONENT(TextFormat, hierarchy->ent))
		{
			CAGE_COMPONENT_GUI(TextFormat, f, hierarchy->ent);
			text.apply(f, impl);
		}
		transcript();
	}

	void TextItem::transcript()
	{
		auto *impl = hierarchy->impl;
		string value;
		{
			CAGE_COMPONENT_GUI(Text, t, hierarchy->ent);
			value = loadInternationalizedText(impl->assetMgr, t.assetName, t.textName, t.value);
		}
		transcript(value);
	}

	void TextItem::transcript(const string &value)
	{
		transcript(value.c_str());
	}

	void TextItem::transcript(const char *value)
	{
		if (text.font)
		{
			text.font->transcript(value, nullptr, text.count);
			text.glyphs = (uint32*)hierarchy->impl->itemsMemory.allocate(text.count * sizeof(uint32), sizeof(uintPtr));
			text.font->transcript(value, text.glyphs, text.count);
		}
	}

	vec2 TextItem::findRequestedSize()
	{
		if (text.font)
		{
			vec2 size;
			text.font->size(text.glyphs, text.count, text.format, size);
			return size;
		}
		return vec2();
	}

	RenderableText *TextItem::emit(vec2 position, vec2 size) const
	{
		if (!text.font)
			return nullptr;
		if ((text.count == 0 || size[0] <= 0) && text.cursor > 0)
			return nullptr; // early exit
		CAGE_ASSERT(text.color.valid());
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<RenderableText>();
		t->setClip(hierarchy);
		t->data = text;
		t->data.glyphs = (uint32*)e->memory.allocate(t->data.count * sizeof(uint32), sizeof(uintPtr));
		detail::memcpy(t->data.glyphs, text.glyphs, t->data.count * sizeof(uint32));
		t->data.format.size *= hierarchy->impl->pointsScale;
		auto orr = hierarchy->impl->outputResolution;
		position *= hierarchy->impl->pointsScale;
		t->data.transform = transpose(mat4(
			2.0 / orr[0], 0, 0, 2.0 * position[0] / orr[0] - 1.0,
			0, 2.0 / orr[1], 0, 1.0 - 2.0 * position[1] / orr[1],
			0, 0, 1, 0,
			0, 0, 0, 1
		));
		t->data.format.wrapWidth = size[0] * hierarchy->impl->pointsScale;
		e->last->next = t;
		e->last = t;
		return t;
	}

	void TextItem::updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor)
	{
		if (!text.font)
			return;
		vec2 dummy;
		Font::FormatStruct f(text.format);
		f.wrapWidth = size[0];
		text.font->size(text.glyphs, text.count, f, dummy, point - position, cursor);
		text.cursor = cursor;
	}
}
