#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	parentComponent::parentComponent() : parent(0), order(0)
	{}

	imageComponent::imageComponent() : animationStart(-1), textureName(0), textureUvSize{1, 1}
	{}

	imageFormatComponent::imageFormatComponent() : animationSpeed(1), mode(imageModeEnum::Stretch)
	{}

	textComponent::textComponent() : assetName(0), textName(0)
	{}

	textFormatComponent::textFormatComponent() : color(vec3::Nan), font(0), size(real::Nan), lineSpacing(real::Nan), align((textAlignEnum)-1)
	{}

	selectionComponent::selectionComponent() : start(-1), length(0)
	{}

	scrollbarsComponent::scrollbarsComponent() : overflow{ overflowModeEnum::Auto, overflowModeEnum::Auto }
	{}

	explicitSizeComponent::explicitSizeComponent() : size(vec2::Nan)
	{}

	generalComponentsStruct::generalComponentsStruct(entityManagerClass *ents)
	{
		detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
	}
}
