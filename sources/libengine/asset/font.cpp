#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/typeIndex.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<Font> font = newFont();
			font->setDebugName(context->textName);

			Deserializer des(context->originalData);
			FontHeader data;
			des >> data;
			PointerRange<const char> image = des.read(data.texSize);
			PointerRange<const char> glyphs = des.read(sizeof(FontHeader::GlyphData) * data.glyphCount);
			PointerRange<const char> kerning;
			if ((data.flags & FontFlags::Kerning) == FontFlags::Kerning)
				kerning = des.read(data.glyphCount * data.glyphCount * sizeof(real));
			PointerRange<const char> charmapChars = des.read(sizeof(uint32) * data.charCount);
			PointerRange<const char> charmapGlyphs = des.read(sizeof(uint32) * data.charCount);
			CAGE_ASSERT(des.available() == 0);

			font->setLine(data.lineHeight, data.firstLineOffset);
			font->setImage(data.texWidth, data.texHeight, image);
			font->setGlyphs(glyphs, bufferCast<const real>(kerning));
			font->setCharmap(bufferCast<const uint32>(charmapChars), bufferCast<const uint32>(charmapGlyphs));

			context->assetHolder = std::move(font).cast<void>();
		}
	}

	AssetScheme genAssetSchemeFont(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeIndex = detail::typeIndex<Font>();
		return s;
	}
}
