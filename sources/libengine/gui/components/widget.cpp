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
	widgetStateComponent::widgetStateComponent() : skinIndex(m), disabled(false)
	{}

	labelComponent::labelComponent()
	{}

	buttonComponent::buttonComponent()
	{}

	inputComponent::Union::Union() : i(0)
	{}

	inputComponent::inputComponent() : cursor(m), type(inputTypeEnum::Text), style(inputStyleFlags::ShowArrowButtons), valid(false)
	{}

	textAreaComponent::textAreaComponent() : buffer(nullptr), cursor(m), maxLength(1024 * 1024), style(inputStyleFlags::None)
	{}

	checkBoxComponent::checkBoxComponent() : state(checkBoxStateEnum::Unchecked)
	{}

	radioBoxComponent::radioBoxComponent() : group(0), state(checkBoxStateEnum::Unchecked)
	{}

	comboBoxComponent::comboBoxComponent() : selected(m)
	{}

	listBoxComponent::listBoxComponent()
	{}

	progressBarComponent::progressBarComponent() : progress(0), showValue(false)
	{}

	sliderBarComponent::sliderBarComponent() : max(1), vertical(false)
	{}

	colorPickerComponent::colorPickerComponent() : collapsible(false), color(vec3(1,0,0))
	{}

	panelComponent::panelComponent()
	{}

	spoilerComponent::spoilerComponent() : collapsesSiblings(true), collapsed(true)
	{}

	namespace privat
	{
		guiWidgetsComponents::guiWidgetsComponents(EntityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
