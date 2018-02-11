namespace cage
{
	enum class inputButtonsPlacementModeEnum : uint32
	{
		InsideRight,
		InsideLeft,
		InsideSides,
		OutsideRight,
		OutsideLeft,
		OutsideSides,
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
			vec4 normalPadding, pressedPadding, margin;
			vec2 size;
			buttonStruct();
		} button;
		struct CAGE_API inputBoxStruct
		{
			textFormatComponent textFormat;
			vec3 invalidValueTextColor;
			vec4 basePadding, buttonsPadding;
			vec4 margin;
			vec2 size;
			real buttonsOffset, buttonsSpacing;
			inputButtonsPlacementModeEnum buttonsMode;
			inputBoxStruct();
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
			real labelOffset;
			checkBoxStruct();
		} checkBox;
		struct CAGE_API comboBoxStruct
		{
			textFormatComponent textFormat;
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
			textFormatComponent textFormat;
			struct directionStruct
			{
				vec4 padding, margin;
				vec2 size;
			} horizontal, vertical;
			sliderBarStruct();
		} sliderBar;
		struct CAGE_API colorPickerStruct
		{
			vec2 collapsedSize;
			vec2 fullSize;
			// todo
			colorPickerStruct();
		} colorPicker;
		struct CAGE_API graphCanvasStruct
		{
			vec2 size;
			// todo
			graphCanvasStruct();
		} graphCanvas;
		struct CAGE_API scrollableBaseStruct
		{
			textFormatComponent textFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec2 scrollbarsSizes;
			real captionHeight;
			scrollableBaseStruct();
		};
		struct CAGE_API groupBoxStruct : public scrollableBaseStruct
		{
			imageFormatComponent imageFormat;
			groupBoxStruct();
		} groupBox;
		struct CAGE_API windowStruct : public scrollableBaseStruct
		{
			imageFormatComponent imageFormat;
			real buttonsSpacing;
			windowStruct();
		} window;
		struct CAGE_API taskBarStruct
		{
			textFormatComponent textFormat;
			imageFormatComponent imageFormat;
			vec2 size;
			taskBarStruct();
		} taskBar;
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
			union
			{
				struct
				{
					textureUvOiStruct normal, focus, hover, disab;
				};
				struct
				{
					textureUvOiStruct data[4];
				};
			};
			textureUvStruct();
		} textureUv;
		vec4 border; // left, top, right, bottom; all in points
		skinElementLayoutStruct();
	};

	enum class elementTypeEnum : uint32
	{
		ButtonNormal,
		ButtonPressed,
		ButtonNormalTop,
		ButtonPressedTop,
		ButtonNormalBottom,
		ButtonPressedBottom,
		ButtonNormalLeft,
		ButtonPressedLeft,
		ButtonNormalRight,
		ButtonPressedRight,
		ButtonNormalHorizontal,
		ButtonPressedHorizontal,
		ButtonNormalVertical,
		ButtonPressedVertical,
		TextArea,
		InputBox,
		InputButtonNormalIncrement,
		InputButtonPressedIncrement,
		InputButtonNormalDecrement,
		InputButtonPressedDecrement,
		SliderHorizontalPanel,
		SliderVerticalPanel,
		SliderHorizontalDot,
		SliderVerticalDot,
		ScrollbarHorizontalPanel,
		ScrollbarVerticalPanel,
		ScrollbarHorizontalDot,
		ScrollbarVerticalDot,
		RadioBoxUnchecked,
		RadioBoxChecked,
		CheckBoxUnchecked,
		CheckBoxChecked,
		CheckBoxIndetermined,
		ProgressBar,
		ComboBoxBase,
		ComboBoxList,
		ComboBoxItem,
		ComboBoxSelectedItem,
		ListBoxList,
		ListBoxItem,
		ListBoxCheckedItem,
		ToolTip,
		ColorPickerCompact,
		ColorPickerBase,
		ColorPickerRect,
		ColorPickerSlider,
		ColorPickerSliderCursor,
		GroupCell,
		GroupPanel,
		GroupCaption,
		GroupSpoilerCollapsed,
		GroupSpoilerShown,
		WindowBaseNormal,
		WindowBaseModal,
		WindowCaption,
		WindowButtonMinimize,
		WindowButtonMaximize,
		WindowButtonRestore,
		WindowButtonClose,
		WindowResizer,
		TaskBarBase,
		TaskBarItem,
		TotalElements,
		InvalidElement = (uint32)-1,
	};
}
