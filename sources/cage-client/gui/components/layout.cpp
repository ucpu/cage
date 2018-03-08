#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	explicitPositionComponent::explicitPositionComponent()
	{}

	layoutLineComponent::layoutLineComponent() : cellsAnchor(0, 0), vertical(false), expandToSameWidth(false), expandToSameHeight(false), addSpacingToFillArea(false)
	{}

	layoutTableComponent::layoutTableComponent() : sections(2), grid(false)
	{
		vertical = true;
		expandToSameWidth = true;
		expandToSameHeight = true;
		cellsAnchor = vec2(0.5, 0.5);
	}

	layoutRadialComponent::layoutRadialComponent()
	{}

	layoutSplitterComponent::layoutSplitterComponent() : vertical(false), inverse(false)
	{}

	layoutsComponentsStruct::layoutsComponentsStruct(entityManagerClass *ents)
	{
		detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
	}
}