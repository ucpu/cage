namespace cage
{
	enum class InputButtonsPlacementModeEnum : uint32
	{
		Left,
		Right,
		Sides,
	};

	struct CAGE_API GuiSkinWidgetDefaults // all sizes are in points
	{
		struct CAGE_API Label
		{
			TextFormatComponent textFormat;
			ImageFormatComponent imageFormat;
			vec4 margin;
			Label();
		} label;
		struct CAGE_API Button
		{
			TextFormatComponent textFormat;
			ImageFormatComponent imageFormat;
			vec4 padding, margin;
			vec2 size;
			Button();
		} button;
		struct CAGE_API Input
		{
			TextFormatComponent textValidFormat;
			TextFormatComponent textInvalidFormat;
			TextFormatComponent placeholderFormat;
			vec4 basePadding;
			vec4 margin;
			vec2 size;
			real buttonsSize;
			real buttonsOffset;
			InputButtonsPlacementModeEnum buttonsMode;
			Input();
		} inputBox;
		struct CAGE_API TextArea
		{
			TextFormatComponent textFormat;
			vec4 padding, margin;
			vec2 size;
			TextArea();
		} textArea;
		struct CAGE_API CheckBox
		{
			TextFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			CheckBox();
		} checkBox;
		struct CAGE_API RadioBox
		{
			TextFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			RadioBox();
		} radioBox;
		struct CAGE_API ComboBox
		{
			TextFormatComponent placeholderFormat;
			TextFormatComponent itemsFormat;
			TextFormatComponent selectedFormat;
			vec4 basePadding, baseMargin;
			vec4 listPadding, itemPadding;
			vec2 size;
			real listOffset, itemSpacing;
			ComboBox();
		} comboBox;
		struct CAGE_API ListBox
		{
			TextFormatComponent textFormat;
			vec4 basePadding, baseMargin;
			vec4 itemPadding;
			vec2 size;
			real itemSpacing;
			ListBox();
		} listBox;
		struct CAGE_API ProgressBar
		{
			TextFormatComponent textFormat;
			ImageFormatComponent backgroundImageFormat;
			ImageFormatComponent fillingImageFormat;
			ImageComponent fillingImage;
			vec4 baseMargin;
			vec4 textPadding, fillingPadding;
			vec2 size;
			ProgressBar();
		} progressBar;
		struct CAGE_API SliderBar
		{
			struct Direction
			{
				vec4 padding, margin;
				vec2 size;
				bool collapsedBar;
			} horizontal, vertical;
			SliderBar();
		} sliderBar;
		struct CAGE_API ColorPicker
		{
			vec4 margin;
			vec2 collapsedSize;
			vec2 fullSize;
			real hueBarPortion;
			real resultBarPortion;
			ColorPicker();
		} colorPicker;
		struct CAGE_API Panel
		{
			TextFormatComponent textFormat;
			ImageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			Panel();
		} panel;
		struct CAGE_API Spoiler
		{
			TextFormatComponent textFormat;
			ImageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			Spoiler();
		} spoiler;
		struct CAGE_API Scrollbars
		{
			real scrollbarSize;
			real contentPadding;
			Scrollbars();
		} scrollbars;
		struct CAGE_API Tooltip
		{
			TextFormatComponent textFormat;
			Tooltip();
		} tooltip;
		GuiSkinWidgetDefaults();
	};

	struct CAGE_API GuiSkinElementLayout
	{
		struct CAGE_API TextureUvOi
		{
			vec4 outer; // 0 .. 1
			vec4 inner;
		};
		struct CAGE_API TextureUv
		{
			TextureUvOi data[4]; // normal, focus, hover, disabled
			TextureUv();
		} textureUv;
		vec4 border; // left, top, right, bottom; all in points
		GuiSkinElementLayout();
	};

	enum class GuiElementTypeEnum : uint32
	{
		ScrollbarHorizontalPanel,
		ScrollbarVerticalPanel,
		ScrollbarHorizontalDot,
		ScrollbarVerticalDot,
		ToolTip,
		PanelBase,
		PanelCaption,
		SpoilerBase,
		SpoilerCaption,
		SpoilerIconCollapsed,
		SpoilerIconShown,
		Button,
		Input,
		InputButtonDecrement,
		InputButtonIncrement,
		TextArea,
		CheckBoxUnchecked,
		CheckBoxChecked,
		CheckBoxIndetermined,
		RadioBoxUnchecked,
		RadioBoxChecked,
		RadioBoxIndetermined,
		ComboBoxBase,
		ComboBoxList,
		ComboBoxItemUnchecked,
		ComboBoxItemChecked,
		ListBoxBase,
		ListBoxItemUnchecked,
		ListBoxItemChecked,
		SliderHorizontalPanel,
		SliderVerticalPanel,
		SliderHorizontalDot,
		SliderVerticalDot,
		ProgressBar,
		ColorPickerCompact,
		ColorPickerFull,
		ColorPickerHuePanel,
		ColorPickerSatValPanel,
		ColorPickerPreviewPanel,
		TotalElements,
		InvalidElement = m,
	};
}
