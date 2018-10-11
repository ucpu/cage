#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "private.h"

namespace cage
{
	namespace
	{
		bool pointIsInside(guiItemStruct *b, const vec2 &p)
		{
			return pointInside(b->renderPos, b->renderSize, p);
		}

		void findWidgets(guiItemStruct *item, uint32 name, std::vector<widgetBaseStruct*> &result)
		{
			if (item->entity && item->entity->name() == name && item->widget)
				result.push_back(item->widget);
			guiItemStruct *c = item->firstChild;
			while (c)
			{
				findWidgets(c, name, result);
				c = c->nextSibling;
			}
		}

		std::vector<widgetBaseStruct*> focused(guiImpl *impl)
		{
			std::vector<widgetBaseStruct*> result;
			if (impl->focusName)
			{
				result.reserve(3);
				findWidgets(impl->root, impl->focusName, result);
			}
			return result;
		}

		template<class A, bool (widgetBaseStruct::*F)(A, modifiersFlags, vec2)>
		bool passMouseEvent(guiImpl *impl, A a, modifiersFlags m, const pointStruct &point)
		{
			vec2 pt;
			if (!impl->eventPoint(point, pt))
				return false;
			{ // first, pass the event to focused widget
				bool res = false;
				for (auto f : focused(impl))
				{
					if (f->widgetState.disabled)
						res = true;
					if ((f->*F)(a, m, pt))
						res = true;
				}
				if (res)
					return true;
			}
			// if nothing has focus, pass the event to anything under the cursor
			for (auto it : impl->mouseEventReceivers)
			{
				if (pointIsInside(it->base, pt))
				{
					if (it->widgetState.disabled)
						return true;
					if ((it->*F)(a, m, pt))
						return true;
				}
			}
			return false;
		}

		template<bool (widgetBaseStruct::*F)(uint32, uint32, modifiersFlags)>
		bool passKeyEvent(guiImpl *impl, uint32 a, uint32 b, modifiersFlags m)
		{
			vec2 dummy;
			if (!impl->eventPoint(impl->inputMouse, dummy))
				return false;
			bool res = false;
			for (auto f : focused(impl))
			{
				if (f->widgetState.disabled)
					res = true;
				if ((f->*F)(a, b, m))
					res = true;
			}
			return res;
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
		bool res = false;
		for (auto f : focused(impl))
		{
			if (f->widgetState.disabled)
				res = true;
			if (f->keyChar(key))
				res = true;
		}
		return res;
	}
}
