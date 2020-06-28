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
			vec4 margin = vec4(1, 2, 1, 2);
			Label();
		} label;
		struct CAGE_ENGINE_API Button
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 padding = vec4(1), margin = vec4(1);
			vec2 size = vec2(150, 32);
			Button();
		} button;
		struct CAGE_ENGINE_API Input
		{
			GuiTextFormatComponent textValidFormat;
			GuiTextFormatComponent textInvalidFormat;
			GuiTextFormatComponent placeholderFormat;
			vec4 basePadding = vec4(2);
			vec4 margin = vec4(1);
			vec2 size = vec2(300, 32);
			real buttonsSize = 32;
			real buttonsOffset = 2;
			InputButtonsPlacementModeEnum buttonsMode = InputButtonsPlacementModeEnum::Sides;
			Input();
		} inputBox;
		struct CAGE_ENGINE_API TextArea
		{
			GuiTextFormatComponent textFormat;
			vec4 padding = vec4(3), margin = vec4(1);
			vec2 size = vec2(450, 200);
			TextArea();
		} textArea;
		struct CAGE_ENGINE_API CheckBox
		{
			GuiTextFormatComponent textFormat;
			vec4 margin = vec4(1);
			vec2 size = vec2(28);
			vec2 labelOffset = vec2(3, 5);
			CheckBox();
		} checkBox;
		struct CAGE_ENGINE_API RadioBox
		{
			GuiTextFormatComponent textFormat;
			vec4 margin = vec4(1);
			vec2 size = vec2(28);
			vec2 labelOffset = vec2(3, 5);
			RadioBox();
		} radioBox;
		struct CAGE_ENGINE_API ComboBox
		{
			GuiTextFormatComponent placeholderFormat;
			GuiTextFormatComponent itemsFormat;
			GuiTextFormatComponent selectedFormat;
			vec4 basePadding = vec4(1), baseMargin = vec4(1);
			vec4 listPadding = vec4(0), itemPadding = vec4(1, 2, 1, 2);
			vec2 size = vec2(250, 32);
			real listOffset = -6, itemSpacing = -2;
			ComboBox();
		} comboBox;
		struct CAGE_ENGINE_API ListBox
		{
			GuiTextFormatComponent textFormat;
			vec4 basePadding = vec4(0), baseMargin = vec4(1);
			vec4 itemPadding = vec4(1);
			vec2 size = vec2(250, 32);
			real itemSpacing = 0;
			ListBox();
		} listBox;
		struct CAGE_ENGINE_API ProgressBar
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent backgroundImageFormat;
			GuiImageFormatComponent fillingImageFormat;
			GuiImageComponent fillingImage;
			vec4 baseMargin = vec4(1);
			vec4 textPadding = vec4(1), fillingPadding = vec4(1);
			vec2 size = vec2(200, 28);
			ProgressBar();
		} progressBar;
		struct CAGE_ENGINE_API SliderBar
		{
			struct Direction
			{
				vec4 padding = vec4(1), margin = vec4(1);
				vec2 size;
				bool collapsedBar = true;
			} horizontal, vertical;
			SliderBar();
		} sliderBar;
		struct CAGE_ENGINE_API ColorPicker
		{
			vec4 margin = vec4(1);
			vec2 collapsedSize = vec2(40, 32);
			vec2 fullSize = vec2(250, 180);
			real hueBarPortion = 0.18;
			real resultBarPortion = 0.35;
		} colorPicker;
		struct CAGE_ENGINE_API Panel
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 baseMargin = vec4(1);
			vec4 contentPadding = vec4(2);
			vec4 captionPadding = vec4(3, 1, 3, 1);
			real captionHeight = 28;
			Panel();
		} panel;
		struct CAGE_ENGINE_API Spoiler
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			vec4 baseMargin = vec4(1);
			vec4 contentPadding = vec4(2);
			vec4 captionPadding = vec4(3, 1, 3, 1);
			real captionHeight = 28;
			Spoiler();
		} spoiler;
		struct CAGE_ENGINE_API Scrollbars
		{
			real scrollbarSize = 15;
			real contentPadding = 4;
		} scrollbars;
		struct CAGE_ENGINE_API Tooltip
		{
			GuiTextFormatComponent textFormat;
			Tooltip();
		} tooltip;
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
		} textureUv;
		vec4 border = vec4(5); // left, top, right, bottom; all in points
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
		uint32 textureName = 0;
		GuiSkinConfig();
	};

	namespace detail
	{
		CAGE_ENGINE_API Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution);
	}
}

#endif // guard_guiSkin_h_4j8uz79rd
