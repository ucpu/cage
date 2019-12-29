namespace cage
{
	class CAGE_API Font : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void setLine(real lineHeight, real firstLineOffset);
		void setImage(uint32 width, uint32 height, uint32 size, const void *data);
		void setGlyphs(uint32 count, const void *data, const real *kerning);
		void setCharmap(uint32 count, const uint32 *chars, const uint32 *glyphs);

		struct CAGE_API FormatStruct
		{
			real size;
			real wrapWidth;
			real lineSpacing;
			TextAlignEnum align;
			FormatStruct();
		};

		void transcript(const string &text, uint32 *glyphs, uint32 &count);
		void transcript(const char *text, uint32 *glyphs, uint32 &count);

		void size(const uint32 *glyphs, uint32 count, const FormatStruct &format, vec2 &size);
		void size(const uint32 *glyphs, uint32 count, const FormatStruct &format, vec2 &size, const vec2 &mousePosition, uint32 &cursor);

		void bind(Mesh *mesh, ShaderProgram *shader) const;
		void render(const uint32 *glyphs, uint32 count, const FormatStruct &format, uint32 cursor = m);
	};

	CAGE_API Holder<Font> newFontFace();

	CAGE_API AssetScheme genAssetSchemeFontFace(uint32 threadIndex, Window *memoryContext);
	static const uint32 assetSchemeIndexFontFace = 16;
}
