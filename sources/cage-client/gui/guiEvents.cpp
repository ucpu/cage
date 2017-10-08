#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/gui.h>
#include "private.h"

namespace cage
{
	namespace
	{
		struct eventStruct
		{
			enum eventTypeEnum
			{
				etPress,
				etRelease,
				etMove,
				etWheel,
			} eventType;
			windowClass *win;
			union
			{
				mouseButtonsFlags buttons;
				sint8 wheel;
			};
			modifiersFlags modifiers;
			pointStruct point;
		};

		const bool handleEventStruct(controlCacheStruct *control, const eventStruct &event)
		{
			if (control->controlMode == controlCacheStruct::cmHover)
			{
				if (!control->verifyControlEntity())
					return true;
				switch (event.eventType)
				{
				case eventStruct::etPress: return control->mousePress(event.win, event.buttons, event.modifiers, event.point);
				case eventStruct::etRelease: return control->mouseRelease(event.win, event.buttons, event.modifiers, event.point);
				case eventStruct::etMove: return control->mouseMove(event.win, event.buttons, event.modifiers, event.point);
				case eventStruct::etWheel: return control->mouseWheel(event.win, event.wheel, event.modifiers, event.point);
				default: CAGE_THROW_CRITICAL(exception, "invalid event type");
				}
			}
			return false;
		}

		const bool passEventStruct(controlCacheStruct *control, const eventStruct &event)
		{
			// events are passed in reverse order (front to back)
			while (control)
			{
				if (passEventStruct(control->lastChild, event))
					return true;
				if (handleEventStruct(control, event))
					return true;
				control = control->prevSibling;
			}
			return false;
		}

		controlCacheStruct *findControl(controlCacheStruct *control, uint32 name)
		{
			if (control->entity == name)
				return control;
			controlCacheStruct *c = control->firstChild;
			while (c)
			{
				controlCacheStruct *r = findControl(c, name);
				if (r)
					return r;
				c = c->nextSibling;
			}
			return nullptr;
		}
	}

	bool guiClass::windowResize(windowClass *win, const pointStruct &p)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->windowSize = vec2(p.x, p.y);
		return false;
	}

	bool guiClass::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!win->isFocused())
			return false;
		guiImpl *impl = (guiImpl*)this;
		impl->mousePosition = vec2(point.x, point.y);
		impl->mouseButtons = (mouseButtonsFlags)(impl->mouseButtons | buttons);
		if (impl->cacheRoot)
		{
			eventStruct e;
			e.win = win;
			e.buttons = buttons;
			e.modifiers = modifiers;
			e.point = point;
			e.eventType = eventStruct::etPress;
			if (passEventStruct(impl->cacheRoot, e))
				return true;
		}
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			impl->focusName = 0;
		return false;
	}

	bool guiClass::mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!win->isFocused())
			return false;
		guiImpl *impl = (guiImpl*)this;
		impl->mousePosition = vec2(point.x, point.y);
		impl->mouseButtons = (mouseButtonsFlags)(impl->mouseButtons & ~buttons);
		if (impl->cacheRoot)
		{
			eventStruct e;
			e.win = win;
			e.buttons = buttons;
			e.modifiers = modifiers;
			e.point = point;
			e.eventType = eventStruct::etRelease;
			if (passEventStruct(impl->cacheRoot, e))
				return true;
		}
		return false;
	}

	bool guiClass::mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!win->isFocused())
			return false;
		guiImpl *impl = (guiImpl*)this;
		impl->mousePosition = vec2(point.x, point.y);
		if (impl->cacheRoot)
		{
			eventStruct e;
			e.win = win;
			e.buttons = buttons;
			e.modifiers = modifiers;
			e.point = point;
			e.eventType = eventStruct::etMove;
			if (passEventStruct(impl->cacheRoot, e))
				return true;
		}
		return false;
	}

	bool guiClass::mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!win->isFocused())
			return false;
		guiImpl *impl = (guiImpl*)this;
		impl->mousePosition = vec2(point.x, point.y);
		if (impl->cacheRoot)
		{
			eventStruct e;
			e.win = win;
			e.wheel = wheel;
			e.modifiers = modifiers;
			e.point = point;
			e.eventType = eventStruct::etWheel;
			if (passEventStruct(impl->cacheRoot, e))
				return true;
		}
		return false;
	}

	bool guiClass::keyPress(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		controlCacheStruct *c = impl->focusName ? findControl(impl->cacheRoot, impl->focusName) : nullptr;
		if (c && c->verifyControlEntity())
		{
			if (c->keyPress(win, key, scanCode, modifiers))
				return true;
		}
		return false;
	}

	bool guiClass::keyRepeat(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		controlCacheStruct *c = impl->focusName ? findControl(impl->cacheRoot, impl->focusName) : nullptr;
		if (c && c->verifyControlEntity())
		{
			if (c->keyRepeat(win, key, scanCode, modifiers))
				return true;
		}
		return false;
	}

	bool guiClass::keyRelease(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		guiImpl *impl = (guiImpl*)this;
		/*
		if (impl->focus && key == 9)
		{
			moveFocus(impl, !!(modifiers & modifiersFlags::Shift));
			return true;
		}
		*/
		controlCacheStruct *c = impl->focusName ? findControl(impl->cacheRoot, impl->focusName) : nullptr;
		if (c && c->verifyControlEntity())
		{
			if (c->keyRelease(win, key, scanCode, modifiers))
				return true;
		}
		return false;
	}

	bool guiClass::keyChar(windowClass *win, uint32 key)
	{
		guiImpl *impl = (guiImpl*)this;
		controlCacheStruct *c = impl->focusName ? findControl(impl->cacheRoot, impl->focusName) : nullptr;
		if (c && c->verifyControlEntity())
		{
			if (c->keyChar(win, key))
				return true;
		}
		return false;
	}

	void guiClass::handleWindowEvents(windowClass *window) const
	{
		guiImpl *impl = (guiImpl*)this;
		impl->listeners.attachAll(window);
	}
}