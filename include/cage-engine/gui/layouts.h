namespace cage
{
	struct CAGE_API GuiLayoutLineComponent
	{
		bool vertical; // false -> items are next to each other, true -> items are above/under each other
		GuiLayoutLineComponent();
	};

	struct CAGE_API GuiLayoutTableComponent
	{
		uint32 sections; // set sections to 0 to make it square-ish
		bool grid; // false -> each column and row sizes are independent; true -> all columns and rows have same sizes
		bool vertical; // false -> fills entire row; true -> fills entire column
		GuiLayoutTableComponent();
	};

	// must have exactly two children
	struct CAGE_API GuiLayoutSplitterComponent
	{
		bool vertical; // false -> left sub-area, right sub-area; true -> top, bottom
		bool inverse; // false -> first item is fixed size, second item fills the remaining space; true -> second item is fixed size, first item fills the remaining space
		GuiLayoutSplitterComponent();
	};

#define GCHL_GUI_LAYOUT_COMPONENTS LayoutLine, LayoutTable, LayoutSplitter

	namespace privat
	{
		struct CAGE_API GuiLayoutsComponents
		{
#define GCHL_GENERATE(T) EntityComponent *T;
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
			GuiLayoutsComponents(EntityManager *ents);
		};
	}
}
