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
	parentComponent::parentComponent() : parent(0), order(0)
	{}

	imageComponent::imageComponent() : animationStart(m), textureName(0), textureUvSize{1, 1}
	{}

	imageFormatComponent::imageFormatComponent() : animationSpeed(1), mode(imageModeEnum::Stretch)
	{}

	textComponent::textComponent() : assetName(0), textName(0)
	{}

	textFormatComponent::textFormatComponent() : color(vec3::Nan()), font(0), size(real::Nan()), lineSpacing(real::Nan()), align((textAlignEnum)-1)
	{}

	selectionComponent::selectionComponent() : start(m), length(0)
	{}

	scrollbarsComponent::scrollbarsComponent() : overflow{ overflowModeEnum::Auto, overflowModeEnum::Auto }
	{}

	explicitSizeComponent::explicitSizeComponent() : size(vec2::Nan())
	{}

	eventComponent::eventComponent()
	{}

	namespace privat
	{
		guiGeneralComponents::guiGeneralComponents(entityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}