namespace cage
{
	struct CAGE_API parentComponent
	{
		uint32 parent;
		sint32 ordering;
		parentComponent();
	};

	struct CAGE_API textComponent
	{
		uint32 assetName;
		uint32 textName;
		string value; // list of parameters separated by '|' when internationalized, otherwise the string as is
		textComponent();
	};

	struct CAGE_API imageComponent
	{
		uint64 animationStart;
		real tx, ty, tw, th;
		real animationSpeed;
		real animationOffset;
		uint32 textureName;
		imageComponent();
	};

	struct CAGE_API controlComponent
	{
		controlTypeEnum controlType;
		union
		{
			float fval;
			uint32 ival;
		};
		controlStateEnum state;
		controlComponent();
	};

	struct CAGE_API layoutComponent
	{
		layoutModeEnum layoutMode;
		scrollbarModeEnum hScrollbarMode, vScrollbarMode;
		real hScrollbar, vScrollbar;
		layoutComponent();
	};

	struct CAGE_API positionComponent
	{
		real anchorX, anchorY;
		real x, y;
		real w, h;
		unitsModeEnum xUnit, yUnit;
		unitsModeEnum wUnit, hUnit;
		positionComponent();
	};

	struct CAGE_API formatComponent
	{
		vec3 color;
		uint32 fontName;
		textAlignEnum align;
		formatComponent();
	};

	struct CAGE_API selectionComponent
	{
		uint16 selectionStart;
		uint16 selectionLength;
		selectionComponent();
	};

	struct CAGE_API componentsStruct
	{
		componentClass *parent;
		componentClass *text;
		componentClass *image;
		componentClass *control;
		componentClass *layout;
		componentClass *position;
		componentClass *format;
		componentClass *selection;
	};
}
