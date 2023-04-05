#ifndef guard_guiComponents_sdf1gh45hk485aws
#define guard_guiComponents_sdf1gh45hk485aws

#include "core.h"

namespace cage
{
	struct MemoryBuffer;
	class Entity;

	struct CAGE_ENGINE_API GuiParentComponent
	{
		uint32 parent = 0;
		sint32 order = 0;
	};

	struct CAGE_ENGINE_API GuiImageComponent
	{
		uint64 animationStart = m; // -1 will be replaced by current time
		Vec2 textureUvOffset;
		Vec2 textureUvSize = Vec2(1);
		uint32 textureName = 0;
	};

	enum class ImageModeEnum : uint32
	{
		Stretch,
		// todo
	};

	struct CAGE_ENGINE_API GuiImageFormatComponent
	{
		Real animationSpeed = 1;
		Real animationOffset;
		ImageModeEnum mode = ImageModeEnum::Stretch;
	};

	struct CAGE_ENGINE_API GuiTextComponent
	{
		String value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName = 0;
		uint32 textName = 0;
	};

	struct CAGE_ENGINE_API GuiTextFormatComponent
	{
		Vec3 color = Vec3::Nan();
		uint32 font = 0;
		Real size = Real::Nan();
		Real lineSpacing = Real::Nan();
		TextAlignEnum align = (TextAlignEnum)m;
	};

	struct CAGE_ENGINE_API GuiSelectionComponent
	{
		uint32 start = m; // utf32 characters (not bytes)
		uint32 length = 0; // utf32 characters (not bytes)
	};

	struct CAGE_ENGINE_API GuiWidgetStateComponent
	{
		uint32 skinIndex = m; // -1 = inherit
		bool disabled = false;
	};

	struct CAGE_ENGINE_API GuiSelectedItemComponent
	{};

	struct CAGE_ENGINE_API GuiExplicitSizeComponent
	{
		Vec2 size = Vec2::Nan();
	};

	struct CAGE_ENGINE_API GuiEventComponent
	{
		Delegate<bool(Entity *)> event;
	};

	enum class TooltipCloseConditionEnum : uint32
	{
		Instant, // the tooltip is closed as soon as the cursor moves
		Modal, // the tooltip acts as a modal window and is closed only when the cursor moves outside of the tooltip
		Never, // the application is responsible for closing the tooltip by removing the entity
	};

	enum class TooltipPlacementEnum : uint32
	{
		Corner, // corner of the tooltip positioned at the cursor
		Center,
		Manual,
	};

	struct CAGE_ENGINE_API GuiTooltipConfig
	{
		Entity *invoker = nullptr; // the widget for which the tooltip is to be shown
		Entity *tooltip = nullptr; // entity automatically prepared by the GuiManager for the application to fill in
		Vec2 anchor; // cursor position
		mutable TooltipCloseConditionEnum closeCondition = TooltipCloseConditionEnum::Instant;
		mutable TooltipPlacementEnum placement = TooltipPlacementEnum::Corner;
	};

	struct CAGE_ENGINE_API GuiTooltipComponent
	{
		using Tooltip = Delegate<void(const GuiTooltipConfig &)>;
		Tooltip tooltip;
		uint64 delay = 500000; // duration to hold mouse over the widget before showing the tooltip
		bool enableForDisabled = false;
	};

	enum class LineEdgeModeEnum : uint32
	{
		None, // elements are stretched so that the first/last element touches the edge
		Spaced, // elements are spaced so that the first/last element touches the edge
		Flexible, // the first/last element stretches to fill all the space to the edge
		Empty, // the space between first/last element and the edge is left empty
	};

	struct CAGE_ENGINE_API GuiLayoutLineComponent
	{
		Real crossAlign = Real::Nan(); // applied to items individually in the secondary axis, nan = fill the space, 0 = left/top, 1 = right/bottom
		LineEdgeModeEnum begin = LineEdgeModeEnum::None;
		LineEdgeModeEnum end = LineEdgeModeEnum::None;
		bool vertical = false; // false -> items go left-to-right, true -> items go top-to-bottom
	};

	struct CAGE_ENGINE_API GuiLayoutTableComponent
	{
		uint32 sections = 2; // set sections to 0 to make it square-ish
		bool grid = false; // false -> each column and row sizes are independent; true -> all columns and rows have same sizes
		bool vertical = true; // false -> fills entire row; true -> fills entire column
	};

	struct CAGE_ENGINE_API GuiLayoutAlignmentComponent
	{
		Vec2 alignment = Vec2(0.5); // use NaN to fill the area in the particular axis
	};

	enum class OverflowModeEnum : uint32
	{
		Auto, // show scrollbar when needed only
		Always, // always show scrollbar
		Never, // never show scrollbar
		// overflowing content is hidden irrespective of this setting
	};

	struct CAGE_ENGINE_API GuiLayoutScrollbarsComponent
	{
		Vec2 scroll;
		OverflowModeEnum overflow[2] = { OverflowModeEnum::Auto, OverflowModeEnum::Auto };
	};

	struct CAGE_ENGINE_API GuiLabelComponent
	{
		// GuiTextComponent defines foreground of the widget
		// GuiImageComponent defines background of the widget
	};

	struct CAGE_ENGINE_API GuiButtonComponent
	{
		// GuiTextComponent defines foreground
		// GuiImageComponent defines background
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
		//ReadOnly = 1 << 0,
		//SelectAllOnFocusGain = 1 << 1,
		GoToEndOnFocusGain = 1 << 2,
		ShowArrowButtons = 1 << 3,
		AlwaysRoundValueToStep = 1 << 4,
		//AcceptTabs = 1 << 5, // tab key will write tab rather than skip to next widget
	};

	struct CAGE_ENGINE_API GuiInputComponent
	{
		String value; // utf8 encoded string (size is in bytes)
		union CAGE_ENGINE_API Union
		{
			Real f;
			sint32 i = 0;
			Union() {};
		} min, max, step;
		uint32 cursor = m; // utf32 characters (not bytes)
		InputTypeEnum type = InputTypeEnum::Text;
		InputStyleFlags style = InputStyleFlags::ShowArrowButtons;
		bool valid = false;
		// GuiTextComponent defines placeholder
		// GuiTextFormatComponent defines format
		// GuiSelectionComponent defines selected text
	};

	struct CAGE_ENGINE_API GuiTextAreaComponent
	{
		MemoryBuffer *buffer = nullptr; // utf8 encoded string (size is in bytes)
		uint32 cursor = m; // utf32 characters (not bytes)
		uint32 maxLength = 1024 * 1024; // bytes
		InputStyleFlags style = InputStyleFlags::None;
		// GuiSelectionComponent defines selected text
	};

	enum class CheckBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_ENGINE_API GuiCheckBoxComponent
	{
		CheckBoxStateEnum state = CheckBoxStateEnum::Unchecked;
		// GuiTextComponent defines label shown next to the check box
	};

	struct CAGE_ENGINE_API GuiRadioBoxComponent
	{
		CheckBoxStateEnum state = CheckBoxStateEnum::Unchecked;
		// GuiTextComponent defines label shown next to the radio box
	};

	struct CAGE_ENGINE_API GuiComboBoxComponent
	{
		uint32 selected = m; // -1 = nothing selected
		// GuiTextComponent defines placeholder
		// children with GuiTextComponent defines individual lines
		// GuiTextFormatComponent applies to all lines, may be overridden by individual childs
		// GuiSelectedItemComponent on childs defines which line is selected (the selected property is authoritative)
	};

	struct CAGE_ENGINE_API GuiProgressBarComponent
	{
		Real progress; // 0 .. 1
		bool showValue = false; // overrides the text with the value (may use internationalization for formatting)
		// GuiTextComponent defines text shown over the bar
	};

	struct CAGE_ENGINE_API GuiSliderBarComponent
	{
		Real value;
		Real min = 0, max = 1;
		bool vertical = false;
	};

	struct CAGE_ENGINE_API GuiColorPickerComponent
	{
		Vec3 color = Vec3(1, 0, 0);
		bool collapsible = false;
	};

	struct CAGE_ENGINE_API GuiPanelComponent
	{
		// GuiTextComponent defines caption
	};

	struct CAGE_ENGINE_API GuiSpoilerComponent
	{
		bool collapsed = true;
		bool collapsesSiblings = true;
		// GuiTextComponent defines caption
	};

	namespace privat
	{
		template<uint32 N>
		struct GuiStringLiteral
		{
			consteval GuiStringLiteral(const char(&str)[N]) noexcept
			{
				detail::memcpy(value, str, N);
			}
			char value[N];
		};

		CAGE_ENGINE_API GuiTooltipComponent::Tooltip guiTooltipText(const GuiTextComponent *txt);
	}

	namespace detail
	{
		CAGE_ENGINE_API void guiDestroyEntityRecursively(Entity *e);

		template<privat::GuiStringLiteral Text, uint32 AssetName = 0, uint32 TextName = 0>
		GuiTooltipComponent::Tooltip guiTooltipText() noexcept
		{
			static constexpr GuiTextComponent txt{ Text.value, AssetName, TextName };
			return privat::guiTooltipText(&txt);
		}
	}
}

#endif // guard_guiComponents_sdf1gh45hk485aws
