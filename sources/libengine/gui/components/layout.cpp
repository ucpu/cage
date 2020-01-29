#include "../private.h"

namespace cage
{
	GuiLayoutLineComponent::GuiLayoutLineComponent() : vertical(false)
	{}

	GuiLayoutTableComponent::GuiLayoutTableComponent() : sections(2), grid(false), vertical(true)
	{}

	GuiLayoutSplitterComponent::GuiLayoutSplitterComponent() : vertical(false), inverse(false)
	{}

	namespace privat
	{
		GuiLayoutsComponents::GuiLayoutsComponents(EntityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(CAGE_JOIN(Gui, CAGE_JOIN(T, Component))(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
