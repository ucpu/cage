#include "private.h"

namespace cage
{
	bool EventReceiver::pointInside(Vec2 point, GuiEventsTypesFlags maskRequest) const
	{
		if (none(mask & maskRequest))
			return false;
		return cage::pointInside(pos, size, point);
	}

	namespace
	{
		void findWidgets(HierarchyItem *item, uint32 name, std::vector<WidgetItem *> &result)
		{
			if (item->ent && item->ent->name() == name)
			{
				if (WidgetItem *w = dynamic_cast<WidgetItem *>(+item->item))
					result.push_back(w);
			}
			for (auto &it : item->children)
				findWidgets(+it, name, result);
		}

		std::vector<WidgetItem *> focused(GuiImpl *impl)
		{
			std::vector<WidgetItem *> result;
			if (impl->focusName)
			{
				result.reserve(3);
				findWidgets(+impl->root, impl->focusName, result);
			}
			return result;
		}

		template<class A, bool (WidgetItem::*F)(A, ModifiersFlags, Vec2)>
		bool passMouseEvent(GuiImpl *impl, A a, ModifiersFlags m, const Vec2 &point)
		{
			Vec2 pt;
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

		template<bool (WidgetItem::*F)(uint32, ModifiersFlags)>
		bool passKeyEvent(GuiImpl *impl, uint32 key, ModifiersFlags m)
		{
			Vec2 dummy;
			if (!impl->eventPoint(impl->inputMouse, dummy))
				return false;
			bool res = false;
			for (auto f : focused(impl))
			{
				if (f->widgetState.disabled || (f->*F)(key, m))
					res = true;
			}
			return res;
		}
	}

	bool GuiImpl::eventPoint(Vec2 ptIn, Vec2 &ptOut)
	{
		inputMouse = ptIn;
		if (!eventsEnabled)
			return false;
		outputMouse = ptOut = ptIn / pointsScale;
		return true;
	}

	bool GuiManager::handleInput(const GenericInput &in)
	{
		GuiImpl *impl = (GuiImpl *)this;
		if (in.has<input::MousePress>())
			return impl->mousePress(in.get<input::MousePress>());
		if (in.has<input::MouseDoublePress>())
			return impl->mouseDoublePress(in.get<input::MouseDoublePress>());
		if (in.has<input::MouseRelease>())
			return impl->mouseRelease(in.get<input::MouseRelease>());
		if (in.has<input::MouseMove>())
			return impl->mouseMove(in.get<input::MouseMove>());
		if (in.has<input::MouseWheel>())
			return impl->mouseWheel(in.get<input::MouseWheel>());
		if (in.has<input::KeyPress>())
			return impl->keyPress(in.get<input::KeyPress>());
		if (in.has<input::KeyRepeat>())
			return impl->keyRepeat(in.get<input::KeyRepeat>());
		if (in.has<input::KeyRelease>())
			return impl->keyRelease(in.get<input::KeyRelease>());
		if (in.has<input::Character>())
			return impl->keyChar(in.get<input::Character>());
		return false;
	}

	bool GuiImpl::mousePress(input::MousePress in)
	{
		focusName = 0;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mousePress>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseDoublePress(input::MouseDoublePress in)
	{
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseDouble>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseRelease(input::MouseRelease in)
	{
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseRelease>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseMove(input::MouseMove in)
	{
		ttMouseMove(in);
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseMove>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseWheel(input::MouseWheel in)
	{
		Vec2 pt;
		if (!eventPoint(in.position, pt))
			return false;
		for (const auto &it : mouseEventReceivers)
		{
			if (it.pointInside(pt, GuiEventsTypesFlags::Wheel))
			{
				if (it.widget->widgetState.disabled || it.widget->mouseWheel(in.wheel, in.mods, pt))
					return true;
			}
		}
		return false;
	}

	bool GuiImpl::keyPress(input::KeyPress in)
	{
		return passKeyEvent<&WidgetItem::keyPress>(this, in.key, in.mods);
	}

	bool GuiImpl::keyRepeat(input::KeyRepeat in)
	{
		return passKeyEvent<&WidgetItem::keyRepeat>(this, in.key, in.mods);
	}

	bool GuiImpl::keyRelease(input::KeyRelease in)
	{
		return passKeyEvent<&WidgetItem::keyRelease>(this, in.key, in.mods);
	}

	bool GuiImpl::keyChar(input::Character in)
	{
		Vec2 dummy;
		if (!eventPoint(inputMouse, dummy))
			return false;
		bool res = false;
		for (auto f : focused(this))
		{
			if (f->widgetState.disabled || f->keyChar(in.character))
				res = true;
		}
		return res;
	}

	void HierarchyItem::fireWidgetEvent(GenericInput in) const
	{
		if (ent->has<GuiEventComponent>())
		{
			GuiEventComponent &v = ent->value<GuiEventComponent>();
			if (v.event)
			{
				if (v.event(in))
					return;
			}
		}
		impl->widgetEvent.dispatch(in);
	}

	void HierarchyItem::fireWidgetEvent() const
	{
		fireWidgetEvent(input::GuiValue{ impl, ent });
	}
}
