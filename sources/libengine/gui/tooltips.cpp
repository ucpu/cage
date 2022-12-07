#include "private.h"

namespace cage
{
	void GuiImpl::ttMouseMove(InputMouse in)
	{
		Vec2 pt;
		if (!eventPoint(in.position, pt))
		{
			clearTooltips(); // the mouse is outside of the scope of the gui
			return;
		}
		ttMouseTraveledDistance += distance(in.position, ttLastMousePos);
		ttLastMousePos = in.position;
	}

	namespace
	{
		HierarchyItem *findHierarchy(HierarchyItem *h, Entity *e)
		{
			if (h->ent == e)
				return h;
			for (const auto &it : h->children)
				if (HierarchyItem *r = findHierarchy(+it, e))
					return r;
			return nullptr;
		}
	}

	void GuiImpl::updateTooltips()
	{
		const uint64 currentTime = applicationTime();
		if (ttMouseTraveledDistance > 5)
		{
			ttTimestampMouseMove = currentTime;
			ttMouseTraveledDistance = 0;

			// close instant tooltips
			for (auto &it : ttData)
				if (it.closeCondition == TooltipCloseConditionEnum::Instant)
					it.removing = true;
		}
		const uint64 elapsed = currentTime - ttTimestampMouseMove;

		// close modal tooltips
		for (auto &it : ttData)
		{
			if (it.closeCondition != TooltipCloseConditionEnum::Modal)
				continue;
			if (it.reposition)
				continue; // cannot close tooltip that has not yet been positioned
			// todo check if mouse is outside of the tooltip
			it.removing = true;
		}

		// actually remove tooltips
		while (!ttData.empty())
		{
			bool anyRemoved = false;
			for (const auto &it : ttData)
			{
				if (it.removing)
				{
					anyRemoved = true;
					detail::guiDestroyEntityRecursively(it.tooltip); // this removes the corresponding item from ttData
					break;
				}
			}
			if (!anyRemoved)
				break;
		}

		// find new tooltips
		bool needsReposition = false;
		for (const auto &it : mouseEventReceivers)
		{
			if (it.pointInside(outputMouse, GuiEventsTypesFlags::Tooltips))
			{
				Entity *ent = it.widget->hierarchy->ent;
				CAGE_ASSERT(ent && ent->has<GuiTooltipComponent>());
				const GuiTooltipComponent &c = ent->value<GuiTooltipComponent>();
				if (c.delay > elapsed)
					continue;
				if (!c.enableForDisabled && it.widget->widgetState.disabled)
					continue;
				if ([&]() {
					for (const auto &tt : ttData)
						if (tt.invoker == ent)
							return true;
					return false;
				}())
					continue; // this tooltip is already shown

				TooltipData tt;
				try
				{
					tt.invoker = ent;
					tt.tooltip = entityMgr->createUnique();
					tt.tooltip->value<GuiTooltipMarkerComponent>();
					tt.position = outputMouse;
					c.tooltip(tt);
					if (tt.reposition)
						needsReposition = true;
					ttData.push_back(std::move(tt));
				}
				catch (...)
				{
					if (tt.tooltip)
						detail::guiDestroyEntityRecursively(tt.tooltip);
					throw;
				}
			}
		}

		// update tooltip positions
		if (needsReposition)
		{
			prepareImplGeneration(); // regenerate the whole hierarchy to calculate the requested sizes
			for (auto &it : ttData)
			{
				if (!it.reposition)
					continue;
				if (const HierarchyItem *h = findHierarchy(+root, it.tooltip))
				{
					Entity *f = entityMgr->createUnique();
					f->value<GuiTooltipMarkerComponent>();
					const Vec2 s = h->requestedSize;
					it.tooltip->value<GuiExplicitSizeComponent>().size = s + Vec2(1e-5);
					it.tooltip->value<GuiParentComponent>().parent = f->name();
					f->value<GuiScrollbarsComponent>().alignment = (it.position - s * Vec2(0.5, 1)) / (outputSize - s) - Vec2(0, 1e-3);
					it.tooltip = f;
				}
				it.reposition = false;
			}
			prepareImplGeneration(); // and again to apply the changes
		}
	}

	void GuiImpl::clearTooltips()
	{
		while (!!ttData.empty())
			detail::guiDestroyEntityRecursively(ttData.front().tooltip);
	}

	bool GuiImpl::tooltipRemoved(Entity *e)
	{
		std::erase_if(ttData, [&](const TooltipData &tt) { return tt.tooltip == e; });
		return false;
	}
}
