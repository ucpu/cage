namespace cage
{
	struct CAGE_API LabelComponent
	{
		// TextComponent defines foreground of the widget
		// ImageComponent defines background of the widget
		LabelComponent();
	};

	struct CAGE_API ButtonComponent
	{
		// TextComponent defines foreground
		// ImageComponent defines background
		ButtonComponent();
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
	GCHL_ENUM_BITS(InputStyleFlags);

	struct CAGE_API InputComponent
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
		// TextComponent defines placeholder
		// TextFormatComponent defines format
		// SelectionComponent defines selected text
		InputComponent();
	};

	struct CAGE_API TextAreaComponent
	{
		MemoryBuffer *buffer; // utf-8 encoded string
		uint32 cursor; // unicode characters (not bytes)
		uint32 maxLength; // bytes
		InputStyleFlags style;
		// SelectionComponent defines selected text
		TextAreaComponent();
	};

	enum class CheckBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_API CheckBoxComponent
	{
		CheckBoxStateEnum state;
		// TextComponent defines label shown next to the check box
		CheckBoxComponent();
	};

	struct CAGE_API RadioBoxComponent
	{
		uint32 group; // defines what other radio buttons are unchecked when this becomes checked
		CheckBoxStateEnum state;
		// TextComponent defines label shown next to the radio box
		RadioBoxComponent();
	};

	struct CAGE_API ComboBoxComponent
	{
		uint32 selected; // -1 = nothing selected
		// TextComponent defines placeholder
		// children with TextComponent defines individual lines
		// TextFormatComponent applies to all lines, may be overriden by individual childs
		// SelectedItemComponent on childs defines which line is selected (the selected property is authoritative)
		ComboBoxComponent();
	};

	struct CAGE_API ListBoxComponent
	{
		// real scrollbar;
		// children with TextComponent defines individual lines
		// TextFormatComponent applies to all lines, may be overriden by individual childs
		// SelectedItemComponent on childs defines which lines are selected
		ListBoxComponent();
	};

	struct CAGE_API ProgressBarComponent
	{
		real progress; // 0 .. 1
		bool showValue; // overrides the text with the value (may use internationalization for formatting)
		// TextComponent defines text shown over the bar
		ProgressBarComponent();
	};

	struct CAGE_API SliderBarComponent
	{
		real value;
		real min, max;
		bool vertical;
		SliderBarComponent();
	};

	struct CAGE_API ColorPickerComponent
	{
		vec3 color;
		bool collapsible;
		ColorPickerComponent();
	};

	struct CAGE_API PanelComponent
	{
		PanelComponent();
		// TextComponent defines caption
		// ImageComponent defines background
	};

	struct CAGE_API SpoilerComponent
	{
		bool collapsesSiblings;
		bool collapsed;
		SpoilerComponent();
		// TextComponent defines caption
		// ImageComponent defines background
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
