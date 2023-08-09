#ifndef guard_font_h_oiu56trd4svdf5
#define guard_font_h_oiu56trd4svdf5

#include <cage-engine/core.h>

namespace cage
{
	class RenderQueue;
	class Model;
	class ShaderProgram;

	struct CAGE_ENGINE_API FontFormat
	{
		Real size = 13;
		Real wrapWidth = Real::Infinity();
		Real lineSpacing = 1;
		TextAlignEnum align = TextAlignEnum::Left;
	};

	class CAGE_ENGINE_API Font : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		void setLine(Real lineHeight, Real firstLineOffset);
		void setImage(Vec2i resolution, PointerRange<const char> buffer);
		void setGlyphs(PointerRange<const char> buffer, PointerRange<const Real> kerning);
		void setCharmap(PointerRange<const uint32> chars, PointerRange<const uint32> glyphs);

		uint32 glyphsCount(const String &text) const;
		uint32 glyphsCount(const char *text) const;
		uint32 glyphsCount(PointerRange<const char> text) const;

		void transcript(const String &text, PointerRange<uint32> glyphs) const;
		void transcript(const char *text, PointerRange<uint32> glyphs) const;
		void transcript(PointerRange<const char> text, PointerRange<uint32> glyphs) const;
		Holder<PointerRange<uint32>> transcript(const String &text) const;
		Holder<PointerRange<uint32>> transcript(const char *text) const;
		Holder<PointerRange<uint32>> transcript(PointerRange<const char> text) const;

		Vec2 size(PointerRange<const uint32> glyphs, const FontFormat &format) const;
		Vec2 size(PointerRange<const uint32> glyphs, const FontFormat &format, const Vec2 &mousePosition, uint32 &cursor) const;

		void render(RenderQueue *queue, const Holder<Model> &model, const Holder<ShaderProgram> &shader, PointerRange<const uint32> glyphs, const FontFormat &format, uint32 cursor = m) const;
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetScheme genAssetSchemeFont(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexFont = 14;
}

#endif // guard_font_h_oiu56trd4svdf5
