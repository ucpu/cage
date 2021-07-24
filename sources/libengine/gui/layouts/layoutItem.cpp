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

	bool LayoutItem::keyPress(uint32 key, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRepeat(uint32 key, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRelease(uint32 key, ModifiersFlags modifiers)
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
