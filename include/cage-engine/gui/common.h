namespace cage
{
	struct CAGE_API GuiParentComponent
	{
		uint32 parent;
		sint32 order;
		GuiParentComponent();
	};

	struct CAGE_API GuiImageComponent
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

	struct CAGE_API GuiImageFormatComponent
	{
		real animationSpeed;
		real animationOffset;
		ImageModeEnum mode;
		GuiImageFormatComponent();
	};

	struct CAGE_API GuiTextComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		GuiTextComponent();
	};

	struct CAGE_API GuiTextFormatComponent
	{
		vec3 color;
		uint32 font;
		real size;
		real lineSpacing;
		TextAlignEnum align;
		GuiTextFormatComponent();
	};

	struct CAGE_API GuiSelectionComponent
	{
		uint32 start; // unicode characters (not bytes)
		uint32 length; // unicode characters (not bytes)
		GuiSelectionComponent();
	};

	struct CAGE_API GuiTooltipComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		GuiTooltipComponent();
	};

	struct CAGE_API GuiWidgetStateComponent
	{
		uint32 skinIndex; // -1 = inherit
		bool disabled;
		GuiWidgetStateComponent();
	};

	struct CAGE_API GuiSelectedItemComponent
	{};

	enum class OverflowModeEnum : uint32
	{
		Auto, // show scrollbar when needed only
		Always, // always show scrollbar
		Never, // never show scrollbar
		// overflowing content is hidden irrespective of this setting
	};

	struct CAGE_API GuiScrollbarsComponent
	{
		vec2 alignment; // 0.5 is center
		vec2 scroll;
		OverflowModeEnum overflow[2];
		GuiScrollbarsComponent();
	};

	struct CAGE_API GuiExplicitSizeComponent
	{
		vec2 size;
		GuiExplicitSizeComponent();
	};

	struct CAGE_API GuiEventComponent
	{
		Delegate<bool(uint32)> event;
		GuiEventComponent();
	};

#define GCHL_GUI_COMMON_COMPONENTS Parent, Image, ImageFormat, Text, TextFormat, Selection, Tooltip, WidgetState, SelectedItem, Scrollbars, ExplicitSize, Event

	namespace privat
	{
		struct CAGE_API GuiGeneralComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
			GuiGeneralComponents(EntityManager *ents);
		};
	}
}
