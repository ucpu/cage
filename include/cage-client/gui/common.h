namespace cage
{
	struct CAGE_API parentComponent
	{
		uint32 parent;
		sint32 order;
		parentComponent();
	};

	struct CAGE_API imageComponent
	{
		uint64 animationStart; // -1 will be replaced by current time
		vec2 textureUvOffset;
		vec2 textureUvSize;
		uint32 textureName;
		imageComponent();
	};

	enum class imageModeEnum : uint32
	{
		Stretch,
		// todo
	};

	struct CAGE_API imageFormatComponent
	{
		real animationSpeed;
		real animationOffset;
		imageModeEnum mode;
		imageFormatComponent();
	};

	struct CAGE_API textComponent
	{
		string value; // list of parameters separated by '|' when formatted, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		textComponent();
	};

	struct CAGE_API textFormatComponent
	{
		vec3 color;
		uint32 font;
		real size;
		real lineSpacing;
		textAlignEnum align;
		textFormatComponent();
	};

	struct CAGE_API selectionComponent
	{
		uint32 start; // unicode characters (not bytes)
		uint32 length; // unicode characters (not bytes)
		selectionComponent();
	};

	typedef textComponent tooltipComponent;

	struct CAGE_API widgetStateComponent
	{
		uint32 skinIndex; // -1 = inherit
		bool disabled;
		widgetStateComponent();
	};

	struct CAGE_API selectedItemComponent
	{};

	enum class overflowModeEnum : uint32
	{
		Auto, // show scrollbar when needed only
		Always, // always show scrollbar
		Never, // never show scrollbar
		// overflowing content is hidden irrespective of this setting
	};

	struct CAGE_API scrollbarsComponent
	{
		vec2 alignment; // 0.5 is center
		vec2 scroll;
		overflowModeEnum overflow[2];
		scrollbarsComponent();
	};

	struct CAGE_API explicitSizeComponent
	{
		vec2 size;
		explicitSizeComponent();
	};

#define GCHL_GUI_COMMON_COMPONENTS parent, image, imageFormat, text, textFormat, selection, tooltip, widgetState, selectedItem, scrollbars, explicitSize

	struct CAGE_API guiGeneralComponents
	{
#define GCHL_GENERATE(T) entityComponent *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		guiGeneralComponents(entityManager *ents);
	};
}
