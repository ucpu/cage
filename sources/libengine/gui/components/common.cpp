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
	GuiParentComponent::GuiParentComponent() : parent(0), order(0)
	{}

	GuiImageComponent::GuiImageComponent() : animationStart(m), textureName(0), textureUvSize{1, 1}
	{}

	GuiImageFormatComponent::GuiImageFormatComponent() : animationSpeed(1), mode(ImageModeEnum::Stretch)
	{}

	GuiTextComponent::GuiTextComponent() : assetName(0), textName(0)
	{}

	GuiTextFormatComponent::GuiTextFormatComponent() : color(vec3::Nan()), font(0), size(real::Nan()), lineSpacing(real::Nan()), align((TextAlignEnum)-1)
	{}

	GuiSelectionComponent::GuiSelectionComponent() : start(m), length(0)
	{}

	GuiTooltipComponent::GuiTooltipComponent() : assetName(0), textName(0)
	{}

	GuiScrollbarsComponent::GuiScrollbarsComponent() : overflow{ OverflowModeEnum::Auto, OverflowModeEnum::Auto }
	{}

	GuiExplicitSizeComponent::GuiExplicitSizeComponent() : size(vec2::Nan())
	{}

	GuiEventComponent::GuiEventComponent()
	{}

	namespace privat
	{
		GuiGeneralComponents::GuiGeneralComponents(EntityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(CAGE_JOIN(Gui, CAGE_JOIN(T, Component))(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
