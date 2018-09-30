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
	widgetStateComponent::widgetStateComponent() : skinIndex(-1), disabled(false)
	{}

	labelComponent::labelComponent()
	{}

	buttonComponent::buttonComponent() : allowMerging(false)
	{}

	inputComponent::Union::Union() : i(0)
	{}

	inputComponent::inputComponent() : cursor(-1), type(inputTypeEnum::Text), style(inputStyleFlags::ShowArrowButtons), valid(false)
	{}

	textAreaComponent::textAreaComponent() : buffer(nullptr), cursor(-1), maxLength(1024 * 1024), style(inputStyleFlags::None)
	{}

	checkBoxComponent::checkBoxComponent() : state(checkBoxStateEnum::Unchecked)
	{}

	radioBoxComponent::radioBoxComponent() : group(0), state(checkBoxStateEnum::Unchecked)
	{}

	comboBoxComponent::comboBoxComponent() : selected(-1)
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

	widgetsComponentsStruct::widgetsComponentsStruct(entityManagerClass *ents)
	{
		detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
	}
}
