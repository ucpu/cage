#include "privateLayouts.h"

namespace cage
{
	manualLayoutCacheStruct::manualLayoutCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{}

	void manualLayoutCacheStruct::updatePositionSize()
	{
		updateContentAndNdcPositionSize();
		controlCacheStruct *c = firstChild;
		while (c)
		{
			vec2 cp, ca;
			if (c->entity)
			{
				entityClass *e = context->entityManager->getEntity(c->entity);
				if (e->hasComponent(context->components.position))
				{
					GUI_GET_COMPONENT(position, p, e);
					cp = vec2(
						context->evaluateUnit(p.x, p.xUnit, 0),
						context->evaluateUnit(p.y, p.yUnit, 0)
					);
					ca = vec2(p.anchorX, p.anchorY);
				}
			}
			c->pixelsEnvelopeSize = c->pixelsRequestedSize;
			c->pixelsEnvelopePosition = pixelsContentPosition + cp - ca * c->pixelsEnvelopeSize;
			c->updatePositionSize();
			c = c->nextSibling;
		}
	}

	void manualLayoutCacheStruct::updateRequestedSize()
	{
		pixelsRequestedSize = vec2();
		if (entity)
		{
			entityClass *e = context->entityManager->getEntity(entity);
			if (e->hasComponent(context->components.position))
			{
				GUI_GET_COMPONENT(position, p, e);
				pixelsRequestedSize = vec2(
					context->evaluateUnit(p.w, p.wUnit, 0),
					context->evaluateUnit(p.h, p.hUnit, 0)
				);
			}
		}

		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->updateRequestedSize();
			c = c->nextSibling;
		}
	}
}