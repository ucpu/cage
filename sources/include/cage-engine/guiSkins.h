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
			Vec4 margin = Vec4(1, 2, 1, 2);
			Label();
		} label;
		struct CAGE_ENGINE_API Button
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 padding = Vec4(1), margin = Vec4(1);
			Vec2 size = Vec2(150, 32);
			Button();
		} button;
		struct CAGE_ENGINE_API Input
		{
			GuiTextFormatComponent textValidFormat;
			GuiTextFormatComponent textInvalidFormat;
			GuiTextFormatComponent placeholderFormat;
			Vec4 basePadding = Vec4(2);
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(300, 32);
			Real buttonsSize = 32;
			Real buttonsOffset = 2;
			InputButtonsPlacementModeEnum buttonsMode = InputButtonsPlacementModeEnum::Sides;
			Input();
		} inputBox;
		struct CAGE_ENGINE_API TextArea
		{
			GuiTextFormatComponent textFormat;
			Vec4 padding = Vec4(3), margin = Vec4(1);
			Vec2 size = Vec2(450, 200);
			TextArea();
		} textArea;
		struct CAGE_ENGINE_API CheckBox
		{
			GuiTextFormatComponent textFormat;
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(28);
			Vec2 labelOffset = Vec2(3, 5);
			CheckBox();
		} checkBox;
		struct CAGE_ENGINE_API RadioBox
		{
			GuiTextFormatComponent textFormat;
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(28);
			Vec2 labelOffset = Vec2(3, 5);
			RadioBox();
		} radioBox;
		struct CAGE_ENGINE_API ComboBox
		{
			GuiTextFormatComponent placeholderFormat;
			GuiTextFormatComponent itemsFormat;
			GuiTextFormatComponent selectedFormat;
			Vec4 basePadding = Vec4(1), baseMargin = Vec4(1);
			Vec4 listPadding = Vec4(0), itemPadding = Vec4(1, 2, 1, 2);
			Vec2 size = Vec2(250, 32);
			Real listOffset = -6, itemSpacing = -2;
			ComboBox();
		} comboBox;
		struct CAGE_ENGINE_API ListBox
		{
			GuiTextFormatComponent textFormat;
			Vec4 basePadding = Vec4(0), baseMargin = Vec4(1);
			Vec4 itemPadding = Vec4(1);
			Vec2 size = Vec2(250, 32);
			Real itemSpacing = 0;
			ListBox();
		} listBox;
		struct CAGE_ENGINE_API ProgressBar
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent backgroundImageFormat;
			GuiImageFormatComponent fillingImageFormat;
			GuiImageComponent fillingImage;
			Vec4 baseMargin = Vec4(1);
			Vec4 textPadding = Vec4(1), fillingPadding = Vec4(1);
			Vec2 size = Vec2(200, 28);
			ProgressBar();
		} progressBar;
		struct CAGE_ENGINE_API SliderBar
		{
			struct Direction
			{
				Vec4 padding = Vec4(1), margin = Vec4(1);
				Vec2 size;
				bool collapsedBar = true;
			} horizontal, vertical;
			SliderBar();
		} sliderBar;
		struct CAGE_ENGINE_API ColorPicker
		{
			Vec4 margin = Vec4(1);
			Vec2 collapsedSize = Vec2(40, 32);
			Vec2 fullSize = Vec2(250, 180);
			Real hueBarPortion = 0.18;
			Real resultBarPortion = 0.35;
		} colorPicker;
		struct CAGE_ENGINE_API Panel
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 baseMargin = Vec4(1);
			Vec4 contentPadding = Vec4(2);
			Vec4 captionPadding = Vec4(3, 1, 3, 1);
			Real captionHeight = 28;
			Panel();
		} panel;
		struct CAGE_ENGINE_API Spoiler
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 baseMargin = Vec4(1);
			Vec4 contentPadding = Vec4(2);
			Vec4 captionPadding = Vec4(3, 1, 3, 1);
			Real captionHeight = 28;
			Spoiler();
		} spoiler;
		struct CAGE_ENGINE_API Scrollbars
		{
			Real scrollbarSize = 15;
			Real contentPadding = 4;
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
			Vec4 outer; // 0 .. 1
			Vec4 inner;
		};
		struct CAGE_ENGINE_API TextureUv
		{
			TextureUvOi data[4]; // normal, focus, hover, disabled
		} textureUv;
		Vec4 border = Vec4(5); // left, top, right, bottom; all in points
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
