#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
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
		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			fontClass *font = nullptr;
			if (context->assetHolder)
			{
				font = static_cast<fontClass*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newFont().cast<void>();
				font = static_cast<fontClass*>(context->assetHolder.get());
				font->setDebugName(context->textName);
			}
			context->returnData = font;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			fontHeaderStruct data;
			des >> data;
			const void *image = des.advance(data.texSize);
			const void *glyphs = des.advance(sizeof(fontHeaderStruct::glyphDataStruct) * data.glyphCount);
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

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeFont(uint32 threadIndex, windowClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		return s;
	}
}
