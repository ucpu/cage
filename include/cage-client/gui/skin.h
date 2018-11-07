namespace cage
{
	enum class inputButtonsPlacementModeEnum : uint32
	{
		Left,
		Right,
		Sides,
	};

	struct CAGE_API skinWidgetDefaultsStruct // all sizes are in points
	{
		struct CAGE_API labelStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent imageFormat;
			vec4 margin;
			labelStruct();
		} label;
		struct CAGE_API buttonStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent imageFormat;
			vec4 padding, margin;
			vec2 size;
			buttonStruct();
		} button;
		struct CAGE_API inputStruct
		{
			textFormatComponent textValidFormat;
			textFormatComponent textInvalidFormat;
			textFormatComponent placeholderFormat;
			vec4 basePadding;
			vec4 margin;
			vec2 size;
			real buttonsSize;
			real buttonsOffset;
			inputButtonsPlacementModeEnum buttonsMode;
			inputStruct();
		} inputBox;
		struct CAGE_API textAreaStruct
		{
			textFormatComponent textFormat;
			vec4 padding, margin;
			vec2 size;
			textAreaStruct();
		} textArea;
		struct CAGE_API checkBoxStruct
		{
			textFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			checkBoxStruct();
		} checkBox;
		struct CAGE_API radioBoxStruct
		{
			textFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			radioBoxStruct();
		} radioBox;
		struct CAGE_API comboBoxStruct
		{
			textFormatComponent placeholderFormat;
			textFormatComponent itemsFormat;
			textFormatComponent selectedFormat;
			vec4 basePadding, baseMargin;
			vec4 listPadding, itemPadding;
			vec2 size;
			real listOffset, itemSpacing;
			comboBoxStruct();
		} comboBox;
		struct CAGE_API listBoxStruct
		{
			textFormatComponent textFormat;
			vec4 basePadding, baseMargin;
			vec4 itemPadding;
			vec2 size;
			real itemSpacing;
			listBoxStruct();
		} listBox;
		struct CAGE_API progressBarStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent backgroundImageFormat;
			imageFormatComponent fillingImageFormat;
			imageComponent fillingImage;
			vec4 baseMargin;
			vec4 textPadding, fillingPadding;
			vec2 size;
			progressBarStruct();
		} progressBar;
		struct CAGE_API sliderBarStruct
		{
			struct directionStruct
			{
				vec4 padding, margin;
				vec2 size;
				bool collapsedBar;
			} horizontal, vertical;
			sliderBarStruct();
		} sliderBar;
		struct CAGE_API colorPickerStruct
		{
			vec4 margin;
			vec2 collapsedSize;
			vec2 fullSize;
			real hueBarPortion;
			real resultBarPortion;
			colorPickerStruct();
		} colorPicker;
		struct CAGE_API panelStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			panelStruct();
		} panel;
		struct CAGE_API spoilerStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			spoilerStruct();
		} spoiler;
		struct CAGE_API scrollbarsStruct
		{
			real scrollbarSize;
			real contentPadding;
			scrollbarsStruct();
		} scrollbars;
		struct CAGE_API tooltipStruct
		{
			textFormatComponent textFormat;
			tooltipStruct();
		} tooltip;
		skinWidgetDefaultsStruct();
	};

	struct CAGE_API skinElementLayoutStruct
	{
		struct CAGE_API textureUvOiStruct
		{
			vec4 outer; // 0 .. 1
			vec4 inner;
		};
		struct CAGE_API textureUvStruct
		{
			textureUvOiStruct data[4]; // normal, focus, hover, disabled
			textureUvStruct();
		} textureUv;
		vec4 border; // left, top, right, bottom; all in points
		skinElementLayoutStruct();
	};

	enum class elementTypeEnum : uint32
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
		InvalidElement = (uint32)-1,
	};
}
