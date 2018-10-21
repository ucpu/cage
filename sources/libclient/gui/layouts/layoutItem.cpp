#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	layoutItemStruct::layoutItemStruct(hierarchyItemStruct *hierarchy) : baseItemStruct(hierarchy)
	{}

	bool layoutItemStruct::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool layoutItemStruct::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return false;
	}

	bool layoutItemStruct::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return false;
	}

	bool layoutItemStruct::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
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
