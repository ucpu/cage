#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/log.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct scrollbarsImpl : public widgetItemStruct
		{
			scrollbarsComponent &data;

			struct scrollbarStruct
			{
				vec2 position;
				vec2 size;
				real dotSize;
				real &value;
				scrollbarStruct(real &value) : position(vec2::Nan), size(vec2::Nan), dotSize(real::Nan), value(value)
				{}
			} scrollbars[2];

			real wheelFactor;

			scrollbarsImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(scrollbars)), scrollbars{ data.scroll[0], data.scroll[1] }
			{
				ensureItemHasLayout(hierarchy);
			}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!hierarchy->text, "scrollbars may not have text");
				CAGE_ASSERT_RUNTIME(!hierarchy->image, "scrollbars may not have image");
			}

			virtual void findRequestedSize() override
			{
				hierarchy->firstChild->findRequestedSize();
				hierarchy->requestedSize = hierarchy->firstChild->requestedSize;
				for (uint32 a = 0; a < 2; a++)
				{
					if (data.overflow[a] == overflowModeEnum::Always)
						hierarchy->requestedSize[a] += skin->defaults.scrollbars.scrollbarSize + skin->defaults.scrollbars.contentPadding;
				}
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				wheelFactor = 70 / (hierarchy->requestedSize[1] - update.renderSize[1]);
				finalPositionStruct u(update);
				for (uint32 a = 0; a < 2; a++)
				{
					bool show = data.overflow[a] == overflowModeEnum::Always;
					if (data.overflow[a] == overflowModeEnum::Auto && update.renderSize[a] + 1e-7 < hierarchy->requestedSize[a])
						show = true;
					if (show)
					{ // the content is larger than the available area
						scrollbarStruct &s = scrollbars[a];
						s.size[a] = update.renderSize[a];
						s.size[1 - a] = skin->defaults.scrollbars.scrollbarSize;
						s.position[a] = update.renderPos[a];
						s.position[1 - a] = update.renderPos[1 - a] + update.renderSize[1 - a] - s.size[1 - a];
						u.renderPos[a] -= (hierarchy->requestedSize[a] - update.renderSize[a]) * scrollbars[a].value;
						u.renderSize[a] = hierarchy->requestedSize[a];
						u.renderSize[1 - a] -= s.size[1 - a] + skin->defaults.scrollbars.contentPadding;
						u.renderSize[1 - a] = max(u.renderSize[1 - a], 0);
						real minSize = min(s.size[0], s.size[1]);
						s.dotSize = max(minSize, sqr(update.renderSize[a]) / hierarchy->requestedSize[a]);
					}
					else
					{ // the content is smaller than the available area
						u.renderPos[a] += (update.renderSize[a] - hierarchy->requestedSize[a]) * data.alignment[a];
						u.renderSize[a] = hierarchy->requestedSize[a];
					}
				}
				hierarchy->firstChild->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				hierarchy->childrenEmit();
				for (uint32 a = 0; a < 2; a++)
				{
					const scrollbarStruct &s = scrollbars[a];
					if (s.position.valid())
					{
						emitElement(a == 0 ? elementTypeEnum::ScrollbarHorizontalPanel : elementTypeEnum::ScrollbarVerticalPanel, mode(s.position, s.size, 1 << (30 + a)), s.position, s.size);
						vec2 ds;
						ds[a] = s.dotSize;
						ds[1 - a] = s.size[1 - a];
						vec2 dp = s.position;
						dp[a] += (s.size[a] - ds[a]) * s.value;
						emitElement(a == 0 ? elementTypeEnum::ScrollbarHorizontalDot : elementTypeEnum::ScrollbarVerticalDot, mode(dp, ds, 1 << (30 + a)), dp, ds);
					}
				}
			}

			bool handleMouse(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point, bool move)
			{
				if (buttons != mouseButtonsFlags::Left)
					return false;
				if (modifiers != modifiersFlags::None)
					return false;
				for (uint32 a = 0; a < 2; a++)
				{
					const scrollbarStruct &s = scrollbars[a];
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
				return false;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				return handleMouse(buttons, modifiers, point, false);
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return handleMouse(buttons, modifiers, point, true);
			}

			virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point) override
			{
				if (modifiers != modifiersFlags::None)
					return false;
				const scrollbarStruct &s = scrollbars[1];
				if (s.position.valid())
				{
					s.value -= wheel * wheelFactor;
					s.value = clamp(s.value, 0, 1);
					return true;
				}
				return false;
			}

			virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers) override
			{
				return false;
			}

			virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers) override
			{
				return false;
			}

			virtual bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers) override
			{
				return false;
			}

			virtual bool keyChar(uint32 key) override
			{
				return false;
			}
		};
	}

	void scrollbarsCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<scrollbarsImpl>(item);
	}
}
