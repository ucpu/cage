#ifndef guard_font_h_oiu56trd4svdf5
#define guard_font_h_oiu56trd4svdf5

#include <cage-engine/core.h>

namespace cage
{
	class GraphicsEncoder;
	class GraphicsAggregateBuffer;
	class AssetsOnDemand;

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

	struct CAGE_ENGINE_API FontRenderConfig : private Noncopyable
	{
		Mat4 transform = {};
		Vec4 color = Vec4(1);
		GraphicsEncoder *encoder = nullptr;
		GraphicsAggregateBuffer *aggregate = nullptr;
		AssetsOnDemand *assets = nullptr;
		bool depthTest = true;
		bool guiShader = true;
	};

	class CAGE_ENGINE_API Font : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		void importBuffer(PointerRange<const char> buffer);

		FontLayoutResult layout(PointerRange<const char> text, const FontFormat &format, Vec2 cursorPoint) const;
		FontLayoutResult layout(PointerRange<const char> text, const FontFormat &format, uint32 cursorIndexUtf32 = m) const;
		FontLayoutResult layout(PointerRange<const uint32> text, const FontFormat &format, Vec2 cursorPoint) const;
		FontLayoutResult layout(PointerRange<const uint32> text, const FontFormat &format, uint32 cursorIndexUtf32 = m) const;

		void render(const FontLayoutResult &layout, const FontRenderConfig &config) const;
	};

	CAGE_ENGINE_API Holder<Font> newFont(const AssetLabel &label);
}

#endif // guard_font_h_oiu56trd4svdf5
