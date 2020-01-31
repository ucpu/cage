#include "../private.h"

namespace cage
{
	GuiWidgetStateComponent::GuiWidgetStateComponent() : skinIndex(m), disabled(false)
	{}

	GuiLabelComponent::GuiLabelComponent()
	{}

	GuiButtonComponent::GuiButtonComponent()
	{}

	GuiInputComponent::Union::Union() : i(0)
	{}

	GuiInputComponent::GuiInputComponent() : cursor(m), type(InputTypeEnum::Text), style(InputStyleFlags::ShowArrowButtons), valid(false)
	{}

	GuiTextAreaComponent::GuiTextAreaComponent() : buffer(nullptr), cursor(m), maxLength(1024 * 1024), style(InputStyleFlags::None)
	{}

	GuiCheckBoxComponent::GuiCheckBoxComponent() : state(CheckBoxStateEnum::Unchecked)
	{}

	GuiRadioBoxComponent::GuiRadioBoxComponent() : group(0), state(CheckBoxStateEnum::Unchecked)
	{}

	GuiComboBoxComponent::GuiComboBoxComponent() : selected(m)
	{}

	GuiListBoxComponent::GuiListBoxComponent()
	{}

	GuiProgressBarComponent::GuiProgressBarComponent() : progress(0), showValue(false)
	{}

	GuiSliderBarComponent::GuiSliderBarComponent() : max(1), vertical(false)
	{}

	GuiColorPickerComponent::GuiColorPickerComponent() : collapsible(false), color(vec3(1,0,0))
	{}

	GuiPanelComponent::GuiPanelComponent()
	{}

	GuiSpoilerComponent::GuiSpoilerComponent() : collapsesSiblings(true), collapsed(true)
	{}
}
