#include "private.h"

namespace cage
{
	void GuiImpl::ttMouseMove(input::MouseMove in)
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

		// returns top-level widget under cursor and whether it has a tooltip
		std::pair<WidgetItem *, bool> findTopWidget(GuiImpl *impl)
		{
			for (const auto &it : impl->mouseEventReceivers)
			{
				if (it.pointInside(impl->outputMouse, GuiEventsTypesFlags::Default | GuiEventsTypesFlags::Tooltips))
					return { it.widget, any(it.mask & GuiEventsTypesFlags::Tooltips) };
			}
			return {};
		}

		bool listContains(PointerRange<const TooltipData> ttData, Entity *ent)
		{
			for (const TooltipData &tt : ttData)
				if (tt.invoker == ent)
					return true;
			return false;
		}

		// test if B is descendant of A
		bool isDescendantOf(const HierarchyItem *a, const HierarchyItem *b)
		{
			if (a == b)
				return true;
			for (const auto &it : a->children)
				if (isDescendantOf(+it, b))
					return true;
			return false;
		}

		void closeAllExceptSequenceWith(GuiImpl *impl, Entity *ent)
		{
			const HierarchyItem *k = findHierarchy(+impl->root, ent);
			for (auto &it : impl->ttData)
			{
				if (it.closeCondition == TooltipCloseConditionEnum::Never)
					continue;
				if (const HierarchyItem *h = findHierarchy(+impl->root, it.tooltip))
					if (!isDescendantOf(h, k))
						it.removing = true;
			}
		}
	}

	void GuiImpl::updateTooltips()
	{
		if (!ttEnabled)
			return;

		if (ttData.empty())
			ttNextOrder = 10000; // reset to reasonable value

		auto candidate = findTopWidget(this);

		const uint64 currentTime = applicationTime();
		if (ttMouseTraveledDistance > 5)
		{
			ttTimestampMouseMove = currentTime;
			ttMouseTraveledDistance = 0;
			ttHasMovedSinceLast = true;

			// close instant tooltips
			for (auto &it : ttData)
			{
				if (it.closeCondition != TooltipCloseConditionEnum::Instant)
					continue;
				if (candidate.first && candidate.first->hierarchy->ent == it.invoker)
					continue; // delay closing tooltip that would be the first to open again
				it.removing = true;
			}
		}
		const uint64 elapsed = currentTime - ttTimestampMouseMove;

		// close modal tooltips
		for (auto &it : ttData)
		{
			if (it.closeCondition != TooltipCloseConditionEnum::Modal)
				continue;
			if (candidate.first && candidate.first->hierarchy->ent == it.invoker)
				continue; // delay closing tooltip that would be the first to open again
			if (const HierarchyItem *h = findHierarchy(+root, it.tooltip))
			{
				Vec2 p = h->renderPos;
				Vec2 s = h->renderSize;
				offset(p, s, Vec4(it.extensionBorder));
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
					detail::guiDestroyEntityRecursively(it.rootTooltip); // this also removes the corresponding item from ttData
					break;
				}
			}
			if (!anyRemoved)
				break;
		}

		if (!ttHasMovedSinceLast)
			return;

		// check new tooltip
		if (!candidate.first || !candidate.second)
			return;

		// create new tooltip
		{
			Entity *ent = candidate.first->hierarchy->ent;
			CAGE_ASSERT(ent && ent->has<GuiTooltipComponent>());
			const GuiTooltipComponent &c = ent->value<GuiTooltipComponent>();
			if (c.delay > elapsed)
				return;
			if (!c.enableForDisabled && candidate.first->widgetState.disabled)
				return;
			if (listContains(ttData, ent))
				return; // this tooltip is already shown

			TooltipData tt;
			try
			{
				tt.invoker = ent;
				tt.tooltip = entityMgr->createUnique();
				tt.tooltip->value<GuiTooltipMarkerComponent>();
				tt.rootTooltip = tt.tooltip;
				tt.cursorPosition = outputMouse;
				{
					CAGE_ASSERT(findHierarchy(+root, ent));
					const HierarchyItem *h = findHierarchy(+root, ent);
					tt.invokerPosition = h->renderPos;
					tt.invokerSize = h->renderSize;
				}
				c.tooltip(tt);
				CAGE_ASSERT(tt.invoker == ent);
				CAGE_ASSERT(tt.tooltip == tt.rootTooltip);
				if (!tt.tooltip->has<GuiWidgetStateComponent>())
					tt.tooltip->value<GuiWidgetStateComponent>().skinIndex = 3;
				closeAllExceptSequenceWith(this, ent);
				ttData.push_back(std::move(tt));
				ttHasMovedSinceLast = false;
			}
			catch (...)
			{
				if (tt.tooltip)
					detail::guiDestroyEntityRecursively(tt.tooltip);
				throw;
			}

			// regenerate hierarchy with the new tooltip
			prepareImplGeneration();
		}

		// check if the new tooltip needs to update position
		TooltipData &it = ttData.back();
		if (it.placement == TooltipPlacementEnum::Manual)
			return;

		// update new tooltip position
		{
			it.rootTooltip = entityMgr->createUnique();
			it.rootTooltip->value<GuiTooltipMarkerComponent>();
			it.rootTooltip->value<GuiParentComponent>().order = ttNextOrder++;
			CAGE_ASSERT(findHierarchy(+root, it.tooltip));
			const Vec2 s = findHierarchy(+root, it.tooltip)->requestedSize;
			it.tooltip->value<GuiExplicitSizeComponent>().size = s + Vec2(1e-5);
			it.tooltip->value<GuiParentComponent>().parent = it.rootTooltip->id();
			Vec2 &al = it.rootTooltip->value<GuiLayoutAlignmentComponent>().alignment;
			switch (it.placement)
			{
				case TooltipPlacementEnum::InvokerCorner:
				{
					Vec2 corner = it.invokerPosition + it.invokerSize * 0.5 - outputSize * 0.5;
					corner[0] = corner[0] < 0 ? 0 : 1;
					corner[1] = corner[1] < 0 ? 0 : 1;
					al = (it.invokerPosition + it.invokerSize * (1 - corner) - s * corner) / max(outputSize - s, 1);
					break;
				}
				case TooltipPlacementEnum::CursorCorner:
				{
					Vec2i corner = Vec2i(((it.cursorPosition - outputSize * 0.5) / (outputSize * 0.5) + 1) * 0.5 * 2.9999) - 1;
					CAGE_ASSERT(corner[0] == -1 || corner[0] == 0 || corner[0] == 1);
					CAGE_ASSERT(corner[1] == -1 || corner[1] == 0 || corner[1] == 1);
					if (corner[1] == 0)
						corner[1] = 1; // avoid centering the tooltip under the cursor
					al = (it.cursorPosition - s * (Vec2(corner) * 0.5 + 0.5)) / max(outputSize - s, 1);
					al += -17 * Vec2(corner) / outputSize;
					break;
				}
				case TooltipPlacementEnum::InvokerCenter:
				{
					const Vec2 invokerCenter = it.invokerPosition + it.invokerSize * 0.5;
					al = (invokerCenter - s * Vec2(0.5)) / max(outputSize - s, 1);
					break;
				}
				case TooltipPlacementEnum::CursorCenter:
				{
					al = (it.cursorPosition - s * Vec2(0.5)) / max(outputSize - s, 1);
					break;
				}
				case TooltipPlacementEnum::ScreenCenter:
				{
					al = Vec2(0.5);
					break;
				}
				case TooltipPlacementEnum::Manual:
					break;
			}
			al = saturate(al);
			prepareImplGeneration(); // regenerate again to apply the changes
		}
	}

	void GuiImpl::clearTooltips()
	{
		while (!ttData.empty())
			detail::guiDestroyEntityRecursively(ttData.front().rootTooltip);
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
			e->value<GuiParentComponent>().parent = cfg.tooltip->id();
			e->value<GuiLabelComponent>();
			e->value<GuiTextComponent>() = *txt;
		}
	}

	namespace privat
	{
		GuiTooltipComponent::TooltipCallback guiTooltipText(const GuiTextComponent *txt)
		{
			GuiTooltipComponent::TooltipCallback tt;
			tt.bind<const GuiTextComponent *, &guiTooltipTextImpl>(txt);
			return tt;
		}
	}
}
