#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const assetContext *context, void *schemePointer)
		{
			fontFace *font = nullptr;
			if (context->assetHolder)
			{
				font = static_cast<fontFace*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newFontFace().cast<void>();
				font = static_cast<fontFace*>(context->assetHolder.get());
				font->setDebugName(context->textName);
			}
			context->returnData = font;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			fontFaceHeader data;
			des >> data;
			const void *image = des.advance(data.texSize);
			const void *glyphs = des.advance(sizeof(fontFaceHeader::glyphData) * data.glyphCount);
			const real *kerning = nullptr;
			if ((data.flags & fontFlags::Kerning) == fontFlags::Kerning)
				kerning = (const real*)des.advance(data.glyphCount * data.glyphCount * sizeof(real));
			const uint32 *charmapChars = (const uint32*)des.advance(sizeof(uint32) * data.charCount);
			const uint32 *charmapGlyphs = (const uint32*)des.advance(sizeof(uint32) * data.charCount);
			CAGE_ASSERT_RUNTIME(des.available() == 0);

			font->setLine(data.lineHeight, data.firstLineOffset);
			font->setImage(data.texWidth, data.texHeight, data.texSize, image);
			font->setGlyphs(data.glyphCount, glyphs, kerning);
			font->setCharmap(data.charCount, charmapChars, charmapGlyphs);
		}
	}

	assetScheme genAssetSchemeFontFace(uint32 threadIndex, windowHandle *memoryContext)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
