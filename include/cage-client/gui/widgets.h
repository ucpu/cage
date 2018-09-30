namespace cage
{
	struct CAGE_API labelComponent
	{
		// textComponent defines foreground of the widget
		// imageComponent defines background of the widget
		labelComponent();
	};

	struct CAGE_API buttonComponent
	{
		bool allowMerging;
		// textComponent defines foreground
		// imageComponent defines background
		buttonComponent();
	};

	enum class inputTypeEnum : uint32
	{
		Text,
		Url,
		Email,
		Integer,
		Real,
		Password,
	};

	enum class inputStyleFlags : uint32
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
	GCHL_ENUM_BITS(inputStyleFlags);

	struct CAGE_API inputComponent
	{
		string value; // utf-8 encoded string (size is in bytes)
		union CAGE_API Union
		{
			real f;
			sint32 i;
			Union();
		} min, max, step;
		uint32 cursor; // unicode characters (not bytes)
		inputTypeEnum type;
		inputStyleFlags style;
		bool valid;
		// textComponent defines placeholder
		// textFormatComponent defines format
		// selectionComponent defines selected text
		inputComponent();
	};

	struct CAGE_API textAreaComponent
	{
		// vec2 scrollbars;
		memoryBuffer *buffer; // utf-8 encoded string
		uint32 cursor; // unicode characters (not bytes)
		uint32 maxLength; // bytes
		inputStyleFlags style;
		// selectionComponent defines selected text
		textAreaComponent();
	};

	enum class checkBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_API checkBoxComponent
	{
		checkBoxStateEnum state;
		// textComponent defines label shown next to the check box
		checkBoxComponent();
	};

	struct CAGE_API radioBoxComponent
	{
		uint32 group; // defines what other radio buttons are unchecked when this becomes checked
		checkBoxStateEnum state;
		// textComponent defines label shown next to the radio box
		radioBoxComponent();
	};

	struct CAGE_API comboBoxComponent
	{
		// real scrollbar;
		uint32 selected; // -1 = nothing selected
		// textComponent defines placeholder
		// children with textComponent defines individual lines
		// textFormatComponent applies to all lines, may be overriden by individual childs
		// selectedItemComponent on childs defines which line is selected (the selected property is authoritative)
		comboBoxComponent();
	};

	struct CAGE_API listBoxComponent
	{
		// real scrollbar;
		// children with textComponent defines individual lines
		// textFormatComponent applies to all lines, may be overriden by individual childs
		// selectedItemComponent on childs defines which lines are selected
		listBoxComponent();
	};

	struct CAGE_API progressBarComponent
	{
		real progress; // 0 .. 1
		bool showValue; // overrides the text with the value (may use internationalization for formatting)
		// textComponent defines text shown over the bar
		progressBarComponent();
	};

	struct CAGE_API sliderBarComponent
	{
		real value;
		real min, max;
		bool vertical;
		sliderBarComponent();
	};

	struct CAGE_API colorPickerComponent
	{
		vec3 color;
		bool collapsible;
		colorPickerComponent();
	};

	struct CAGE_API panelComponent
	{
		panelComponent();
		// textComponent defines caption
		// imageComponent defines background
		// may be merged with scrollbarsComponent
	};

	struct CAGE_API spoilerComponent
	{
		bool collapsesSiblings;
		bool collapsed;
		spoilerComponent();
		// textComponent defines caption
		// imageComponent defines background
		// may be merged with scrollbarsComponent
	};

#define GCHL_GUI_WIDGET_COMPONENTS label, button, input, textArea, checkBox, radioBox, comboBox, listBox, progressBar, sliderBar, colorPicker, panel, spoiler

	struct CAGE_API widgetsComponentsStruct
	{
#define GCHL_GENERATE(T) componentClass *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		widgetsComponentsStruct(entityManagerClass *ents);
	};
}
