#include "../private.h"

namespace cage
{
	GuiLayoutLineComponent::GuiLayoutLineComponent() : vertical(false)
	{}

	GuiLayoutTableComponent::GuiLayoutTableComponent() : sections(2), grid(false), vertical(true)
	{}

	GuiLayoutSplitterComponent::GuiLayoutSplitterComponent() : vertical(false), inverse(false)
	{}
}
