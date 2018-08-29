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
	widgetBaseStruct::widgetBaseStruct(guiItemStruct *base) : base(base)
	{
		auto *impl = base->impl;
		if (GUI_HAS_COMPONENT(widgetState, base->entity))
		{
			GUI_GET_COMPONENT(widgetState, ws, base->entity);
			widgetState = ws;
		}
	}

	const skinDataStruct &widgetBaseStruct::skin() const
	{
		CAGE_ASSERT_RUNTIME(widgetState.skinIndex < base->impl->skins.size(), widgetState.skinIndex, base->impl->skins.size(), base->entity ? base->entity->getName() : 0);
		return base->impl->skins[widgetState.skinIndex];
	}

	uint32 widgetBaseStruct::mode(bool hover) const
	{
		if (widgetState.disabled)
			return 3;
		if (base->impl->hover == this && hover)
			return 2;
		if (hasFocus())
			return 1;
		return 0;
	}

	uint32 widgetBaseStruct::mode(const vec2 &pos, const vec2 &size) const
	{
		return mode(pointInside(pos, size, base->impl->outputMouse));
	}

	bool widgetBaseStruct::hasFocus() const
	{
		return base->entity && base->impl->focusName && base->impl->focusName == base->entity->getName();
	}

	void widgetBaseStruct::makeFocused()
	{
		CAGE_ASSERT_RUNTIME(base->entity);
		base->impl->focusName = base->entity->getName();
	}

	void widgetBaseStruct::findFinalPosition(const finalPositionStruct &update)
	{
		// do nothing
	}

	renderableElementStruct *widgetBaseStruct::emitElement(elementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements);
		CAGE_ASSERT_RUNTIME(mode < 4);
		CAGE_ASSERT_RUNTIME(pos.valid());
		CAGE_ASSERT_RUNTIME(size.valid());
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableElementStruct>();
		t->data.element = (uint32)element;
		t->data.mode = mode;
		t->data.outer = base->impl->pointsToNdc(pos, size);
		vec4 border = skin().layouts[(uint32)element].border;
		offset(pos, size, -border);
		t->data.inner = base->impl->pointsToNdc(pos, size);
		t->skinBuffer = skin().elementsGpuBuffer.get();
		t->skinTexture = skin().texture;
		e->last->next = t;
		e->last = t;
		return t;
	}

	bool widgetBaseStruct::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		makeFocused();
		return true;
	}

	bool widgetBaseStruct::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetBaseStruct::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetBaseStruct::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetBaseStruct::mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool widgetBaseStruct::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetBaseStruct::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetBaseStruct::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return true;
	}

	bool widgetBaseStruct::keyChar(uint32 key)
	{
		return true;
	}
}
