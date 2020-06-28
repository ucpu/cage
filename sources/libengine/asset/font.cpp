#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context)
		{
			Holder<Font> font = newFont();
			font->setDebugName(context->textName);

			Deserializer des(context->originalData());
			FontHeader data;
			des >> data;
			PointerRange<const char> image = des.advance(data.texSize);
			PointerRange<const char> glyphs = des.advance(sizeof(FontHeader::GlyphData) * data.glyphCount);
			PointerRange<const char> kerning;
			if ((data.flags & FontFlags::Kerning) == FontFlags::Kerning)
				kerning = des.advance(data.glyphCount * data.glyphCount * sizeof(real));
			PointerRange<const char> charmapChars = des.advance(sizeof(uint32) * data.charCount);
			PointerRange<const char> charmapGlyphs = des.advance(sizeof(uint32) * data.charCount);
			CAGE_ASSERT(des.available() == 0);

			font->setLine(data.lineHeight, data.firstLineOffset);
			font->setImage(data.texWidth, data.texHeight, image);
			font->setGlyphs(glyphs, bufferCast<const real>(kerning));
			font->setCharmap(bufferCast<const uint32>(charmapChars), bufferCast<const uint32>(charmapGlyphs));

			context->assetHolder = templates::move(font).cast<void>();
		}
	}

	AssetScheme genAssetSchemeFont(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
