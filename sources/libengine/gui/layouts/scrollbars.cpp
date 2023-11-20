#include "../private.h"

namespace cage
{
	namespace
	{
		struct ScrollbarsImpl : public WidgetItem
		{
			GuiLayoutScrollbarsComponent &data;

			struct Scrollbar
			{
				Vec2 position = Vec2::Nan();
				Vec2 size = Vec2::Nan();
				Real dotSize = Real::Nan();
				Real wheelFactor = 0;
				Real &value;
				bool shown = false;

				Scrollbar(Real &value) : value(value) {}
			} scrollbars[2];

			ScrollbarsImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(LayoutScrollbars)), scrollbars{ data.scroll[0], data.scroll[1] } { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT(data.scroll.valid() && data.scroll == saturate(data.scroll));
			}

			void findRequestedSize() override
			{
				const Real scw = skin->defaults.scrollbars.scrollbarSize + skin->defaults.scrollbars.contentPadding;
				hierarchy->children[0]->findRequestedSize();
				for (uint32 a = 0; a < 2; a++)
					hierarchy->requestedSize[a] = hierarchy->children[0]->requestedSize[a] + (data.overflow[1 - a] == OverflowModeEnum::Never ? 0 : scw);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				const Vec2 requested = hierarchy->children[0]->requestedSize;
				FinalPosition u(update);
				for (uint32 a = 0; a < 2; a++)
				{
					Scrollbar &s = scrollbars[a];
					s.shown = data.overflow[a] == OverflowModeEnum::Always;
					if (data.overflow[a] == OverflowModeEnum::Auto && requested[a] > update.renderSize[a] + 1e-3)
						s.shown = true; // the content is larger than the available area
				}
				const Real scw1 = skin->defaults.scrollbars.scrollbarSize + skin->defaults.scrollbars.contentPadding;
				const Vec2 scw = Vec2(scrollbars[1].shown ? scw1 : 0, scrollbars[0].shown ? scw1 : 0);
				for (uint32 a = 0; a < 2; a++)
				{
					Scrollbar &s = scrollbars[a];
					if (s.shown)
					{
						s.size[a] = update.renderSize[a] - scw[a];
						s.size[1 - a] = skin->defaults.scrollbars.scrollbarSize;
						s.position[a] = update.renderPos[a];
						s.position[1 - a] = update.renderPos[1 - a] + update.renderSize[1 - a] - s.size[1 - a];
						s.dotSize = max(min(s.size[0], s.size[1]), sqr(update.renderSize[a]) / hierarchy->requestedSize[a]);

						u.renderSize[a] = requested[a];
						u.renderPos[a] -= (hierarchy->children[0]->requestedSize[a] - update.renderSize[a] + scw[a]) * scrollbars[a].value;
					}
					else
						u.renderSize[a] -= scw[a];
				}
				u.renderSize = max(u.renderSize, 0);
				u.clipSize = min(u.clipSize, update.renderSize - scw);
				hierarchy->children[0]->findFinalPosition(u);
				for (uint32 a = 0; a < 2; a++)
					scrollbars[a].wheelFactor = 70 / max(requested[a] - update.renderSize[a], 1);
			}

			void emit() override
			{
				CAGE_ASSERT(data.scroll.valid());
				hierarchy->childrenEmit();
				for (uint32 a = 0; a < 2; a++)
				{
					const Scrollbar &s = scrollbars[a];
					CAGE_ASSERT(s.shown == s.position.valid());
					CAGE_ASSERT(s.shown == s.size.valid());
					CAGE_ASSERT(s.shown == s.dotSize.valid());
					if (s.shown)
					{
						emitElement(a == 0 ? GuiElementTypeEnum::ScrollbarHorizontalPanel : GuiElementTypeEnum::ScrollbarVerticalPanel, ElementModeEnum::Default, s.position, s.size);
						Vec2 ds;
						ds[a] = s.dotSize;
						ds[1 - a] = s.size[1 - a];
						Vec2 dp = s.position;
						dp[a] += (s.size[a] - ds[a]) * s.value;
						emitElement(a == 0 ? GuiElementTypeEnum::ScrollbarHorizontalDot : GuiElementTypeEnum::ScrollbarVerticalDot, mode(s.position, s.size, 1 << (30 + a)), dp, ds);
					}
				}
			}

			void generateEventReceivers() override
			{
				for (uint32 a = 0; a < 2; a++)
				{
					const Scrollbar &s = scrollbars[a];
					if (s.shown)
					{
						EventReceiver e;
						e.widget = this;
						e.pos = s.position;
						e.size = s.size;
						if (clip(e.pos, e.size, hierarchy->clipPos, hierarchy->clipSize))
							hierarchy->impl->mouseEventReceivers.push_back(e);
					}
				}

				{ // event receiver for wheel
					EventReceiver e;
					e.widget = this;
					e.pos = hierarchy->renderPos;
					e.size = hierarchy->renderSize;
					e.mask = GuiEventsTypesFlags::Wheel;
					if (clip(e.pos, e.size, hierarchy->clipPos, hierarchy->clipSize))
						hierarchy->impl->mouseEventReceivers.push_back(e);
				}
			}

			bool handleMouse(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point, bool move)
			{
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				for (uint32 a = 0; a < 2; a++)
				{
					const Scrollbar &s = scrollbars[a];
					if (s.shown)
					{
						if (!move && pointInside(s.position, s.size, point))
							makeFocused(1 << (30 + a));
						if (hasFocus(1 << (30 + a)))
						{
							s.value = (point[a] - s.position[a] - s.dotSize * 0.5) / (s.size[a] - s.dotSize);
							s.value = saturate(s.value);
							return true;
						}
					}
				}
				return true;
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				makeFocused();
				return handleMouse(buttons, modifiers, point, false);
			}

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override { return handleMouse(buttons, modifiers, point, true); }

			bool mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point) override
			{
				if (modifiers != ModifiersFlags::None)
					return false;
				for (Scrollbar *s : { &scrollbars[1], &scrollbars[0] })
				{
					if (s->shown)
					{
						s->value -= wheel * s->wheelFactor;
						s->value = saturate(s->value);
						return true;
					}
				}
				return false;
			}
		};
	}

	void scrollbarsCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ScrollbarsImpl>(item).cast<BaseItem>();
	}
}
