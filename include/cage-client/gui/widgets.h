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
		bool skipMarginWhereMerging;
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
		EventOnAnyChange = 1 << 3,
		EventOnKeyEnter = 1 << 4,
		EventOnFocusLost = 1 << 5,
		// input box only
		ShowArrowButtons = 1 << 6,
		AlwaysRoundValueToStep = 1 << 7,
		// text area only
		CtrlEnterNewLine = 1 << 8, // supresses event from EventOnKeyEnter
		WriteTabs = 1 << 9, // tab key will write tab rather than skip to next widget
	};
	GCHL_ENUM_BITS(inputStyleFlags);

	struct CAGE_API inputBoxComponent
	{
		string value; // utf-8 encoded string (size is in bytes)
		union CAGE_API Union
		{
			real f;
			sint32 i;
			Union();
		} min, max, step;
		uint32 cursor; // utf-32 characters
		inputTypeEnum type;
		inputStyleFlags style;
		bool valid;
		// textComponent defines placeholder
		// textFormatComponent defines format for the (valid) value (not the placeholder)
		// selectionComponent defines selected text
		inputBoxComponent();
	};

	struct CAGE_API textAreaComponent
	{
		memoryBuffer *buffer; // utf-8 encoded string
		uint32 cursor; // utf-32 characters
		uint32 maxLength; // bytes
		inputStyleFlags style;
		// selectionComponent defines selected text
		textAreaComponent();
	};

	enum class checkBoxTypeEnum : uint32
	{
		CheckBox,
		TriStateBox,
		RadioButton,
	};

	enum class checkBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_API checkBoxComponent
	{
		checkBoxTypeEnum type;
		checkBoxStateEnum state;
		// textComponent defines label shown next to the check box
		checkBoxComponent();
	};

	struct CAGE_API comboBoxComponent
	{
		uint32 selected; // -1 = nothing selected
		// textComponent defines placeholder
		// children with textComponent defines individual lines
		// textFormatComponent applies to all lines (not the placeholder), may be overriden by individual childs
		// selectedItemComponent defines which line is selected (same as the selected property)
		comboBoxComponent();
	};

	struct CAGE_API listBoxComponent
	{
		// textComponent defines placeholder
		// children with textComponent defines individual lines
		// textFormatComponent applies to all lines (not the placeholder), may be overriden by individual childs
		// selectedItemComponent defines which lines are selected
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

	struct CAGE_API graphCanvasComponent
	{
		vec2 min;
		vec2 max;
		graphCanvasComponent();
	};

	enum class groupBoxTypeEnum : uint32
	{
		Invisible,
		Cell, // thin border without background
		Panel, // thick border and background
		Spoiler, // same as panel; plus a button inside caption area
	};

	enum class overflowModeEnum : uint32
	{
		Auto,
		Scroll,
		Hidden,
		Visible,
	};

	struct CAGE_API scrollableBaseComponentStruct
	{
		valuesStruct<2> minSize;
		vec2 scrollbars;
		overflowModeEnum overflow;
		scrollableBaseComponentStruct();
	};

	struct CAGE_API groupBoxComponent : public scrollableBaseComponentStruct
	{
		groupBoxTypeEnum type;
		bool spoilerCollapsesSiblings;
		bool spoilerCollapsed;
		groupBoxComponent();
		// textComponent defines caption
		// imageComponent defines background
	};

	enum class windowModeFlags : uint32
	{
		None = 0,
		Modal = 1 << 0, // prevents interaction with any other (non-modal) window
		Close = 1 << 1,
		Maximize = 1 << 2,
		Minimize = 1 << 3,
		Move = 1 << 4,
		Resize = 1 << 5,
		ForceShowOnTaskBar = 1 << 6,
	};
	GCHL_ENUM_BITS(windowModeFlags);

	enum class windowStateFlags : uint32
	{
		None = 0,
		Maximized = (uint32)windowModeFlags::Maximize,
		Minimized = (uint32)windowModeFlags::Minimize,
	};
	GCHL_ENUM_BITS(windowStateFlags);

	struct CAGE_API windowComponent : public scrollableBaseComponentStruct
	{
		windowModeFlags mode;
		windowStateFlags state;
		// textComponent defines caption
		// imageComponent defines background
		windowComponent();
	};

	enum class taskBarModeFlags : uint32
	{
		None = 0,
		ShowActive = 1 << 0, // minimized windows are not active
		ShowModal = 1 << 1,
	};
	GCHL_ENUM_BITS(taskBarModeFlags);

	struct CAGE_API taskBarComponent
	{
		taskBarModeFlags mode;
		// children with textComponent defines additional buttons
		// imageComponent defines background
		taskBarComponent();
	};

#define GCHL_GUI_WIDGET_COMPONENTS label, button, inputBox, textArea, checkBox, comboBox, listBox, progressBar, sliderBar, colorPicker, graphCanvas, groupBox, window, taskBar

	struct CAGE_API widgetsComponentsStruct
	{
#define GCHL_GENERATE(T) componentClass *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		widgetsComponentsStruct(entityManagerClass *ents);
	};
}
