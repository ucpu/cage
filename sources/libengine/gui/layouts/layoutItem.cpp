#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	LayoutItem::LayoutItem(HierarchyItem *hierarchy) : BaseItem(hierarchy)
	{}

	bool LayoutItem::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool LayoutItem::keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyChar(uint32 key)
	{
		return false;
	}

	void LayoutItem::emit() const
	{
		hierarchy->childrenEmit();
	}

	void LayoutItem::generateEventReceivers()
	{
		// do nothing
	}
}
