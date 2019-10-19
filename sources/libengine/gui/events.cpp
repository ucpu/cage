#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "private.h"

namespace cage
{
	eventReceiverStruct::eventReceiverStruct() : widget(nullptr), mask(1)
	{}

	bool eventReceiverStruct::pointInside(vec2 point, uint32 maskRequest) const
	{
		if ((mask & maskRequest) == 0)
			return false;
		return cage::pointInside(pos, size, point);
	}

	namespace
	{
		void findWidgets(hierarchyItemStruct *item, uint32 name, std::vector<widgetItemStruct*> &result)
		{
			{
				widgetItemStruct *w = nullptr;
				if (item->ent && item->ent->name() == name && (w = dynamic_cast<widgetItemStruct*>(item->item)))
					result.push_back(w);
			}
			hierarchyItemStruct *c = item->firstChild;
			while (c)
			{
				findWidgets(c, name, result);
				c = c->nextSibling;
			}
		}

		std::vector<widgetItemStruct*> focused(guiImpl *impl)
		{
			std::vector<widgetItemStruct*> result;
			if (impl->focusName)
			{
				result.reserve(3);
				findWidgets(impl->root, impl->focusName, result);
			}
			return result;
		}

		template<class A, bool (widgetItemStruct::*F)(A, modifiersFlags, vec2)>
		bool passMouseEvent(guiImpl *impl, A a, modifiersFlags m, const ivec2 &point)
		{
			vec2 pt;
			if (!impl->eventPoint(point, pt))
				return false;
			{ // first, pass the event to focused widget
				bool res = false;
				for (auto f : focused(impl))
				{
					if (f->widgetState.disabled || (f->*F)(a, m, pt))
						res = true;
				}
				if (res)
					return true;
			}
			// if nothing has focus, pass the event to anything under the cursor
			for (const auto &it : impl->mouseEventReceivers)
			{
				if (it.pointInside(pt))
				{
					if (it.widget->widgetState.disabled || (it.widget->*F)(a, m, pt))
						return true;
				}
			}
			return false;
		}

		template<bool (widgetItemStruct::*F)(uint32, uint32, modifiersFlags)>
		bool passKeyEvent(guiImpl *impl, uint32 a, uint32 b, modifiersFlags m)
		{
			vec2 dummy;
			if (!impl->eventPoint(impl->inputMouse, dummy))
				return false;
			bool res = false;
			for (auto f : focused(impl))
			{
				if (f->widgetState.disabled || (f->*F)(a, b, m))
					res = true;
			}
			return res;
		}
	}

	bool guiImpl::eventPoint(const ivec2 &ptIn, vec2 &ptOut)
	{
		inputMouse = ptIn;
		if (!eventsEnabled)
			return false;
		if (eventCoordinatesTransformer)
		{
			bool ret = eventCoordinatesTransformer(ptIn, ptOut);
			if (ret)
				outputMouse = ptOut / pointsScale;
			return ret;
		}
		outputMouse = ptOut = vec2(ptIn.x, ptIn.y) / pointsScale;
		return true;
	}

	bool guiManager::windowResize(const ivec2 &res)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->inputResolution = res;
		return false;
	}

	bool guiManager::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->focusName = 0;
		return passMouseEvent<mouseButtonsFlags, &widgetItemStruct::mousePress>(impl, buttons, modifiers, point);
	}

	bool guiManager::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetItemStruct::mouseDouble>(impl, buttons, modifiers, point);
	}

	bool guiManager::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetItemStruct::mouseRelease>(impl, buttons, modifiers, point);
	}

	bool guiManager::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point)
	{
		guiImpl *impl = (guiImpl*)this;
		return passMouseEvent<mouseButtonsFlags, &widgetItemStruct::mouseMove>(impl, buttons, modifiers, point);
	}

	bool guiManager::mouseWheel(sint8 wheel, modifiersFlags modifiers, const ivec2 &point)
	{
		guiImpl *impl = (guiImpl*)this;
		vec2 pt;
		if (!impl->eventPoint(point, pt))
			return false;
		for (const auto &it : impl->mouseEventReceivers)
		{
			if (it.pointInside(pt, 1 | (1 << 31))) // also accept wheel events
			{
				if (it.widget->widgetState.disabled || it.widget->mouseWheel(wheel, modifiers, pt))
					return true;
			}
		}
		return false;
	}

	bool guiManager::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetItemStruct::keyPress>(impl, key, scanCode, modifiers);
	}

	bool guiManager::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetItemStruct::keyRepeat>(impl, key, scanCode, modifiers);
	}

	bool guiManager::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		return passKeyEvent<&widgetItemStruct::keyRelease>(impl, key, scanCode, modifiers);
	}

	bool guiManager::keyChar(uint32 key)
	{
		guiImpl *impl = (guiImpl*)this;
		vec2 dummy;
		if (!impl->eventPoint(impl->inputMouse, dummy))
			return false;
		bool res = false;
		for (auto f : focused(impl))
		{
			if (f->widgetState.disabled || f->keyChar(key))
				res = true;
		}
		return res;
	}

	void hierarchyItemStruct::fireWidgetEvent() const
	{
		if (ent->has(impl->components.event))
		{
			eventComponent &v = ent->value<eventComponent>(impl->components.event);
			if (v.event)
			{
				if (v.event(ent->name()))
					return;
			}
		}
		impl->widgetEvent.dispatch(ent->name());
	}
}
