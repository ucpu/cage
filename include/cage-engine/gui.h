#ifndef guard_gui_h_D134A0110D344468B65B3F8D68E079CB
#define guard_gui_h_D134A0110D344468B65B3F8D68E079CB

#include "core.h"

namespace cage
{
	struct CAGE_ENGINE_API GuiParentComponent
	{
		uint32 parent;
		sint32 order;
		GuiParentComponent();
	};

	struct CAGE_ENGINE_API GuiImageComponent
	{
		uint64 animationStart; // -1 will be replaced by current time
		vec2 textureUvOffset;
		vec2 textureUvSize;
		uint32 textureName;
		GuiImageComponent();
	};

	enum class ImageModeEnum : uint32
	{
		Stretch,
		// todo
	};

	struct CAGE_ENGINE_API GuiImageFormatComponent
	{
		real animationSpeed;
		real animationOffset;
		ImageModeEnum mode;
		GuiImageFormatComponent();
	};

	struct CAGE_ENGINE_API GuiTextComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		GuiTextComponent();
	};

	struct CAGE_ENGINE_API GuiTextFormatComponent
	{
		vec3 color;
		uint32 font;
		real size;
		real lineSpacing;
		TextAlignEnum align;
		GuiTextFormatComponent();
	};

	struct CAGE_ENGINE_API GuiSelectionComponent
	{
		uint32 start; // unicode characters (not bytes)
		uint32 length; // unicode characters (not bytes)
		GuiSelectionComponent();
	};

	struct CAGE_ENGINE_API GuiTooltipComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		GuiTooltipComponent();
	};

	struct CAGE_ENGINE_API GuiWidgetStateComponent
	{
		uint32 skinIndex; // -1 = inherit
		bool disabled;
		GuiWidgetStateComponent();
	};

	struct CAGE_ENGINE_API GuiSelectedItemComponent
	{};

	enum class OverflowModeEnum : uint32
	{
		Auto, // show scrollbar when needed only
		Always, // always show scrollbar
		Never, // never show scrollbar
			   // overflowing content is hidden irrespective of this setting
	};

	struct CAGE_ENGINE_API GuiScrollbarsComponent
	{
		vec2 alignment; // 0.5 is center
		vec2 scroll;
		OverflowModeEnum overflow[2];
		GuiScrollbarsComponent();
	};

	struct CAGE_ENGINE_API GuiExplicitSizeComponent
	{
		vec2 size;
		GuiExplicitSizeComponent();
	};

	struct CAGE_ENGINE_API GuiEventComponent
	{
		Delegate<bool(uint32)> event;
		GuiEventComponent();
	};

#define GCHL_GUI_COMMON_COMPONENTS Parent, Image, ImageFormat, Text, TextFormat, Selection, Tooltip, WidgetState, SelectedItem, Scrollbars, ExplicitSize, Event

	namespace privat
	{
		struct CAGE_ENGINE_API GuiGeneralComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
			GuiGeneralComponents(EntityManager *ents);
		};
	}

	struct CAGE_ENGINE_API GuiLayoutLineComponent
	{
		bool vertical; // false -> items are next to each other, true -> items are above/under each other
		GuiLayoutLineComponent();
	};

	struct CAGE_ENGINE_API GuiLayoutTableComponent
	{
		uint32 sections; // set sections to 0 to make it square-ish
		bool grid; // false -> each column and row sizes are independent; true -> all columns and rows have same sizes
		bool vertical; // false -> fills entire row; true -> fills entire column
		GuiLayoutTableComponent();
	};

	// must have exactly two children
	struct CAGE_ENGINE_API GuiLayoutSplitterComponent
	{
		bool vertical; // false -> left sub-area, right sub-area; true -> top, bottom
		bool inverse; // false -> first item is fixed size, second item fills the remaining space; true -> second item is fixed size, first item fills the remaining space
		GuiLayoutSplitterComponent();
	};

#define GCHL_GUI_LAYOUT_COMPONENTS LayoutLine, LayoutTable, LayoutSplitter

	namespace privat
	{
		struct CAGE_ENGINE_API GuiLayoutsComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
			GuiLayoutsComponents(EntityManager *ents);
		};
	}

	struct CAGE_ENGINE_API GuiLabelComponent
	{
		// GuiTextComponent defines foreground of the widget
		// GuiImageComponent defines background of the widget
		GuiLabelComponent();
	};

	struct CAGE_ENGINE_API GuiButtonComponent
	{
		// GuiTextComponent defines foreground
		// GuiImageComponent defines background
		GuiButtonComponent();
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
		ReadOnly = 1 << 0,
		SelectAllOnFocusGain = 1 << 1,
		GoToEndOnFocusGain = 1 << 2,
		ShowArrowButtons = 1 << 3,
		AlwaysRoundValueToStep = 1 << 4,
		//WriteTabs = 1 << 5, // tab key will write tab rather than skip to next widget
	};

	struct CAGE_ENGINE_API GuiInputComponent
	{
		string value; // utf-8 encoded string (size is in bytes)
		union CAGE_ENGINE_API Union
		{
			real f;
			sint32 i;
			Union();
		} min, max, step;
		uint32 cursor; // unicode characters (not bytes)
		InputTypeEnum type;
		InputStyleFlags style;
		bool valid;
		// GuiTextComponent defines placeholder
		// GuiTextFormatComponent defines format
		// GuiSelectionComponent defines selected text
		GuiInputComponent();
	};

	struct CAGE_ENGINE_API GuiTextAreaComponent
	{
		MemoryBuffer *buffer; // utf-8 encoded string
		uint32 cursor; // unicode characters (not bytes)
		uint32 maxLength; // bytes
		InputStyleFlags style;
		// GuiSelectionComponent defines selected text
		GuiTextAreaComponent();
	};

	enum class CheckBoxStateEnum : uint32
	{
		Unchecked,
		Checked,
		Indeterminate,
	};

	struct CAGE_ENGINE_API GuiCheckBoxComponent
	{
		CheckBoxStateEnum state;
		// GuiTextComponent defines label shown next to the check box
		GuiCheckBoxComponent();
	};

	struct CAGE_ENGINE_API GuiRadioBoxComponent
	{
		uint32 group; // defines what other radio buttons are unchecked when this becomes checked
		CheckBoxStateEnum state;
		// GuiTextComponent defines label shown next to the radio box
		GuiRadioBoxComponent();
	};

	struct CAGE_ENGINE_API GuiComboBoxComponent
	{
		uint32 selected; // -1 = nothing selected
						 // GuiTextComponent defines placeholder
						 // children with GuiTextComponent defines individual lines
						 // GuiTextFormatComponent applies to all lines, may be overriden by individual childs
						 // GuiSelectedItemComponent on childs defines which line is selected (the selected property is authoritative)
		GuiComboBoxComponent();
	};

	struct CAGE_ENGINE_API GuiListBoxComponent
	{
		// real scrollbar;
		// children with GuiTextComponent defines individual lines
		// GuiTextFormatComponent applies to all lines, may be overriden by individual childs
		// GuiSelectedItemComponent on childs defines which lines are selected
		GuiListBoxComponent();
	};

	struct CAGE_ENGINE_API GuiProgressBarComponent
	{
		real progress; // 0 .. 1
		bool showValue; // overrides the text with the value (may use internationalization for formatting)
						// GuiTextComponent defines text shown over the bar
		GuiProgressBarComponent();
	};

	struct CAGE_ENGINE_API GuiSliderBarComponent
	{
		real value;
		real min, max;
		bool vertical;
		GuiSliderBarComponent();
	};

	struct CAGE_ENGINE_API GuiColorPickerComponent
	{
		vec3 color;
		bool collapsible;
		GuiColorPickerComponent();
	};

	struct CAGE_ENGINE_API GuiPanelComponent
	{
		GuiPanelComponent();
		// GuiTextComponent defines caption
		// GuiImageComponent defines background
	};

	struct CAGE_ENGINE_API GuiSpoilerComponent
	{
		bool collapsesSiblings;
		bool collapsed;
		GuiSpoilerComponent();
		// GuiTextComponent defines caption
		// GuiImageComponent defines background
	};

#define GCHL_GUI_WIDGET_COMPONENTS Label, Button, Input, TextArea, CheckBox, RadioBox, ComboBox, ListBox, ProgressBar, SliderBar, ColorPicker, Panel, Spoiler

	namespace privat
	{
		struct CAGE_ENGINE_API GuiWidgetsComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
			GuiWidgetsComponents(EntityManager *ents);
		};
	}

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

	namespace privat
	{
		struct CAGE_ENGINE_API GuiComponents : public privat::GuiGeneralComponents, public privat::GuiWidgetsComponents, public privat::GuiLayoutsComponents
		{
			GuiComponents(EntityManager* ents);
		};
	}

	class CAGE_ENGINE_API Gui : private Immovable
	{
	public:
		void graphicsInitialize();
		void graphicsFinalize();
		void graphicsRender();

		//void soundInitialize();
		//void soundFinalize();
		//void soundRender();

		void setOutputResolution(const ivec2 &resolution, real retina = 1); // resolution: pixels; retina: how many pixels per point (1D)
		void setZoom(real zoom); // pixels per point (1D)
		ivec2 getOutputResolution() const;
		real getOutputRetina() const;
		real getZoom() const;
		void setOutputSoundBus(MixingBus *bus);
		MixingBus *getOutputSoundBus() const;
		void setFocus(uint32 widget);
		uint32 getFocus() const;
		void controlUpdateStart(); // must be called before processing events
		void controlUpdateDone(); // accesses asset manager

		bool windowResize(const ivec2 &);
		bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseWheel(sint32 wheel, ModifiersFlags modifiers, const ivec2 &point);
		bool keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyChar(uint32 key);

		void handleWindowEvents(Window *window, sint32 order = 0);
		void skipAllEventsUntilNextUpdate();
		ivec2 getInputResolution() const;
		Delegate<bool(const ivec2&, vec2&)> eventCoordinatesTransformer; // called from controlUpdateStart or from any window events, it should return false to signal that the point is outside the gui, otherwise the point should be converted from window coordinate system to the gui output resolution coordinate system
		EventDispatcher<bool(uint32)> widgetEvent; // called from controlUpdateStart or window events

		GuiSkinConfig &skin(uint32 index = 0);
		const GuiSkinConfig &skin(uint32 index = 0) const;

		privat::GuiComponents &components();
		EntityManager *entities();
		AssetManager *assets();
	};

	struct CAGE_ENGINE_API GuiCreateConfig
	{
		AssetManager *assetMgr;
		EntityManagerCreateConfig *entitiesConfig;
		uintPtr itemsArenaSize;
		uintPtr emitArenaSize;
		uint32 skinsCount;
		GuiCreateConfig();
	};

	CAGE_ENGINE_API Holder<Gui> newGui(const GuiCreateConfig &config);

	namespace detail
	{
		CAGE_ENGINE_API Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution);
	}
}

#endif // guard_gui_h_D134A0110D344468B65B3F8D68E079CB
