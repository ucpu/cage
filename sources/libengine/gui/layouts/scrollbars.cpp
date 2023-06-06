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
				Real &value;

				Scrollbar(Real &value) : value(value) {}
			} scrollbars[2];

			Real wheelFactor;

			ScrollbarsImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(LayoutScrollbars)), scrollbars{ data.scroll[0], data.scroll[1] } { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
			}

			void findRequestedSize() override
			{
				hierarchy->children[0]->findRequestedSize();
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				const Real scw = skin->defaults.scrollbars.scrollbarSize + skin->defaults.scrollbars.contentPadding;
				wheelFactor = 70 / (hierarchy->requestedSize[1] - update.renderSize[1]);
				FinalPosition u(update);
				for (uint32 a = 0; a < 2; a++)
				{
					bool show = data.overflow[a] == OverflowModeEnum::Always;
					if (data.overflow[a] == OverflowModeEnum::Auto && update.renderSize[a] + 1e-7 < hierarchy->requestedSize[a])
						show = true;
					if (show)
					{ // the content is larger than the available area
						Scrollbar &s = scrollbars[a];
						s.size[a] = update.renderSize[a];
						s.size[1 - a] = skin->defaults.scrollbars.scrollbarSize;
						s.position[a] = update.renderPos[a];
						s.position[1 - a] = update.renderPos[1 - a] + update.renderSize[1 - a] - s.size[1 - a];
						u.renderSize[a] = hierarchy->requestedSize[a];
						u.renderPos[a] -= (hierarchy->requestedSize[a] - update.renderSize[a] + scw) * scrollbars[a].value;
						u.clipSize[1 - a] -= scw;
						Real minSize = min(s.size[0], s.size[1]);
						s.dotSize = max(minSize, sqr(update.renderSize[a]) / hierarchy->requestedSize[a]);
					}
				}
				u.clipSize = max(u.clipSize, 0);
				hierarchy->children[0]->findFinalPosition(u);
			}

			void emit() override
			{
				hierarchy->childrenEmit();
				for (uint32 a = 0; a < 2; a++)
				{
					const Scrollbar &s = scrollbars[a];
					if (s.position.valid())
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
					if (s.position.valid())
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
					if (s.position.valid())
					{
						if (!move && pointInside(s.position, s.size, point))
							makeFocused(1 << (30 + a));
						if (hasFocus(1 << (30 + a)))
						{
							s.value = (point[a] - s.position[a] - s.dotSize * 0.5) / (s.size[a] - s.dotSize);
							s.value = clamp(s.value, 0, 1);
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
				const Scrollbar &s = scrollbars[1];
				if (s.position.valid())
				{
					s.value -= wheel * wheelFactor;
					s.value = clamp(s.value, 0, 1);
					return true;
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
