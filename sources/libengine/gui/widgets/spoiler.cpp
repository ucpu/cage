#include "../private.h"

namespace cage
{
	namespace
	{
		struct SpoilerImpl : public WidgetItem
		{
			GuiSpoilerComponent &data;
			bool collapsed = false;

			SpoilerImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Spoiler))
			{
				ensureItemHasLayout(hierarchy);
			}

			virtual void initialize() override
			{
				collapsed = data.collapsed;
				if (collapsed)
					hierarchy->children.clear();
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.spoiler.textFormat);
			}

			virtual void findRequestedSize() override
			{
				if (!hierarchy->children.empty())
				{
					hierarchy->children[0]->findRequestedSize();
					hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				}
				else
					hierarchy->requestedSize = Vec2();
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.contentPadding);
				hierarchy->requestedSize[1] += skin->defaults.spoiler.captionHeight;
				Vec2 cs = hierarchy->text ? hierarchy->text->findRequestedSize() : Vec2();
				offsetSize(cs, skin->defaults.spoiler.captionPadding);
				hierarchy->requestedSize[0] = max(hierarchy->requestedSize[0], cs[0] + skin->defaults.spoiler.captionHeight);
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border);
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.baseMargin);
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				if (hierarchy->children.empty())
					return;
				FinalPosition u(update);
				u.renderPos[1] += skin->defaults.spoiler.captionHeight;
				u.renderSize[1] -= skin->defaults.spoiler.captionHeight;
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border);
				offset(u.renderPos, u.renderSize, -skin->defaults.spoiler.baseMargin - skin->defaults.spoiler.contentPadding);
				hierarchy->children[0]->findFinalPosition(u);
			}

			virtual void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.spoiler.baseMargin);
				emitElement(GuiElementTypeEnum::SpoilerBase, mode(false), p, s);
				s = Vec2(s[0], skin->defaults.spoiler.captionHeight);
				emitElement(GuiElementTypeEnum::SpoilerCaption, mode(p, s), p, s);
				offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerCaption].border - skin->defaults.spoiler.captionPadding);
				Vec2 is = Vec2(s[1], s[1]);
				Vec2 ip = Vec2(p[0] + s[0] - is[0], p[1]);
				emitElement(collapsed ? GuiElementTypeEnum::SpoilerIconCollapsed : GuiElementTypeEnum::SpoilerIconShown, mode(false, 0), ip, is);
				if (hierarchy->text)
				{
					s[0] -= s[1];
					s[0] = max(0, s[0]);
					hierarchy->text->emit(p, s);
				}
				hierarchy->childrenEmit();
			}

			void collapse(HierarchyItem *item)
			{
				if (SpoilerImpl *b = dynamic_cast<SpoilerImpl *>(+item->item))
					b->data.collapsed = true;
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				hierarchy->impl->focusName = 0;
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				Vec2 p = hierarchy->renderPos;
				Vec2 s = Vec2(hierarchy->renderSize[0], skin->defaults.spoiler.captionHeight);
				offset(p, s, -skin->defaults.spoiler.baseMargin * Vec4(1, 1, 1, 0));
				if (pointInside(p, s, point))
				{
					data.collapsed = !data.collapsed;
					if (data.collapsesSiblings)
					{
						HierarchyItem *parent = hierarchy->impl->root->findParentOf(hierarchy);
						for (const auto &it : parent->children)
						{
							if (+it == hierarchy)
								continue;
							collapse(+it);
						}
					}
				}
				return true;
			}
		};
	}

	void SpoilerCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SpoilerImpl>(item).cast<BaseItem>();
	}
}
