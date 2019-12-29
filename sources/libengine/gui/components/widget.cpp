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
	WidgetStateComponent::WidgetStateComponent() : skinIndex(m), disabled(false)
	{}

	LabelComponent::LabelComponent()
	{}

	ButtonComponent::ButtonComponent()
	{}

	InputComponent::Union::Union() : i(0)
	{}

	InputComponent::InputComponent() : cursor(m), type(InputTypeEnum::Text), style(InputStyleFlags::ShowArrowButtons), valid(false)
	{}

	TextAreaComponent::TextAreaComponent() : buffer(nullptr), cursor(m), maxLength(1024 * 1024), style(InputStyleFlags::None)
	{}

	CheckBoxComponent::CheckBoxComponent() : state(CheckBoxStateEnum::Unchecked)
	{}

	RadioBoxComponent::RadioBoxComponent() : group(0), state(CheckBoxStateEnum::Unchecked)
	{}

	ComboBoxComponent::ComboBoxComponent() : selected(m)
	{}

	ListBoxComponent::ListBoxComponent()
	{}

	ProgressBarComponent::ProgressBarComponent() : progress(0), showValue(false)
	{}

	SliderBarComponent::SliderBarComponent() : max(1), vertical(false)
	{}

	ColorPickerComponent::ColorPickerComponent() : collapsible(false), color(vec3(1,0,0))
	{}

	PanelComponent::PanelComponent()
	{}

	SpoilerComponent::SpoilerComponent() : collapsesSiblings(true), collapsed(true)
	{}

	namespace privat
	{
		GuiWidgetsComponents::GuiWidgetsComponents(EntityManager *ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(T, Component)>(CAGE_JOIN(T, Component)(), false);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		}
	}
}
