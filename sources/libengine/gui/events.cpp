#include "private.h"

namespace cage
{
	EventReceiver::EventReceiver() : widget(nullptr), mask(1)
	{}

	bool EventReceiver::pointInside(vec2 point, uint32 maskRequest) const
	{
		if ((mask & maskRequest) == 0)
			return false;
		return cage::pointInside(pos, size, point);
	}

	namespace
	{
		void findWidgets(HierarchyItem *item, uint32 name, std::vector<WidgetItem*> &result)
		{
			{
				WidgetItem *w = nullptr;
				if (item->ent && item->ent->name() == name && (w = dynamic_cast<WidgetItem*>(item->item)))
					result.push_back(w);
			}
			HierarchyItem *c = item->firstChild;
			while (c)
			{
				findWidgets(c, name, result);
				c = c->nextSibling;
			}
		}

		std::vector<WidgetItem*> focused(GuiImpl *impl)
		{
			std::vector<WidgetItem*> result;
			if (impl->focusName)
			{
				result.reserve(3);
				findWidgets(impl->root, impl->focusName, result);
			}
			return result;
		}

		template<class A, bool (WidgetItem::*F)(A, ModifiersFlags, vec2)>
		bool passMouseEvent(GuiImpl *impl, A a, ModifiersFlags m, const ivec2 &point)
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

		template<bool (WidgetItem::*F)(uint32, uint32, ModifiersFlags)>
		bool passKeyEvent(GuiImpl *impl, uint32 a, uint32 b, ModifiersFlags m)
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

	bool GuiImpl::eventPoint(const ivec2 &ptIn, vec2 &ptOut)
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
		outputMouse = ptOut = vec2(ptIn[0], ptIn[1]) / pointsScale;
		return true;
	}

	bool Gui::windowResize(const ivec2 &res)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->inputResolution = res;
		return false;
	}

	bool Gui::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->focusName = 0;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mousePress>(impl, buttons, modifiers, point);
	}

	bool Gui::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseDouble>(impl, buttons, modifiers, point);
	}

	bool Gui::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseRelease>(impl, buttons, modifiers, point);
	}

	bool Gui::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseMove>(impl, buttons, modifiers, point);
	}

	bool Gui::mouseWheel(sint32 wheel, ModifiersFlags modifiers, const ivec2 &point)
	{
		GuiImpl *impl = (GuiImpl*)this;
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

	bool Gui::keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passKeyEvent<&WidgetItem::keyPress>(impl, key, scanCode, modifiers);
	}

	bool Gui::keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passKeyEvent<&WidgetItem::keyRepeat>(impl, key, scanCode, modifiers);
	}

	bool Gui::keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return passKeyEvent<&WidgetItem::keyRelease>(impl, key, scanCode, modifiers);
	}

	bool Gui::keyChar(uint32 key)
	{
		GuiImpl *impl = (GuiImpl*)this;
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

	void HierarchyItem::fireWidgetEvent() const
	{
		if (ent->has(impl->components.Event))
		{
			GuiEventComponent &v = ent->value<GuiEventComponent>(impl->components.Event);
			if (v.event)
			{
				if (v.event(ent->name()))
					return;
			}
		}
		impl->widgetEvent.dispatch(ent->name());
	}
}
