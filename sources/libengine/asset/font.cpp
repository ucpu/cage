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
			const void *Image = des.advance(data.texSize);
			const void *glyphs = des.advance(sizeof(FontHeader::GlyphData) * data.glyphCount);
			const real *kerning = nullptr;
			if ((data.flags & FontFlags::Kerning) == FontFlags::Kerning)
				kerning = (const real*)des.advance(data.glyphCount * data.glyphCount * sizeof(real));
			const uint32 *charmapChars = (const uint32*)des.advance(sizeof(uint32) * data.charCount);
			const uint32 *charmapGlyphs = (const uint32*)des.advance(sizeof(uint32) * data.charCount);
			CAGE_ASSERT(des.available() == 0);

			font->setLine(data.lineHeight, data.firstLineOffset);
			font->setImage(data.texWidth, data.texHeight, data.texSize, Image);
			font->setGlyphs(data.glyphCount, glyphs, kerning);
			font->setCharmap(data.charCount, charmapChars, charmapGlyphs);

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
