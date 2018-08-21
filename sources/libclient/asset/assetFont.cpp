#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/utility/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/assetStructs.h>

#include <cage-core/utility/pointer.h>

namespace cage
{
	namespace
	{
		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			windowClass *gm = (windowClass *)schemePointer;

			fontClass *font = nullptr;
			if (context->assetHolder)
			{
				font = static_cast<fontClass*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newFont(gm).transfev();
				font = static_cast<fontClass*>(context->assetHolder.get());
			}
			context->returnData = font;

			deserializer des(context->originalData, context->originalSize);
			fontHeaderStruct data;
			des >> data;
			const char *image = des.access(data.texSize);
			const char *glyphs = des.access(sizeof(fontHeaderStruct::glyphDataStruct) * data.glyphCount);
			const real *kerning = nullptr;
			if ((data.flags & fontFlags::Kerning) == fontFlags::Kerning)
				kerning = (const real*)des.access(data.glyphCount * data.glyphCount * sizeof(real));
			const uint32 *charmapChars = (const uint32*)des.access(sizeof(uint32) * data.charCount);
			const uint32 *charmapGlyphs = (const uint32*)des.access(sizeof(uint32) * data.charCount);
			CAGE_ASSERT_RUNTIME(des.current == des.end, des.current - des.end);

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
