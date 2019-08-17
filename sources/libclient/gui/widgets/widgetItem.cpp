#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	baseItemStruct::baseItemStruct(hierarchyItemStruct *hierarchy) : hierarchy(hierarchy)
	{}

	widgetItemStruct::widgetItemStruct(hierarchyItemStruct *hierarchy) : baseItemStruct(hierarchy), skin(nullptr)
	{}

	uint32 widgetItemStruct::mode(bool hover, uint32 focusParts) const
	{
		if (widgetState.disabled)
			return 3;
		if (hierarchy->impl->hover == this && hover)
			return 2;
		if (hasFocus(focusParts))
			return 1;
		return 0;
	}

	uint32 widgetItemStruct::mode(const vec2 &pos, const vec2 &size, uint32 focusParts) const
	{
		return mode(pointInside(pos, size, hierarchy->impl->outputMouse), focusParts);
	}

	bool widgetItemStruct::hasFocus(uint32 parts) const
	{
		CAGE_ASSERT(hierarchy->ent);
		return hierarchy->impl->focusName && hierarchy->impl->focusName == hierarchy->ent->name() && (hierarchy->impl->focusParts & parts) > 0;
	}

	void widgetItemStruct::makeFocused(uint32 parts)
	{
		CAGE_ASSERT(parts != 0);
		CAGE_ASSERT(hierarchy->ent);
		CAGE_ASSERT(!widgetState.disabled);
		hierarchy->impl->focusName = hierarchy->ent->name();
		hierarchy->impl->focusParts = parts;
	}

	void widgetItemStruct::findFinalPosition(const finalPositionStruct &update)
	{
		// do nothing
	}

	void widgetItemStruct::generateEventReceivers()
	{
		eventReceiverStruct e;
		e.widget = this;
		e.pos = hierarchy->renderPos;
		e.size = hierarchy->renderSize;
		if (clip(e.pos, e.size, hierarchy->clipPos, hierarchy->clipSize))
			hierarchy->impl->mouseEventReceivers.push_back(e);
	}

	renderableElementStruct *widgetItemStruct::emitElement(elementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const
	{
		CAGE_ASSERT(element < elementTypeEnum::TotalElements);
		CAGE_ASSERT(mode < 4);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<renderableElementStruct>();
		t->setClip(hierarchy);
		t->data.element = (uint32)element;
		t->data.mode = mode;
		t->data.outer = hierarchy->impl->pointsToNdc(pos, size);
		vec4 border = skin->layouts[(uint32)element].border;
		offset(pos, size, -border);
		t->data.inner = hierarchy->impl->pointsToNdc(pos, size);
		t->skinBuffer = skin->elementsGpuBuffer.get();
		t->skinTexture = skin->texture;
		e->last->next = t;
		e->last = t;
		return t;
	}

	bool widgetItemStruct::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		makeFocused();
		return true;
	}

	bool widgetItemStruct::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetItemStruct::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetItemStruct::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetItemStruct::mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetItemStruct::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetItemStruct::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetItemStruct::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetItemStruct::keyChar(uint32 key)
	{
		return true;
	}
}
