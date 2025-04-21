#ifndef guard_guiSkin_h_4j8uz79rd
#define guard_guiSkin_h_4j8uz79rd

#include <cage-engine/guiComponents.h>

namespace cage
{
	class Image;

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
			Vec4 margin = Vec4(4);
			Label();
		} label;
		struct CAGE_ENGINE_API Header
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 margin = Vec4(4);
			Vec4 padding = Vec4(8);
			Vec2 size = Vec2(150, 42);
			Header();
		} header;
		struct CAGE_ENGINE_API Separator
		{
			struct Direction
			{
				Vec4 margin = Vec4(4);
				Vec2 size;
			} horizontal, vertical;
			Separator();
		} separator;
		struct CAGE_ENGINE_API Button
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 padding = Vec4(2);
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(150, 28);
			uint32 clickSound = 0;
			Button();
		} button;
		struct CAGE_ENGINE_API Input
		{
			GuiTextFormatComponent textValidFormat;
			GuiTextFormatComponent textInvalidFormat;
			GuiTextFormatComponent placeholderFormat;
			Vec4 basePadding = Vec4(2);
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(300, 28);
			Real buttonsWidth = 28;
			Real buttonsOffset = 2;
			InputButtonsPlacementModeEnum buttonsMode = InputButtonsPlacementModeEnum::Sides;
			uint32 typingValidSound = 0;
			uint32 typingInvalidSound = 0;
			uint32 confirmationSound = 0;
			// uint32 focusGainSound = 0;
			Input();
		} inputBox;
		struct CAGE_ENGINE_API TextArea
		{
			GuiTextFormatComponent textFormat;
			Vec4 padding = Vec4(4);
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(450, 200);
			TextArea();
		} textArea;
		struct CAGE_ENGINE_API CheckBox
		{
			GuiTextFormatComponent textFormat;
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(28);
			Vec2 labelOffset = Vec2(4, 6);
			uint32 clickSound = 0;
			CheckBox();
		} checkBox;
		struct CAGE_ENGINE_API RadioBox
		{
			GuiTextFormatComponent textFormat;
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(28);
			Vec2 labelOffset = Vec2(4, 5);
			uint32 clickSound = 0;
			RadioBox();
		} radioBox;
		struct CAGE_ENGINE_API ComboBox
		{
			GuiTextFormatComponent placeholderFormat;
			GuiTextFormatComponent itemsFormat;
			GuiTextFormatComponent selectedFormat;
			Vec4 basePadding = Vec4(2);
			Vec4 baseMargin = Vec4(1);
			Vec4 listPadding = Vec4(-4);
			Vec4 itemPadding = Vec4(1);
			Vec2 size = Vec2(250, 28);
			Real listOffset = -2;
			Real itemSpacing = -2;
			uint32 openSound = 0;
			uint32 selectSound = 0;
			ComboBox();
		} comboBox;
		struct CAGE_ENGINE_API ProgressBar
		{
			GuiTextFormatComponent textFormat;
			Vec4 baseMargin = Vec4(1);
			Vec4 textPadding = Vec4(2);
			Vec4 fillingPadding = Vec4();
			Vec2 size = Vec2(300, 28);
			uint32 fillingTextureName = 0;
			ProgressBar();
		} progressBar;
		struct CAGE_ENGINE_API SliderBar
		{
			struct Direction
			{
				Vec4 padding = Vec4(1);
				Vec4 margin = Vec4(1);
				Vec2 size;
				bool collapsedBar = true;
			} horizontal, vertical;
			uint32 slidingSound = 0;
			SliderBar();
		} sliderBar;
		struct CAGE_ENGINE_API ColorPicker
		{
			Vec4 margin = Vec4(1);
			Vec2 collapsedSize = Vec2(28);
			Vec2 fullSize = Vec2(250, 180);
			Real hueBarPortion = 0.18;
			Real resultBarPortion = 0.35;
			uint32 slidingSound = 0;
			ColorPicker();
		} colorPicker;
		struct CAGE_ENGINE_API SolidColor
		{
			Vec4 margin = Vec4(1);
			Vec2 size = Vec2(20);
			SolidColor();
		} solidColor;
		struct CAGE_ENGINE_API Frame
		{
			Vec4 margin = Vec4(1);
			Vec4 padding = Vec4(2);
			Frame();
		} frame;
		struct CAGE_ENGINE_API Panel
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 baseMargin = Vec4(1);
			Vec4 contentPadding = Vec4(2);
			Vec4 captionPadding = Vec4(2);
			Real captionHeight = 30;
			Panel();
		} panel;
		struct CAGE_ENGINE_API Spoiler
		{
			GuiTextFormatComponent textFormat;
			GuiImageFormatComponent imageFormat;
			Vec4 baseMargin = Vec4(1);
			Vec4 contentPadding = Vec4(2);
			Vec4 captionPadding = Vec4(2);
			Real captionHeight = 30;
			uint32 clickSound = 0;
			Spoiler();
		} spoiler;
		struct CAGE_ENGINE_API Scrollbars
		{
			Real scrollbarSize = 15;
			Real contentPadding = 4;
			uint32 slidingSound = 0;
			Scrollbars();
		} scrollbars;
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
		PanelBase,
		PanelCaption,
		Frame,
		SpoilerBase,
		SpoilerCaption,
		SpoilerIconCollapsed,
		SpoilerIconShown,
		Button,
		Input,
		InputButtonDecrement,
		InputButtonIncrement,
		TextArea,
		ComboBoxBase,
		ComboBoxList,
		ComboBoxItemUnchecked,
		ComboBoxItemChecked,
		ComboBoxIconCollapsed,
		ComboBoxIconShown,
		CheckBoxUnchecked,
		CheckBoxChecked,
		CheckBoxIndetermined,
		RadioBoxUnchecked,
		RadioBoxChecked,
		RadioBoxIndetermined,
		ScrollbarHorizontalPanel,
		ScrollbarVerticalPanel,
		ScrollbarHorizontalDot,
		ScrollbarVerticalDot,
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
		Header,
		SeparatorHorizontalLine,
		SeparatorVerticalLine,
		TotalElements,
		InvalidElement = m,
	};

	struct CAGE_ENGINE_API GuiSkinConfig
	{
		GuiSkinElementLayout layouts[(uint32)GuiElementTypeEnum::TotalElements];
		GuiSkinWidgetDefaults defaults;
		uint32 textureId = 0;
		uint32 hoverSound = 0;
		uint32 openTooltipSound = 0;
		GuiSkinConfig();
	};

	namespace detail
	{
		CAGE_ENGINE_API GuiSkinConfig guiSkinGenerate(GuiSkinIndex style);
		CAGE_ENGINE_API Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution);
	}
}

#endif // guard_guiSkin_h_4j8uz79rd
