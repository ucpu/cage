#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	layoutLineComponent::layoutLineComponent() : vertical(false)
	{}

	layoutTableComponent::layoutTableComponent() : sections(2), grid(false), vertical(true)
	{}

	layoutSplitterComponent::layoutSplitterComponent() : vertical(false), inverse(false)
	{}

	namespace privat
	{
		guiLayoutsComponents::guiLayoutsComponents(entityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
