namespace cage
{
	struct CAGE_API GuiLabelComponent
	{
		// GuiTextComponent defines foreground of the widget
		// GuiImageComponent defines background of the widget
		GuiLabelComponent();
	};

	struct CAGE_API GuiButtonComponent
	{
		// GuiTextComponent defines foreground
		// GuiImageComponent defines background
		GuiButtonComponent();
	};

	enum class InputTypeEnum : uint32
	{
		Text,
		Url,
		Email,
		Integer,
		Real,
		Password,
	};

	enum class InputStyleFlags : uint32
	{
		// input box and text area
		None = 0,
		ReadOnly = 1 << 0,
		SelectAllOnFocusGain = 1 << 1,
		GoToEndOnFocusGain = 1 << 2,
		ShowArrowButtons = 1 << 3,
		AlwaysRoundValueToStep = 1 << 4,
		//WriteTabs = 1 << 5, // tab key will write tab rather than skip to next widget
	};

	struct CAGE_API GuiInputComponent
	{
		string value; // utf-8 encoded string (size is in bytes)
		union CAGE_API Union
		{
			real f;
			sint32 i;
			Union();
		} min, max, step;
		uint32 cursor; // unicode characters (not bytes)
		InputTypeEnum type;
		InputStyleFlags style;
		bool valid;
		// GuiTextComponent defines placeholder
		// GuiTextFormatComponent defines format
		// GuiSelectionComponent defines selected text
		GuiInputComponent();
	};

	struct CAGE_API GuiTextAreaComponent
	{
		MemoryBuffer *buffer; // utf-8 encoded string
		uint32 cursor; // unicode characters (not bytes)
		uint32 maxLength; // bytes
		InputStyleFlags style;
		// GuiSelectionComponent defines selected text
		GuiTextAreaComponent();
	};

	enum class CheckBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_API GuiCheckBoxComponent
	{
		CheckBoxStateEnum state;
		// GuiTextComponent defines label shown next to the check box
		GuiCheckBoxComponent();
	};

	struct CAGE_API GuiRadioBoxComponent
	{
		uint32 group; // defines what other radio buttons are unchecked when this becomes checked
		CheckBoxStateEnum state;
		// GuiTextComponent defines label shown next to the radio box
		GuiRadioBoxComponent();
	};

	struct CAGE_API GuiComboBoxComponent
	{
		uint32 selected; // -1 = nothing selected
		// GuiTextComponent defines placeholder
		// children with GuiTextComponent defines individual lines
		// GuiTextFormatComponent applies to all lines, may be overriden by individual childs
		// GuiSelectedItemComponent on childs defines which line is selected (the selected property is authoritative)
		GuiComboBoxComponent();
	};

	struct CAGE_API GuiListBoxComponent
	{
		// real scrollbar;
		// children with GuiTextComponent defines individual lines
		// GuiTextFormatComponent applies to all lines, may be overriden by individual childs
		// GuiSelectedItemComponent on childs defines which lines are selected
		GuiListBoxComponent();
	};

	struct CAGE_API GuiProgressBarComponent
	{
		real progress; // 0 .. 1
		bool showValue; // overrides the text with the value (may use internationalization for formatting)
		// GuiTextComponent defines text shown over the bar
		GuiProgressBarComponent();
	};

	struct CAGE_API GuiSliderBarComponent
	{
		real value;
		real min, max;
		bool vertical;
		GuiSliderBarComponent();
	};

	struct CAGE_API GuiColorPickerComponent
	{
		vec3 color;
		bool collapsible;
		GuiColorPickerComponent();
	};

	struct CAGE_API GuiPanelComponent
	{
		GuiPanelComponent();
		// GuiTextComponent defines caption
		// GuiImageComponent defines background
	};

	struct CAGE_API GuiSpoilerComponent
	{
		bool collapsesSiblings;
		bool collapsed;
		GuiSpoilerComponent();
		// GuiTextComponent defines caption
		// GuiImageComponent defines background
	};

#define GCHL_GUI_WIDGET_COMPONENTS Label, Button, Input, TextArea, CheckBox, RadioBox, ComboBox, ListBox, ProgressBar, SliderBar, ColorPicker, Panel, Spoiler

	namespace privat
	{
		struct CAGE_API GuiWidgetsComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
			GuiWidgetsComponents(EntityManager *ents);
		};
	}
}
