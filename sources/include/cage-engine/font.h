#ifndef guard_font_h_oiu56trd4svdf5
#define guard_font_h_oiu56trd4svdf5

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API Font : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void setLine(real lineHeight, real firstLineOffset);
		void setImage(uint32 width, uint32 height, PointerRange<const char> buffer);
		void setGlyphs(PointerRange<const char> buffer, PointerRange<const real> kerning);
		void setCharmap(PointerRange<const uint32> chars, PointerRange<const uint32> glyphs);

		struct CAGE_ENGINE_API FormatStruct
		{
			real size = 13;
			real wrapWidth = real::Infinity();
			real lineSpacing = 0;
			TextAlignEnum align = TextAlignEnum::Left;
		};

		uint32 glyphsCount(const string &text) const;
		uint32 glyphsCount(const char *text) const;
		uint32 glyphsCount(PointerRange<const char> text) const;

		void transcript(const string &text, PointerRange<uint32> glyphs) const;
		void transcript(const char *text, PointerRange<uint32> glyphs) const;
		void transcript(PointerRange<const char> text, PointerRange<uint32> glyphs) const;

		vec2 size(PointerRange<const uint32> glyphs, const FormatStruct &format) const;
		vec2 size(PointerRange<const uint32> glyphs, const FormatStruct &format, const vec2 &mousePosition, uint32 &cursor) const;

		void bind(Model *model, ShaderProgram *shader);
		void render(PointerRange<const uint32> glyphs, const FormatStruct &format, uint32 cursor = m);
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetScheme genAssetSchemeFont(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexFont = 14;
}

#endif // guard_font_h_oiu56trd4svdf5
