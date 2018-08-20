#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
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

			fontHeaderStruct *data = (fontHeaderStruct*)context->originalData;
			pointer ptr(context->originalData);
			ptr += sizeof(fontHeaderStruct);
			void *image = ptr;
			ptr += data->texSize;
			void *glyphs = ptr;
			ptr += sizeof(fontHeaderStruct::glyphDataStruct) * data->glyphCount;
			void *kerning = nullptr;
			if ((data->flags & fontFlags::Kerning) == fontFlags::Kerning)
			{
				kerning = ptr;
				ptr += data->glyphCount * data->glyphCount * sizeof(real);
			}
			void *charmapChars = ptr;
			ptr += sizeof(uint32) * data->charCount;
			void *charmapGlyphs = ptr;
			ptr += sizeof(uint32) * data->charCount;
			CAGE_ASSERT_RUNTIME(ptr == pointer(context->originalData) + (uintPtr)context->originalSize, ptr, context->originalData, context->originalSize);

			font->setLine(data->lineHeight, data->firstLineOffset);
			font->setImage(data->texWidth, data->texHeight, data->texSize, image);
			font->setGlyphs(data->glyphCount, glyphs, (real*)kerning);
			font->setCharmap(data->charCount, (uint32*)charmapChars, (uint32*)charmapGlyphs);
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
		s.load.bind <&processLoad>();
		s.done.bind <&processDone>();
		return s;
	}
}
