namespace cage
{
	struct CAGE_API layoutLineComponent // items are placed in a single line
	{
		vec2 cellsAnchor; // center is at 0.5
		bool vertical; // false -> items are next to each other, true -> items are above/under each other
		bool expandToSameWidth;
		bool expandToSameHeight;
		bool addSpacingToFillArea;
		layoutLineComponent();
	};

	struct CAGE_API layoutTableComponent : public layoutLineComponent
	{
		// set sections to 0 to make it square-ish
		uint32 sections;
		bool grid; // false -> each column and row sizes are independent; true -> all columns and rows have same sizes
		layoutTableComponent();
	};

	struct CAGE_API layoutRadialComponent
	{
		layoutRadialComponent();
	};

	struct CAGE_API layoutSplitterComponent // splits the given area into two parts in a way that it is fully used
	{
		real anchor; // secondary axis alignment
		bool allowMasterResize;
		bool allowSlaveResize;
		bool vertical; // false -> left sub-area, right sub-area; true -> top, bottom
		bool inverse; // false -> first item is fixed size, second item fills the remaining space; true -> second item is fixed size, first item fills the remaining space
		layoutSplitterComponent();
	};

#define GCHL_GUI_LAYOUT_COMPONENTS layoutLine, layoutTable, layoutRadial, layoutSplitter

	struct CAGE_API layoutsComponentsStruct
	{
#define GCHL_GENERATE(T) componentClass *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		layoutsComponentsStruct(entityManagerClass *ents);
	};
}
