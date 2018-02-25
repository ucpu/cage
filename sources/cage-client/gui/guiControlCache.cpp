#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include "private.h"

namespace cage
{
	itemCacheStruct::itemCacheStruct(guiImpl *context, itemTypeEnum type, uint32 entity) :
		type(type), entity(entity), next(nullptr)
	{
		switch (type)
		{
		case itText:
			color[0] = context->defaultFontColor[0].value;
			color[1] = context->defaultFontColor[1].value;
			color[2] = context->defaultFontColor[2].value;
			font = context->defaultFontName && context->assetManager->ready(context->defaultFontName) ? context->assetManager->get<assetSchemeIndexFont, fontClass>(context->defaultFontName) : nullptr;
			size = 14;
			align = textAlignEnum::Left;
			lineSpacing = 0;
			translatedText = nullptr;
			translatedLength = 0;
			break;
		case itImage:
			uvClip[0] = uvClip[1] = uvClip[2] = uvClip[3] = 0;
			texture = nullptr;
			animationStart = 0;
			animationSpeed = 1;
			animationOffset = 0;
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalit item type");
		}
	}

	controlCacheStruct::controlCacheStruct(guiImpl *context, uint32 entity) :
		context(context), entity(entity), controlMode((controlModeEnum)-1),
		parent(nullptr), nextSibling(nullptr), prevSibling(nullptr), firstChild(nullptr), lastChild(nullptr), hScrollbar(nullptr), vScrollbar(nullptr), firstItem(nullptr),
		elementType((elementTypeEnum)-1)
	{
		if (entity)
		{
			entityClass *e = context->entityManager->getEntity(entity);
			if (e->hasComponent(context->components.control))
			{
				GUI_GET_COMPONENT(control, c, e);
				switch (c.state)
				{
				case controlStateEnum::Normal: controlMode = cmNormal; break;
				case controlStateEnum::Disabled: controlMode = cmDisabled; break;
				case controlStateEnum::Hidden: controlMode = cmHidden; break;
				default: CAGE_THROW_CRITICAL(exception, "invalid control state");
				}
			}
		}
	}

	bool itemCacheStruct::verifyItemEntity(guiImpl *context)
	{
		return entity && context->entityManager->hasEntity(entity) && context->entityManager->getEntity(entity)->hasComponent(type == itText ? context->components.text : context->components.image);
	}

	bool controlCacheStruct::verifyControlEntity()
	{
		return entity && context->entityManager->hasEntity(entity) && context->entityManager->getEntity(entity)->hasComponent(context->components.control);
	}

	bool controlCacheStruct::isPointInside(vec2 pixelsPoint)
	{
		vec2 p = pixelsContentPosition;
		vec2 s = pixelsContentSize;
		context->contentToOuter(p, s, elementType);
		pixelsPoint -= p;
		if (pixelsPoint[0] < 0 || pixelsPoint[1] < 0)
			return false;
		if (pixelsPoint[0] >= s[0] || pixelsPoint[1] >= s[1])
			return false;
		return true;
	}

	void controlCacheStruct::graphicPrepare()
	{
		if (elementType != elementTypeEnum::InvalidElement)
		{
			if (controlMode == cmHidden)
				return;
			context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);
		}

		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->graphicPrepare();
			c = c->nextSibling;
		}
	}

	void controlCacheStruct::updatePositionSize()
	{
		CAGE_ASSERT_RUNTIME(elementType != elementTypeEnum::InvalidElement, "this method must be implemented in all loayouts");
		CAGE_ASSERT_RUNTIME(firstChild == nullptr, "control must not have children");
		updateContentAndNdcPositionSize();
	}

	void controlCacheStruct::updateRequestedSize()
	{
		CAGE_ASSERT_RUNTIME(elementType < elementTypeEnum::TotalElements, elementType, elementTypeEnum::TotalElements);
		elementLayoutStruct &layout = context->elements[(uint32)elementType];
		vec2 defaul = context->evaluateUnit(layout.defaultSize, layout.defaultSizeUnits);

		if (entity && context->entityManager->getEntity(entity)->hasComponent(context->components.position))
		{
			GUI_GET_COMPONENT(position, p, context->entityManager->getEntity(entity));
			pixelsRequestedSize = vec2(
				context->evaluateUnit(p.w, p.wUnit, defaul[0]),
				context->evaluateUnit(p.h, p.hUnit, defaul[1])
			);
		}
		else
			pixelsRequestedSize = defaul;

		controlCacheStruct *c = firstChild;
		while (c)
		{
			c->updateRequestedSize();
			c = c->nextSibling;
		}
	}

	void controlCacheStruct::updateContentAndNdcPositionSize()
	{
		if (elementType == elementTypeEnum::InvalidElement)
		{
			pixelsContentPosition = pixelsEnvelopePosition;
			pixelsContentSize = pixelsEnvelopeSize;
			ndcInnerPositionSize = ndcOuterPositionSize = context->pixelsToNdc(pixelsContentPosition, pixelsContentSize);
		}
		else
		{
			vec2 pos = pixelsEnvelopePosition;
			vec2 siz = pixelsEnvelopeSize;
			context->envelopeToContent(pos, siz, elementType);
			pixelsContentPosition = pos;
			pixelsContentSize = siz;
			context->contentToInner(pos, siz, elementType);
			ndcInnerPositionSize = context->pixelsToNdc(pos, siz);
			pos = pixelsContentPosition;
			siz = pixelsContentSize;
			context->contentToOuter(pos, siz, elementType);
			ndcOuterPositionSize = context->pixelsToNdc(pos, siz);
		}
	}

	bool controlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left) context->focusName = 0; return true; };
	bool controlCacheStruct::mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { return true; };
	bool controlCacheStruct::mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { return true; };
	bool controlCacheStruct::mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point) { return true; };
	bool controlCacheStruct::keyPress(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return true; };
	bool controlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return true; };
	bool controlCacheStruct::keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return true; };
	bool controlCacheStruct::keyChar(windowClass *win, uint32 key) { return true; };
}