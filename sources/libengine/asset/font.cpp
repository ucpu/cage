#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context, void *schemePointer)
		{
			Font *font = nullptr;
			if (context->assetHolder)
			{
				font = static_cast<Font*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newFontFace().cast<void>();
				font = static_cast<Font*>(context->assetHolder.get());
				font->setDebugName(context->textName);
			}
			context->returnData = font;

			Deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
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
		}
	}

	AssetScheme genAssetSchemeFontFace(uint32 threadIndex, Window *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
