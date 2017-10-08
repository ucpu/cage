#include "privateLayouts.h"

namespace cage
{
	rowLayoutCacheStruct::rowLayoutCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{}

	void rowLayoutCacheStruct::updatePositionSize()
	{
		updateContentAndNdcPositionSize();
		vec2 pos = pixelsContentPosition;
		controlCacheStruct *c = firstChild;
		while (c)
		{
			vec2 cs = c->pixelsRequestedSize;
			c->pixelsEnvelopeSize = vec2(cs[0], pixelsContentSize[1]);
			c->pixelsEnvelopePosition = pos;
			c->updatePositionSize();
			pos[0] += cs[0];
			c = c->nextSibling;
		}
	}

	void rowLayoutCacheStruct::updateRequestedSize()
	{
		pixelsRequestedSize = vec2();
		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->updateRequestedSize();
			vec2 cs = c->pixelsRequestedSize;
			pixelsRequestedSize = vec2(pixelsRequestedSize[0] + cs[0], max(pixelsRequestedSize[1], cs[1]));
			c = c->nextSibling;
		}
	}
}