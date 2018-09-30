#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	void layoutDefaultCreate(guiItemStruct *item);

	namespace
	{
		struct scrollbarsImpl : public widgetBaseStruct
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

			scrollbarsImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(scrollbars)), scrollbars{ data.scroll[0], data.scroll[1] }
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->text, "scrollbars may not have text");
				CAGE_ASSERT_RUNTIME(!base->image, "scrollbars may not have image");
				if (!base->layout)
					layoutDefaultCreate(base);
			}

			virtual void findRequestedSize() override
			{
				base->layout->findRequestedSize();
				for (uint32 a = 0; a < 2; a++)
				{
					if (data.overflow[a] == overflowModeEnum::Always)
						base->requestedSize[a] += skin().defaults.scrollbars.scrollbarSize + skin().defaults.scrollbars.contentPadding;
				}
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				finalPositionStruct u(update);
				for (uint32 a = 0; a < 2; a++)
				{
					bool show = data.overflow[a] == overflowModeEnum::Always;
					if (data.overflow[a] == overflowModeEnum::Auto && update.size[a] < base->requestedSize[a])
						show = true;
					if (show)
					{ // the content is larger than the available area
						scrollbarStruct &s = scrollbars[a];
						s.size[a] = update.size[a];
						s.size[1 - a] = skin().defaults.scrollbars.scrollbarSize;
						s.position[a] = update.position[a];
						s.position[1 - a] = update.position[1 - a] + update.size[1 - a] - s.size[1 - a];
						u.position[a] -= (base->requestedSize[a] - update.size[a]) * scrollbars[a].value;
						u.size[a] = base->requestedSize[a];
						u.size[1 - a] -= s.size[1 - a] + skin().defaults.scrollbars.contentPadding;
						real minSize = min(s.size[0], s.size[1]);
						s.dotSize = max(minSize, sqr(update.size[a]) / base->requestedSize[a]);
					}
					else
					{ // the content is smaller than the available area
						u.position[a] += (update.size[a] - base->requestedSize[a]) * data.alignment[a];
						u.size[a] = base->requestedSize[a];
					}
				}
				base->layout->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				base->childrenEmit();
				for (uint32 a = 0; a < 2; a++)
				{
					const scrollbarStruct &s = scrollbars[a];
					if (s.position.valid())
					{
						emitElement(a == 0 ? elementTypeEnum::ScrollbarHorizontalPanel : elementTypeEnum::ScrollbarVerticalPanel, mode(s.position, s.size), s.position, s.size);
						vec2 ds;
						ds[a] = s.dotSize;
						ds[1 - a] = s.size[1 - a];
						vec2 dp = s.position;
						dp[a] += (s.size[a] - ds[a]) * s.value;
						emitElement(a == 0 ? elementTypeEnum::ScrollbarHorizontalDot : elementTypeEnum::ScrollbarVerticalDot, mode(dp, ds), dp, ds);
					}
				}
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				base->impl->focusName = 0;
				for (uint32 a = 0; a < 2; a++)
				{
					const scrollbarStruct &s = scrollbars[a];
					if (s.position.valid())
					{
						if (pointInside(s.position, s.size, point))
						{
							if (buttons != mouseButtonsFlags::Left)
								return true;
							if (modifiers != modifiersFlags::None)
								return true;
							s.value = (point[a] - s.position[a] - s.dotSize * 0.5) / (s.size[a] - s.dotSize);
							s.value = clamp(s.value, 0, 1);
							return true;
						}
					}
				}
				return false;
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return mousePress(buttons, modifiers, point);
			}
		};

		struct dummyLayoutImpl : public layoutBaseStruct
		{
			dummyLayoutImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(base->firstChild == base->lastChild);
			}

			virtual void findRequestedSize() override
			{
				if (base->firstChild)
				{
					base->firstChild->findRequestedSize();
					base->requestedSize = base->firstChild->requestedSize;
				}
				else
					base->requestedSize = vec2();
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				if (base->firstChild)
				{
					finalPositionStruct u(update);
					base->firstChild->findFinalPosition(u);
				}
			}
		};

		struct dummyWidgetImpl : public widgetBaseStruct
		{
			dummyWidgetImpl(guiItemStruct *base) : widgetBaseStruct(base)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(base->firstChild && base->firstChild == base->lastChild);
			}

			virtual void findRequestedSize() override
			{
				base->layout->findRequestedSize();
				vec2 pos;
				base->checkExplicitPosition(pos, base->requestedSize);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				vec2 pos, size = base->requestedSize;
				base->checkExplicitPosition(pos, size);
				finalPositionStruct u(update);
				u.position += pos;
				u.size = size;
				base->firstChild->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				base->childrenEmit();
			}
		};
	}

	void scrollbarsCreate(guiItemStruct *item)
	{
		if (item->widget)
		{ // scrollbars merged with another widget
			guiItemStruct *n = item->impl->itemsMemory.createObject<guiItemStruct>(item->impl, item->entity);
			{
				guiItemStruct *i = item->firstChild;
				while (i)
				{
					CAGE_ASSERT_RUNTIME(i->parent == item);
					i->parent = n;
					i = i->nextSibling;
				}
			}
			n->parent = item;
			n->firstChild = item->firstChild;
			n->lastChild = item->lastChild;
			item->firstChild = item->lastChild = n;
			n->layout = item->layout;
			if (n->layout)
				const_cast<guiItemStruct*&>(n->layout->base) = n;
			item->layout = item->impl->itemsMemory.createObject<dummyLayoutImpl>(item);
			n->widget = item->impl->itemsMemory.createObject<scrollbarsImpl>(n);
			const_cast<entityClass*&>(n->entity) = nullptr;
		}
		else
		{ // standalone scrollbars widget
			item->widget = item->impl->itemsMemory.createObject<dummyWidgetImpl>(item);
			scrollbarsCreate(item);
		}
	}
}
