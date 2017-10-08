#include "privateControls.h"

namespace cage
{
	sliderControlCacheStruct::sliderControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::SliderVerticalPanel;
	}

	void sliderControlCacheStruct::graphicPrepare()
	{
		CAGE_ASSERT_RUNTIME(!firstChild);

		if (controlMode == -1)
			return;

		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);

		if (!verifyControlEntity())
			return;

		float f = value();
		vec2 cp = pixelsContentPosition;
		vec2 cs = pixelsContentSize;
		uint32 vertical = cs[1] > cs[0];
		elementType = vertical ? elementTypeEnum::SliderVerticalPanel : elementTypeEnum::SliderHorizontalPanel;
		vec2 outerSiz = vec2(cs[1 - vertical], cs[1 - vertical]);
		vec2 innerPos = cp + (vertical ? vec2(cs[0] * 0.5, (cs[1] - cs[0]) * (1 - f) + cs[0] * 0.5) : vec2((cs[0] - cs[1]) * f + cs[1] * 0.5, cs[1] * 0.5));
		vec2 outerPos = innerPos - outerSiz * 0.5;
		context->prepareElement(context->pixelsToNdc(outerPos, outerSiz), context->pixelsToNdc(innerPos, vec2()), vertical ? elementTypeEnum::SliderVerticalDot : elementTypeEnum::SliderHorizontalDot, controlMode);
	}

	float &sliderControlCacheStruct::value()
	{
		CAGE_ASSERT_RUNTIME(verifyControlEntity());
		GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
		return control.fval;
	}

	void sliderControlCacheStruct::updateByMouse()
	{
		vec2 cp = pixelsContentPosition;
		vec2 cs = pixelsContentSize;
		uint32 vertical = cs[1] > cs[0];
		real p = cp[vertical] + cs[1 - vertical] * 0.5, s = cs[vertical] - cs[1 - vertical];
		real c = context->mousePosition[vertical];
		float f = clamp((c - p) / s, 0.f, 1.f).value;
		value() = vertical ? 1 - f : f;
		context->genericEvent.dispatch(entity);
	}

	bool sliderControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
		{
			context->focusName = entity;
			updateByMouse();
		}
		return true;
	}

	bool sliderControlCacheStruct::mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (context->focusName == entity && (buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			updateByMouse();
		return true;
	}

	bool sliderControlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		static const real change = 0.05;
		switch (key)
		{
		case 263: // left
		case 264: // down
		{
			float &sel = value();
			sel = max((real)sel - change, 0).value;
			context->genericEvent.dispatch(entity);
		} break;
		case 262: // right
		case 265: // up
		{
			float &sel = value();
			sel = min((real)sel + change, 1).value;
			context->genericEvent.dispatch(entity);
		} break;
		}
		return true;
	}
}