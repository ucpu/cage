namespace cage
{
	class CAGE_API fontClass
	{
	public:
		uint32 lineHeight;
		sint32 firstLineOffset;

		void setImage(uint16 width, uint16 height, uint32 size, void *data);
		void setGlyphs(uint32 count, void *data, sint8 *kerning);
		void setCharmap(uint32 count, uint32 *chars, uint32 *glyphs);

		void bind(meshClass *mesh, shaderClass *shader, uint32 screenWidth, uint32 screenHeight) const;

		struct CAGE_API formatStruct
		{
			textAlignEnum align;
			uint16 wrapWidth;
			sint16 lineSpacing;
			formatStruct();
		};

		void transcript(const string &text, uint32 *glyphs, uint32 &count);
		void transcript(const char *text, uint32 *glyphs, uint32 &count);

		void size(const uint32 *glyphs, uint32 count, const formatStruct &format, uint32 &width, uint32 &height);
		void size(const uint32 *glyphs, uint32 count, const formatStruct &format, uint32 &width, uint32 &height, const vec2 &mousePosition, uint32 &cursor);
		void render(const uint32 *glyphs, uint32 count, const formatStruct &format, sint16 posX, sint16 posY, const vec3 &color, uint32 cursor = -1);
	};

	CAGE_API holder<fontClass> newFont(windowClass *context);
}
