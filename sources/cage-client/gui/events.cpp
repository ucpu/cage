#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

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
		bool pointIsInside(guiItemStruct *b, vec2 p)
		{
			for (int a = 0; a < 2; a++)
			{
				if (p[a] < b->position[a])
					return false;
				if (p[a] > b->position[a] + b->size[a])
					return false;
			}
			return true;
		}

		template<class A, bool (widgetBaseStruct::*F)(A, modifiersFlags, vec2)>
		bool passMouseEvent(guiImpl *impl, A a, modifiersFlags m, const pointStruct &point)
		{
			vec2 pt;
			if (!impl->eventPoint(point, pt))
				return false;
			for (auto it : impl->mouseEventReceivers)
			{
				if (pointIsInside(it->base, pt))
				{
					if ((it->*F)(a, m, pt))
						return true;
				}
			}
			return false;
		}

		widgetBaseStruct *focused(guiImpl *impl)
		{
			return nullptr;
			// todo
		}

		template<bool (widgetBaseStruct::*F)(uint32, uint32, modifiersFlags)>
		bool passKeyEvent(guiImpl *impl, uint32 a, uint32 b, modifiersFlags m)
		{
			vec2 dummy;
			if (!impl->eventPoint(impl->inputMouse, dummy))
				return false;
			widgetBaseStruct *f = focused(impl);
			if (f)
				return (f->*F)(a, b, m);
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
		return passMouseEvent<mouseButtonsFlags, &widgetBaseStruct::mousePress>(impl, buttons, modifiers, point);
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
		return passMouseEvent<sint8, &widgetBaseStruct::mouseWheel>(impl, wheel, modifiers, point);
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
			return f->keyChar(key);
		return false;
	}
}
