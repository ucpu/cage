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
			ttHasMovedSinceLast = true;

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
			if (const HierarchyItem *h = findHierarchy(+root, it.rect))
			{
				Vec2 p = h->renderPos;
				Vec2 s = h->renderSize;
				offset(p, s, Vec4(20));
				it.removing |= !pointInside(p, s, outputMouse);
			}
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

		if (!ttHasMovedSinceLast)
			return;

		// find new tooltips
		bool needsReposition = false;
		for (const auto &it : mouseEventReceivers)
		{
			if (!it.pointInside(outputMouse, GuiEventsTypesFlags::Default | GuiEventsTypesFlags::Tooltips))
				continue; // this widget is not under the cursor - completely ignore and continue searching
			if (none(it.mask & GuiEventsTypesFlags::Tooltips))
				break; // this is top-level widget under the cursor, but does not have a tooltip - stop searching as all following widgets would be covered by this one
			Entity *ent = it.widget->hierarchy->ent;
			CAGE_ASSERT(ent && ent->has<GuiTooltipComponent>());
			const GuiTooltipComponent &c = ent->value<GuiTooltipComponent>();
			if (c.delay > elapsed)
				break;
			if (!c.enableForDisabled && it.widget->widgetState.disabled)
				break;
			if ([&]() {
				for (const auto &tt : ttData)
					if (tt.invoker == ent)
						return true;
				return false;
			}())
				break; // this tooltip is already shown

			TooltipData tt;
			try
			{
				tt.invoker = ent;
				tt.tooltip = entityMgr->createUnique();
				tt.tooltip->value<GuiTooltipMarkerComponent>();
				tt.rect = tt.tooltip;
				tt.anchor = outputMouse;
				c.tooltip(tt);
				if (tt.placement != TooltipPlacementEnum::Manual)
					needsReposition = true;
				if (!tt.tooltip->has<GuiWidgetStateComponent>())
					tt.tooltip->value<GuiWidgetStateComponent>().skinIndex = 3;
				ttData.push_back(std::move(tt));
				ttHasMovedSinceLast = false;
			}
			catch (...)
			{
				if (tt.tooltip)
					detail::guiDestroyEntityRecursively(tt.tooltip);
				throw;
			}
			break;
		}

		// update tooltip positions
		if (needsReposition)
		{
			prepareImplGeneration(); // regenerate the whole hierarchy to calculate the requested sizes
			for (auto &it : ttData)
			{
				if (const HierarchyItem *h = findHierarchy(+root, it.tooltip))
				{
					Entity *f = entityMgr->createUnique();
					f->value<GuiTooltipMarkerComponent>();
					const Vec2 s = h->requestedSize;
					it.tooltip->value<GuiExplicitSizeComponent>().size = s + Vec2(1e-5);
					it.tooltip->value<GuiParentComponent>().parent = f->name();
					switch (it.placement)
					{
					case TooltipPlacementEnum::Corner:
					{
						Vec2i corner = Vec2i(((it.anchor - outputSize * 0.5) / (outputSize * 0.5) + 1) * 0.5 * 2.9999) - 1;
						CAGE_ASSERT(corner[0] == -1 || corner[0] == 0 || corner[0] == 1);
						CAGE_ASSERT(corner[1] == -1 || corner[1] == 0 || corner[1] == 1);
						if (corner[1] == 0)
							corner[1] = 1; // avoid centering the tooltip under the cursor
						f->value<GuiScrollbarsComponent>().alignment = (it.anchor - s * (Vec2(corner) * 0.5 + 0.5)) / (outputSize - s);
						f->value<GuiScrollbarsComponent>().alignment += -17 * Vec2(corner) / outputSize;
					} break;
					case TooltipPlacementEnum::Center:
						f->value<GuiScrollbarsComponent>().alignment = (it.anchor - s * Vec2(0.5)) / (outputSize - s);
						break;
					case TooltipPlacementEnum::Manual:
						break;
					}
					it.tooltip = f;
				}
				it.placement = TooltipPlacementEnum::Manual;
			}
			prepareImplGeneration(); // and again to apply the changes
		}
	}

	void GuiImpl::clearTooltips()
	{
		while (!ttData.empty())
			detail::guiDestroyEntityRecursively(ttData.front().tooltip);
	}

	bool GuiImpl::tooltipRemoved(Entity *e)
	{
		std::erase_if(ttData, [&](const TooltipData &tt) { return tt.tooltip == e; });
		return false;
	}

	namespace
	{
		void guiTooltipTextImpl(const GuiTextComponent *txt, const GuiTooltipConfig &cfg)
		{
			cfg.tooltip->value<GuiPanelComponent>();
			Entity *e = cfg.tooltip->manager()->createUnique();
			e->value<GuiParentComponent>().parent = cfg.tooltip->name();
			e->value<GuiLabelComponent>();
			e->value<GuiTextComponent>() = *txt;
		}
	}

	namespace privat
	{
		decltype(GuiTooltipComponent::tooltip) guiTooltipText(const GuiTextComponent *txt)
		{
			decltype(GuiTooltipComponent::tooltip) tt;
			tt.bind<const GuiTextComponent *, &guiTooltipTextImpl>(txt);
			return tt;
		}
	}
}
