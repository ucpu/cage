#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/textPack.h>

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
	void textCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->text);
		item->text = item->impl->itemsMemory.createObject<textItemStruct>(item);
	}

	textItemStruct::textItemStruct(hierarchyItemStruct *hierarchy) : hierarchy(hierarchy), skipInitialize(false)
	{}

	// this is also used in engine
	string loadInternationalizedText(assetManagerClass *assets, uint32 asset, uint32 text, string params)
	{
		if (asset == 0 || text == 0)
			return params;
		auto a = assets->tryGet<assetSchemeIndexTextPackage, textPackClass>(asset);
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

	void textItemStruct::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = hierarchy->impl;
		if (GUI_HAS_COMPONENT(textFormat, hierarchy->entity))
		{
			GUI_GET_COMPONENT(textFormat, f, hierarchy->entity);
			text.apply(f, impl);
		}
		transcript();
	}

	void textItemStruct::transcript()
	{
		auto *impl = hierarchy->impl;
		string value;
		{
			GUI_GET_COMPONENT(text, t, hierarchy->entity);
			value = loadInternationalizedText(impl->assetManager, t.assetName, t.textName, t.value);
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
			text.glyphs = (uint32*)hierarchy->impl->itemsMemory.allocate(text.count * sizeof(uint32), sizeof(uintPtr));
			text.font->transcript(value, text.glyphs, text.count);
		}
	}

	vec2 textItemStruct::findRequestedSize()
	{
		if (text.font)
		{
			vec2 size;
			text.font->size(text.glyphs, text.count, text.format, size);
			return size;
		}
		return vec2();
	}

	renderableTextStruct *textItemStruct::emit(vec2 position, vec2 size) const
	{
		if (!text.font)
			return nullptr;
		if ((text.count == 0 || size[0] <= 0) && text.cursor > 0)
			return nullptr; // early exit
		CAGE_ASSERT_RUNTIME(text.color.valid());
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<renderableTextStruct>();
		t->setClip(hierarchy);
		t->data = text;
		t->data.glyphs = (uint32*)e->memory.allocate(t->data.count * sizeof(uint32), sizeof(uintPtr));
		detail::memcpy(t->data.glyphs, text.glyphs, t->data.count * sizeof(uint32));
		t->data.format.size *= hierarchy->impl->pointsScale;
		auto orr = hierarchy->impl->outputResolution;
		position *= hierarchy->impl->pointsScale;
		t->data.transform = mat4(
			2.0 / orr[0], 0, 0, 2.0 * position[0] / orr[0] - 1.0,
			0, 2.0 / orr[1], 0, 1.0 - 2.0 * position[1] / orr[1],
			0, 0, 1, 0,
			0, 0, 0, 1
		).transpose();
		t->data.format.wrapWidth = size[0] * hierarchy->impl->pointsScale;
		e->last->next = t;
		e->last = t;
		return t;
	}

	void textItemStruct::updateCursorPosition(vec2 position, vec2 size, vec2 point, uint32 &cursor)
	{
		if (!text.font)
			return;
		vec2 dummy;
		fontClass::formatStruct f(text.format);
		f.wrapWidth = size[0];
		text.font->size(text.glyphs, text.count, f, dummy, point - position, cursor);
		text.cursor = cursor;
	}
}
