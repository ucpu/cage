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

	template<uint32 N> struct valuesStruct
	{
		union
		{
			real values[N];
			typename vecN<N>::type value;
		};
		unitEnum units[N];
		static const uint32 Dimension = N;
		valuesStruct() : value(vecN<N>::type()) { for (uint32 i = 0; i < N; i++) { units[i] = unitEnum::None; } }
	};

	template<> struct valuesStruct<1>
	{
		union
		{
			real values[1];
			real value;
		};
		union
		{
			unitEnum units[1];
			unitEnum unit;
		};
		valuesStruct() : value(0), unit(unitEnum::None) {}
	};

	typedef valuesStruct<1> valueStruct;

	template struct CAGE_API valuesStruct<1>;
	template struct CAGE_API valuesStruct<2>;
	template struct CAGE_API valuesStruct<3>;
	template struct CAGE_API valuesStruct<4>;

	struct CAGE_API parentComponent
	{
		uint32 parent;
		sint32 order;
		parentComponent();
	};

	struct CAGE_API imageComponent
	{
		uint64 animationStart; // -1 will be replaced by current time
		uint32 textureName;
		vec2 textureUvOffset;
		vec2 textureUvSize;
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
		string value; // list of parameters separated by '|' when internationalized, otherwise the string as is
		uint32 assetName;
		uint32 textName;
		textComponent();
	};

	struct CAGE_API textFormatComponent
	{
		vec3 color;
		uint32 fontName;
		textAlignEnum align;
		sint16 lineSpacing;
		textFormatComponent();
	};

	struct CAGE_API selectionComponent
	{
		uint32 start; // utf-32 characters
		uint32 length; // utf-32 characters
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

	struct CAGE_API explicitPositionComponent
	{
		vec2 anchor; // center is at 0.5
		valuesStruct<2> position;
		valuesStruct<2> size;
		explicitPositionComponent();
	};

	struct CAGE_API graphPointComponent
	{
		vec3 color;
		vec2 position;
		bool separate; // makes this point first in a new sequance of lines
		graphPointComponent();
	};

#define GCHL_GUI_COMMON_COMPONENTS parent, image, imageFormat, text, textFormat, selection, tooltip, widgetState, selectedItem, explicitPosition, graphPoint

	struct CAGE_API generalComponentsStruct
	{
#define GCHL_GENERATE(T) componentClass *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
#undef GCHL_GENERATE
		generalComponentsStruct(entityManagerClass *ents);
	};
}
