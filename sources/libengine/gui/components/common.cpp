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
	ParentComponent::ParentComponent() : parent(0), order(0)
	{}

	ImageComponent::ImageComponent() : animationStart(m), textureName(0), textureUvSize{1, 1}
	{}

	ImageFormatComponent::ImageFormatComponent() : animationSpeed(1), mode(ImageModeEnum::Stretch)
	{}

	TextComponent::TextComponent() : assetName(0), textName(0)
	{}

	TextFormatComponent::TextFormatComponent() : color(vec3::Nan()), font(0), size(real::Nan()), lineSpacing(real::Nan()), align((TextAlignEnum)-1)
	{}

	SelectionComponent::SelectionComponent() : start(m), length(0)
	{}

	TooltipComponent::TooltipComponent() : assetName(0), textName(0)
	{}

	ScrollbarsComponent::ScrollbarsComponent() : overflow{ OverflowModeEnum::Auto, OverflowModeEnum::Auto }
	{}

	ExplicitSizeComponent::ExplicitSizeComponent() : size(vec2::Nan())
	{}

	EventComponent::EventComponent()
	{}

	namespace privat
	{
		GuiGeneralComponents::GuiGeneralComponents(EntityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
