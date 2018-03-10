#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "private.h"

namespace cage
{
	namespace
	{
		bool pointIsInside(guiItemStruct *b, const vec2 &p)
		{
			return pointInside(b->position, b->size, p);
		}

		guiItemStruct *findItem(guiItemStruct *item, uint32 name)
		{
			if (item->entity && item->entity->getName() == name)
				return item;
			guiItemStruct *c = item->firstChild;
			while (c)
			{
				guiItemStruct *r = findItem(c, name);
				if (r)
					return r;
				c = c->nextSibling;
			}
			return nullptr;
		}

		widgetBaseStruct *focused(guiImpl *impl)
		{
			if (impl->focusName)
			{
				guiItemStruct *item = findItem(impl->root, impl->focusName);
				if (item && item->widget)
					return item->widget;
			}
			return nullptr;
		}

		template<class A, bool (widgetBaseStruct::*F)(A, modifiersFlags, vec2)>
		bool passMouseEvent(guiImpl *impl, A a, modifiersFlags m, const pointStruct &point)
		{
			vec2 pt;
			if (!impl->eventPoint(point, pt))
				return false;
			widgetBaseStruct *f = focused(impl);
			if (f)
			{
				if (f->widgetState.disabled)
					return true;
				return (f->*F)(a, m, pt);
			}
			return false;
		}

		template<bool (widgetBaseStruct::*F)(uint32, uint32, modifiersFlags)>
		bool passKeyEvent(guiImpl *impl, uint32 a, uint32 b, modifiersFlags m)
		{
			vec2 dummy;
			if (!impl->eventPoint(impl->inputMouse, dummy))
				return false;
			widgetBaseStruct *f = focused(impl);
			if (f)
			{
				if (f->widgetState.disabled)
					return true;
				return (f->*F)(a, b, m);
			}
			return false;
		}
	}

	bool guiImpl::eventPoint(const pointStruct &ptIn, vec2 &ptOut)
	{
		inputMouse = ptIn;
		if (!eventsEnabled)
			return false;
		if (eventCoordinatesTransformer)
		{
			bool ret = eventCoordinatesTransformer(ptIn, ptOut);
			if (ret)
				outputMouse = ptOut;
			return ret;
		}
		outputMouse = ptOut = vec2(ptIn.x, ptIn.y) / pointsScale;
		return true;
	}

	bool guiClass::windowResize(const pointStruct &res)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->inputResolution = res;
		return false;
	}

	bool guiClass::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		vec2 pt;
		if (!impl->eventPoint(point, pt))
			return false;
		for (auto it : impl->mouseEventReceivers)
		{
			if (pointIsInside(it->base, pt))
			{
				if (it->widgetState.disabled)
					return true;
				if (it->mousePress(buttons, modifiers, pt))
					return true;
			}
		}
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			impl->focusName = 0;
		return false;
	}

	bool guiClass::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetBaseStruct::mouseDouble>(impl, buttons, modifiers, point);
	}

	bool guiClass::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetBaseStruct::mouseRelease>(impl, buttons, modifiers, point);
	}

	bool guiClass::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetBaseStruct::mouseMove>(impl, buttons, modifiers, point);
	}

	bool guiClass::mouseWheel(sint8 wheel, modifiersFlags modifiers, const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		vec2 pt;
		if (!impl->eventPoint(point, pt))
			return false;
		for (auto it : impl->mouseEventReceivers)
		{
			if (pointIsInside(it->base, pt))
			{
				if (it->widgetState.disabled)
					return true;
				if (it->mouseWheel(wheel, modifiers, pt))
					return true;
			}
		}
		return false;
	}

	bool guiClass::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetBaseStruct::keyPress>(impl, key, scanCode, modifiers);
	}

	bool guiClass::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetBaseStruct::keyRepeat>(impl, key, scanCode, modifiers);
	}

	bool guiClass::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetBaseStruct::keyRelease>(impl, key, scanCode, modifiers);
	}

	bool guiClass::keyChar(uint32 key)
	{
		guiImpl *impl = (guiImpl*)this;
		vec2 dummy;
		if (!impl->eventPoint(impl->inputMouse, dummy))
			return false;
		widgetBaseStruct *f = focused(impl);
		if (f)
		{
			if (f->widgetState.disabled)
				return true;
			return f->keyChar(key);
		}
		return false;
	}
}
