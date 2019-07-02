namespace cage
{
	struct CAGE_API layoutLineComponent
	{
		bool vertical; // false -> items are next to each other, true -> items are above/under each other
		layoutLineComponent();
	};

	struct CAGE_API layoutTableComponent
	{
		uint32 sections; // set sections to 0 to make it square-ish
		bool grid; // false -> each column and row sizes are independent; true -> all columns and rows have same sizes
		bool vertical; // false -> fills entire row; true -> fills entire column
		layoutTableComponent();
	};

	// must have exactly two children
	struct CAGE_API layoutSplitterComponent
	{
		bool vertical; // false -> left sub-area, right sub-area; true -> top, bottom
		bool inverse; // false -> first item is fixed size, second item fills the remaining space; true -> second item is fixed size, first item fills the remaining space
		layoutSplitterComponent();
	};

#define GCHL_GUI_LAYOUT_COMPONENTS layoutLine, layoutTable, layoutSplitter

	struct CAGE_API layoutsComponentsStruct
	{
#define GCHL_GENERATE(T) entityComponent *T;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		layoutsComponentsStruct(entityManager *ents);
	};
}
