namespace cage
{
	enum class unitEnum : uint32
	{
		None, // automatic, inherited, defaulted or otherwise defined
		Points, // = pixels times scale
		Pixels,
		ScreenWidth,
		ScreenHeight,
		ScreenShorter,
		ScreenLonger,
	};

	template<uint32 N>
	struct valuesStruct
	{
		union
		{
			real values[N];
			typename vecN<N>::type value;
		};
		unitEnum units[N];
		static const uint32 Dimension = N;
		valuesStruct() : value(typename vecN<N>::type()) { for (uint32 i = 0; i < N; i++) { units[i] = unitEnum::None; } }
	};

	typedef valuesStruct<1> valueStruct;

	struct CAGE_API positionComponent
	{
		vec2 anchor; // center is at 0.5
		valuesStruct<2> position;
		valuesStruct<2> size;
		positionComponent();
	};

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

#define GCHL_GUI_COMMON_COMPONENTS position, parent, image, imageFormat, text, textFormat, selection, tooltip, widgetState, selectedItem, scrollbars

	struct CAGE_API generalComponentsStruct
	{
#define GCHL_GENERATE(T) componentClass *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		generalComponentsStruct(entityManagerClass *ents);
	};
}
