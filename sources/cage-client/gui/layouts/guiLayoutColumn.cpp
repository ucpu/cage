#include "privateLayouts.h"

namespace cage
{
	columnLayoutCacheStruct::columnLayoutCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{}

	void columnLayoutCacheStruct::updatePositionSize()
	{
		updateContentAndNdcPositionSize();
		vec2 pos = pixelsContentPosition;
		controlCacheStruct *c = firstChild;
		while (c)
		{
			vec2 cs = c->pixelsRequestedSize;
			c->pixelsEnvelopeSize = vec2(pixelsContentSize[0], cs[1]);
			c->pixelsEnvelopePosition = pos;
			c->updatePositionSize();
			pos[1] += cs[1];
			c = c->nextSibling;
		}
	}

	void columnLayoutCacheStruct::updateRequestedSize()
	{
		pixelsRequestedSize = vec2();
		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->updateRequestedSize();
			vec2 cs = c->pixelsRequestedSize;
			pixelsRequestedSize = vec2(max(pixelsRequestedSize[0], cs[0]), pixelsRequestedSize[1] + cs[1]);
			c = c->nextSibling;
		}
	}
}