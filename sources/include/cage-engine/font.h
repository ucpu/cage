#ifndef guard_font_h_oiu56trd4svdf5
#define guard_font_h_oiu56trd4svdf5

#include <cage-engine/core.h>

namespace cage
{
	class RenderQueue;
	class Model;

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
		uint32 index = 0;
	};

	struct CAGE_ENGINE_API FontLayoutResult : private Noncopyable
	{
		Holder<PointerRange<FontLayoutGlyph>> glyphs;
		Vec2 size;
	};

	class CAGE_ENGINE_API Font : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		void importBuffer(PointerRange<const char> buffer);

		FontLayoutResult layout(PointerRange<const char> text, const FontFormat &format) const;

		void render(RenderQueue *queue, const Holder<Model> &model, const FontLayoutResult &layout) const;
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetsScheme genAssetSchemeFont(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexFont = 14;
}

#endif // guard_font_h_oiu56trd4svdf5
