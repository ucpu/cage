#ifndef guard_guiComponents_sdf1gh45hk485aws
#define guard_guiComponents_sdf1gh45hk485aws

#include <cage-core/stringLiteral.h>
#include <cage-engine/inputs.h>

namespace cage
{
	struct MemoryBuffer;
	class Entity;

	namespace detail
	{
		CAGE_ENGINE_API extern uint32 GuiTextFontDefault;
		CAGE_ENGINE_API extern uint64 GuiTooltipDelayDefault;
	}

	struct CAGE_ENGINE_API GuiSkinIndex
	{
		constexpr GuiSkinIndex() = default;
		constexpr explicit GuiSkinIndex(uint32 index) : index(index) {};

		uint32 index = m; // -1 = inherit
	};

	constexpr GuiSkinIndex GuiSkinDefault = GuiSkinIndex(0);
	constexpr GuiSkinIndex GuiSkinTooltips = GuiSkinIndex(1);

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
		None,
		Stretch, // image is resized to fill the area
		Fill, // preserves aspect ratio, and is scaled to fill the entire area, it may be clipped
		Fit, // preserves aspect ratio, and is scaled to fit in its entirety into the area, there may be empty areas around
	};

	struct CAGE_ENGINE_API GuiImageFormatComponent
	{
		Real animationSpeed = Real::Nan();
		Real animationOffset = Real::Nan();
		ImageModeEnum mode = ImageModeEnum::None;
	};

	struct CAGE_ENGINE_API GuiTextComponent
	{
		String value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 textId = 0;
	};

	struct CAGE_ENGINE_API GuiTextFormatComponent
	{
		Vec3 color = Vec3::Nan();
		uint32 font = detail::GuiTextFontDefault;
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
		Vec4 accent = Vec4::Nan(); // sRGB, blending factor
		GuiSkinIndex skin; // inherit by default
		bool disabled = false;
	};

	struct CAGE_ENGINE_API GuiSelectedItemComponent
	{};

	struct CAGE_ENGINE_API GuiExplicitSizeComponent
	{
		Vec2 size = Vec2::Nan(); // use nan to preserve original size in that axis
	};

	struct CAGE_ENGINE_API GuiEventComponent
	{
		Delegate<bool(const GenericInput &)> event;
	};

	struct CAGE_ENGINE_API GuiUpdateComponent
	{
		// called periodically from the gui itself
		// useful for updating text, image, format, etc.
		// do NOT use for adding/removing entities
		Delegate<void(Entity *)> update;
	};

	enum class TooltipCloseConditionEnum : uint32
	{
		Instant, // closes as soon as cursor moves
		Modal, // acts as a modal window and is closed when the cursor moves far away from the tooltip
		Never, // the application is responsible for closing the tooltip (by removing the entity)
	};

	enum class TooltipPlacementEnum : uint32
	{
		InvokerCorner, // corner of the tooltip positioned at corner of the invoker
		CursorCorner, // corner of the tooltip positioned at the cursor
		InvokerCenter, // center of the tooltip positioned at center of the invoker
		CursorCenter, // center of the tooltip positioned at the cursor
		ScreenCenter, // center of the tooltip positioned at center of the screen
		Manual,
	};

	struct CAGE_ENGINE_API GuiTooltipConfig
	{
		Entity *invoker = nullptr; // the widget for which the tooltip is to be shown
		Entity *tooltip = nullptr; // entity automatically prepared by the GuiManager for the application to fill in
		Vec2 cursorPosition = Vec2::Nan();
		Vec2 invokerPosition = Vec2::Nan();
		Vec2 invokerSize = Vec2::Nan();
		mutable Real extensionBorder = 20;
		mutable TooltipCloseConditionEnum closeCondition = TooltipCloseConditionEnum::Instant;
		mutable TooltipPlacementEnum placement = TooltipPlacementEnum::InvokerCorner;
	};

	struct CAGE_ENGINE_API GuiTooltipComponent
	{
		using TooltipCallback = Delegate<void(const GuiTooltipConfig &)>;
		TooltipCallback tooltip;
		uint64 delay = detail::GuiTooltipDelayDefault; // duration to hold cursor over the widget before showing the tooltip
		bool enableForDisabled = false; // allow showing the tooltip even if the widget is disabled
	};

	enum class LineEdgeModeEnum : uint32
	{
		None, // all elements are stretched so that the first/last element touches the edge
		Flexible, // the first/last element only is stretched to fill all the space to the edge
		Spaced, // add spaces between elements so that the first/last element touches the edge
		Empty, // the space between first/last element and the edge is left empty
	};

	struct CAGE_ENGINE_API GuiLayoutLineComponent
	{
		Real crossAlign = Real::Nan(); // applied to items individually in the secondary axis, nan = fill the space, 0 = left/top, 1 = right/bottom
		LineEdgeModeEnum first = LineEdgeModeEnum::None;
		LineEdgeModeEnum last = LineEdgeModeEnum::None;
		bool vertical = false;
	};

	struct CAGE_ENGINE_API GuiLayoutSplitComponent
	{
		Real crossAlign = Real::Nan(); // applied to items individually in the secondary axis, nan = fill the space, 0 = left/top, 1 = right/bottom
		bool vertical = false;
	};

	/* TODO
	struct CAGE_ENGINE_API GuiLayoutFlowComponent
	{
		Real crossAlign = Real::Nan(); // applied to items individually in the secondary axis, nan = fill the space, 0 = left/top, 1 = right/bottom
		bool vertical = false;
	};
	*/

	struct CAGE_ENGINE_API GuiLayoutTableComponent
	{
		uint32 sections = 2; // set sections to 0 to make it square-ish
		bool grid = false; // false -> each column and row sizes are independent; true -> all cells have same size
		bool vertical = true;
	};

	struct CAGE_ENGINE_API GuiLayoutAlignmentComponent
	{
		Vec2 alignment = Vec2(0.5); // use nan to fill the area in the particular axis
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

	struct CAGE_ENGINE_API GuiHeaderComponent
	{
		// GuiTextComponent defines foreground of the widget
		// GuiImageComponent defines background of the widget
	};

	struct CAGE_ENGINE_API GuiSeparatorComponent
	{
		bool vertical = false;
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
		ShowArrowButtons = 1 << 1,
		AlwaysRoundValueToStep = 1 << 2,
		GoToEndOnFocusGain = 1 << 3,
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

	struct CAGE_ENGINE_API GuiSolidColorComponent
	{
		Vec3 color; // sRGB
	};

	struct CAGE_ENGINE_API GuiFrameComponent
	{};

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
		CAGE_ENGINE_API GuiTooltipComponent::TooltipCallback guiTooltipText(const GuiTextComponent *txt);
		CAGE_ENGINE_API GuiTooltipComponent::TooltipCallback guiTooltipText(Entity *e, uint32 textId, const String &txt);
	}

	namespace detail
	{
		CAGE_ENGINE_API void guiDestroyChildrenRecursively(Entity *e);
		CAGE_ENGINE_API void guiDestroyEntityRecursively(Entity *e);

		template<uint32 TextId, StringLiteral Text = "">
		GuiTooltipComponent::TooltipCallback guiTooltipText()
		{
			static constexpr GuiTextComponent txt{ Text.value, TextId };
			return privat::guiTooltipText(&txt);
		}
	}
}

#endif // guard_guiComponents_sdf1gh45hk485aws
