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
	layoutItemStruct::layoutItemStruct(hierarchyItemStruct *hierarchy) : baseItemStruct(hierarchy)
	{}

	bool layoutItemStruct::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool layoutItemStruct::keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool layoutItemStruct::keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return false;
	}

	bool layoutItemStruct::keyChar(uint32 key)
	{
		return false;
	}

	void layoutItemStruct::emit() const
	{
		hierarchy->childrenEmit();
	}

	void layoutItemStruct::generateEventReceivers()
	{
		// do nothing
	}
}
