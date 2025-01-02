#ifndef guard_font_h_oiu56trd4svdf5
#define guard_font_h_oiu56trd4svdf5

#include <cage-engine/core.h>

namespace cage
{
	class AssetsOnDemand;
	class RenderQueue;

	struct CAGE_ENGINE_API FontFormat
	{
		Real size = 13;
		Real wrapWidth = Real::Infinity();
		Real lineSpacing = 1;
		TextAlignEnum align = TextAlignEnum::Left;
	};

	struct CAGE_ENGINE_API FontLayoutGlyph
	{
		Vec4 wrld;
		uint32 index = m; // glyph info
		uint32 cluster = m; // index in utf32 encoded input string
	};

	struct CAGE_ENGINE_API FontLayoutResult : private Noncopyable
	{
		Holder<PointerRange<FontLayoutGlyph>> glyphs;
		Vec2 size;
		uint32 cursor = m; // index in utf32 encoded input string
	};

	class CAGE_ENGINE_API Font : private Immovable
	{
	protected:
		detail::StringBase<64> debugName;

	public:
		void setDebugName(const String &name);

		void importBuffer(PointerRange<const char> buffer);

		FontLayoutResult layout(PointerRange<const char> text, const FontFormat &format, Vec2 cursorPoint) const;
		FontLayoutResult layout(PointerRange<const char> text, const FontFormat &format, uint32 cursorIndexUtf32 = m) const;
		FontLayoutResult layout(PointerRange<const uint32> text, const FontFormat &format, Vec2 cursorPoint) const;
		FontLayoutResult layout(PointerRange<const uint32> text, const FontFormat &format, uint32 cursorIndexUtf32 = m) const;

		void render(RenderQueue *queue, AssetsOnDemand *assets, const FontLayoutResult &layout) const;
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetsScheme genAssetSchemeFont();
	constexpr uint32 AssetSchemeIndexFont = 14;
}

#endif // guard_font_h_oiu56trd4svdf5
