#include "private.h"

namespace cage
{
	EventReceiver::EventReceiver() : widget(nullptr), mask(1)
	{}

	bool EventReceiver::pointInside(Vec2 point, uint32 maskRequest) const
	{
		if ((mask & maskRequest) == 0)
			return false;
		return cage::pointInside(pos, size, point);
	}

	namespace
	{
		void findWidgets(HierarchyItem *item, uint32 name, std::vector<WidgetItem *> &result)
		{
			if (item->ent && item->ent->name() == name)
			{
				WidgetItem *w = dynamic_cast<WidgetItem *>(+item->item);
				if (w)
					result.push_back(w);
			}
			for (auto &it : item->children)
				findWidgets(+it, name, result);
		}

		std::vector<WidgetItem *> focused(GuiImpl *impl)
		{
			std::vector<WidgetItem*> result;
			if (impl->focusName)
			{
				result.reserve(3);
				findWidgets(+impl->root, impl->focusName, result);
			}
			return result;
		}

		template<class A, bool (WidgetItem::*F)(A, ModifiersFlags, Vec2)>
		bool passMouseEvent(GuiImpl *impl, A a, ModifiersFlags m, const Vec2i &point)
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

	bool GuiImpl::eventPoint(const Vec2i &ptIn, Vec2 &ptOut)
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
		outputMouse = ptOut = Vec2(ptIn[0], ptIn[1]) / pointsScale;
		return true;
	}

	bool GuiManager::handleInput(const GenericInput &in)
	{
		GuiImpl *impl = (GuiImpl *)this;
		return impl->inputsDispatchers.dispatch(in);
	}

	bool GuiImpl::windowResize(InputWindowValue in)
	{
		inputResolution = in.value;
		return false;
	}

	bool GuiImpl::mousePress(InputMouse in)
	{
		focusName = 0;
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mousePress>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseDoublePress(InputMouse in)
	{
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseDouble>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseRelease(InputMouse in)
	{
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseRelease>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseMove(InputMouse in)
	{
		return passMouseEvent<MouseButtonsFlags, &WidgetItem::mouseMove>(this, in.buttons, in.mods, in.position);
	}

	bool GuiImpl::mouseWheel(InputMouseWheel in)
	{
		Vec2 pt;
		if (!eventPoint(in.position, pt))
			return false;
		for (const auto &it : mouseEventReceivers)
		{
			if (it.pointInside(pt, 1 | (1 << 31))) // also accept wheel events
			{
				if (it.widget->widgetState.disabled || it.widget->mouseWheel(in.wheel, in.mods, pt))
					return true;
			}
		}
		return false;
	}

	bool GuiImpl::keyPress(InputKey in)
	{
		return passKeyEvent<&WidgetItem::keyPress>(this, in.key, in.mods);
	}

	bool GuiImpl::keyRepeat(InputKey in)
	{
		return passKeyEvent<&WidgetItem::keyRepeat>(this, in.key, in.mods);
	}

	bool GuiImpl::keyRelease(InputKey in)
	{
		return passKeyEvent<&WidgetItem::keyRelease>(this, in.key, in.mods);
	}

	bool GuiImpl::keyChar(InputKey in)
	{
		Vec2 dummy;
		if (!eventPoint(inputMouse, dummy))
			return false;
		bool res = false;
		for (auto f : focused(this))
		{
			if (f->widgetState.disabled || f->keyChar(in.key))
				res = true;
		}
		return res;
	}

	void HierarchyItem::fireWidgetEvent() const
	{
		if (ent->has<GuiEventComponent>())
		{
			GuiEventComponent &v = ent->value<GuiEventComponent>();
			if (v.event)
			{
				if (v.event(ent->name()))
					return;
			}
		}
		InputGuiWidget e;
		e.manager = impl;
		e.widget = ent->name();
		impl->widgetEvent.dispatch({ e, InputClassEnum::GuiWidget });
	}
}
