namespace cage
{
	struct CAGE_API ParentComponent
	{
		uint32 parent;
		sint32 order;
		ParentComponent();
	};

	struct CAGE_API ImageComponent
	{
		uint64 animationStart; // -1 will be replaced by current time
		vec2 textureUvOffset;
		vec2 textureUvSize;
		uint32 textureName;
		ImageComponent();
	};

	enum class ImageModeEnum : uint32
	{
		Stretch,
		// todo
	};

	struct CAGE_API ImageFormatComponent
	{
		real animationSpeed;
		real animationOffset;
		ImageModeEnum mode;
		ImageFormatComponent();
	};

	struct CAGE_API TextComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		TextComponent();
	};

	struct CAGE_API TextFormatComponent
	{
		vec3 color;
		uint32 font;
		real size;
		real lineSpacing;
		TextAlignEnum align;
		TextFormatComponent();
	};

	struct CAGE_API SelectionComponent
	{
		uint32 start; // unicode characters (not bytes)
		uint32 length; // unicode characters (not bytes)
		SelectionComponent();
	};

	struct CAGE_API TooltipComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		TooltipComponent();
	};

	struct CAGE_API WidgetStateComponent
	{
		uint32 skinIndex; // -1 = inherit
		bool disabled;
		WidgetStateComponent();
	};

	struct CAGE_API SelectedItemComponent
	{};

	enum class OverflowModeEnum : uint32
	{
		Auto, // show scrollbar when needed only
		Always, // always show scrollbar
		Never, // never show scrollbar
		// overflowing content is hidden irrespective of this setting
	};

	struct CAGE_API ScrollbarsComponent
	{
		vec2 alignment; // 0.5 is center
		vec2 scroll;
		OverflowModeEnum overflow[2];
		ScrollbarsComponent();
	};

	struct CAGE_API ExplicitSizeComponent
	{
		vec2 size;
		ExplicitSizeComponent();
	};

	struct CAGE_API EventComponent
	{
		Delegate<bool(uint32)> event;
		EventComponent();
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
