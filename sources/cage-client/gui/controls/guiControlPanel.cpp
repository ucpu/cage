#include "privateControls.h"

namespace cage
{
	panelControlCacheStruct::panelControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::Panel;
	}

	void panelControlCacheStruct::updatePositionSize()
	{
		updateContentAndNdcPositionSize();
		if (firstChild)
		{
			vec2 cp, ca;
			if (firstChild->entity)
			{
				entityClass *e = context->entityManager->getEntity(firstChild->entity);
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
			firstChild->pixelsEnvelopeSize = firstChild->pixelsRequestedSize;
			firstChild->pixelsEnvelopePosition = pixelsContentPosition + cp - ca * firstChild->pixelsEnvelopeSize;
			firstChild->updatePositionSize();
		}
	}

	void panelControlCacheStruct::updateRequestedSize()
	{
		if (firstChild)
		{
			CAGE_ASSERT_RUNTIME(!firstChild->nextSibling, "panel may not have more than one child");
			firstChild->updateRequestedSize();
			pixelsRequestedSize = firstChild->pixelsRequestedSize;
			vec2 dummy;
			context->contentToEnvelope(dummy, pixelsRequestedSize, elementType);
		}
		else
			pixelsRequestedSize = vec2();

		if (entity)
		{
			entityClass *e = context->entityManager->getEntity(entity);
			if (e->hasComponent(context->components.position))
			{
				GUI_GET_COMPONENT(position, p, e);
				pixelsRequestedSize = vec2(
					context->evaluateUnit(p.w, p.wUnit, pixelsRequestedSize[0]),
					context->evaluateUnit(p.h, p.hUnit, pixelsRequestedSize[1])
				);
			}
		}
	}

	void panelControlCacheStruct::graphicPrepare()
	{
		CAGE_ASSERT_RUNTIME(!firstItem || (firstItem->type == itemCacheStruct::itImage && !firstItem->next));
		if (controlMode == -1)
			return;
		controlCacheStruct::graphicPrepare();
		if (firstItem)
			context->prepareImage(firstItem, ndcInnerPositionSize);
		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->graphicPrepare();
			c = c->nextSibling;
		}
	}
}