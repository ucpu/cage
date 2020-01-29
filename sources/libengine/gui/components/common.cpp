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
