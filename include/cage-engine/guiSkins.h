#ifndef guard_guiSkin_h_4j8uz79rd
#define guard_guiSkin_h_4j8uz79rd

#include "gui.h"

namespace cage
{
	enum class InputButtonsPlacementModeEnum : uint32
	{
		Left,
		Right,
		Sides,
	};

	struct CAGE_ENGINE_API GuiSkinWidgetDefaults // all sizes are in points
	{
		struct CAGE_ENGINE_API Label
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 margin;
			Label();
		} label;
		struct CAGE_ENGINE_API Button
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 padding, margin;
			vec2 size;
			Button();
		} button;
		struct CAGE_ENGINE_API Input
		{
			GuiTextFormatComponent textValidFormat;
			GuiTextFormatComponent textInvalidFormat;
			GuiTextFormatComponent placeholderFormat;
			vec4 basePadding;
			vec4 margin;
			vec2 size;
			real buttonsSize;
			real buttonsOffset;
			InputButtonsPlacementModeEnum buttonsMode;
			Input();
		} inputBox;
		struct CAGE_ENGINE_API TextArea
		{
			GuiTextFormatComponent textFormat;
			vec4 padding, margin;
			vec2 size;
			TextArea();
		} textArea;
		struct CAGE_ENGINE_API CheckBox
		{
			GuiTextFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			CheckBox();
		} checkBox;
		struct CAGE_ENGINE_API RadioBox
		{
			GuiTextFormatComponent textFormat;
			vec4 margin;
			vec2 size;
			vec2 labelOffset;
			RadioBox();
		} radioBox;
		struct CAGE_ENGINE_API ComboBox
		{
			GuiTextFormatComponent placeholderFormat;
			GuiTextFormatComponent itemsFormat;
			GuiTextFormatComponent selectedFormat;
			vec4 basePadding, baseMargin;
			vec4 listPadding, itemPadding;
			vec2 size;
			real listOffset, itemSpacing;
			ComboBox();
		} comboBox;
		struct CAGE_ENGINE_API ListBox
		{
			GuiTextFormatComponent textFormat;
			vec4 basePadding, baseMargin;
			vec4 itemPadding;
			vec2 size;
			real itemSpacing;
			ListBox();
		} listBox;
		struct CAGE_ENGINE_API ProgressBar
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent backgroundImageFormat;
			GuiImageFormatComponent fillingImageFormat;
			GuiImageComponent fillingImage;
			vec4 baseMargin;
			vec4 textPadding, fillingPadding;
			vec2 size;
			ProgressBar();
		} progressBar;
		struct CAGE_ENGINE_API SliderBar
		{
			struct Direction
			{
				vec4 padding, margin;
				vec2 size;
				bool collapsedBar;
			} horizontal, vertical;
			SliderBar();
		} sliderBar;
		struct CAGE_ENGINE_API ColorPicker
		{
			vec4 margin;
			vec2 collapsedSize;
			vec2 fullSize;
			real hueBarPortion;
			real resultBarPortion;
			ColorPicker();
		} colorPicker;
		struct CAGE_ENGINE_API Panel
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			Panel();
		} panel;
		struct CAGE_ENGINE_API Spoiler
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 baseMargin;
			vec4 contentPadding;
			vec4 captionPadding;
			real captionHeight;
			Spoiler();
		} spoiler;
		struct CAGE_ENGINE_API Scrollbars
		{
			real scrollbarSize;
			real contentPadding;
			Scrollbars();
		} scrollbars;
		struct CAGE_ENGINE_API Tooltip
		{
			GuiTextFormatComponent textFormat;
			Tooltip();
		} tooltip;
		GuiSkinWidgetDefaults();
	};

	struct CAGE_ENGINE_API GuiSkinElementLayout
	{
		struct CAGE_ENGINE_API TextureUvOi
		{
			vec4 outer; // 0 .. 1
			vec4 inner;
		};
		struct CAGE_ENGINE_API TextureUv
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

	struct CAGE_ENGINE_API GuiSkinConfig
	{
		GuiSkinElementLayout layouts[(uint32)GuiElementTypeEnum::TotalElements];
		GuiSkinWidgetDefaults defaults;
		uint32 textureName;
		GuiSkinConfig();
	};

	namespace detail
	{
		CAGE_ENGINE_API Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution);
	}
}

#endif // guard_guiSkin_h_4j8uz79rd
