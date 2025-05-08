#include "../private.h"

namespace cage
{
	namespace
	{
		struct SpoilerImpl : public WidgetItem
		{
			GuiSpoilerComponent &data;
			bool collapsed = false; // must keep copy of data.collapsed here because data is not accessible while emitting

			SpoilerImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Spoiler)) { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->image);
				collapsed = data.collapsed;
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.spoiler.textFormat);
			}

			void findRequestedSize(Real maxWidth) override
			{
				if (!hierarchy->children.empty())
				{
					hierarchy->children[0]->findRequestedSize(maxWidth);
					hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				}
				if (collapsed)
				{
					hierarchy->children.clear();
					hierarchy->requestedSize[1] = 0;
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.contentPadding);
				hierarchy->requestedSize[1] += skin->defaults.spoiler.captionHeight;
				{
					Vec2 cs = hierarchy->text ? hierarchy->text->findRequestedSize() : Vec2();
					offsetSize(cs, skin->defaults.spoiler.captionPadding);
					hierarchy->requestedSize[0] = max(hierarchy->requestedSize[0], cs[0] + skin->defaults.spoiler.captionHeight);
				}
				const Real k = hierarchy->children.empty() ? 0 : 1;
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border * Vec4(1, k, 1, k));
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.baseMargin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				if (collapsed)
					return;
				FinalPosition u(update);
				u.renderPos[1] += skin->defaults.spoiler.captionHeight;
				u.renderSize[1] -= skin->defaults.spoiler.captionHeight;
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border);
				offset(u.renderPos, u.renderSize, -skin->defaults.spoiler.baseMargin - skin->defaults.spoiler.contentPadding);
				hierarchy->children[0]->findFinalPosition(u);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.spoiler.baseMargin);
				if (!collapsed)
					emitElement(GuiElementTypeEnum::SpoilerBase, mode(false), p, s);
				s[1] = skin->defaults.spoiler.captionHeight;
				emitElement(GuiElementTypeEnum::SpoilerCaption, mode(p, s), p, s);
				offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerCaption].border - skin->defaults.spoiler.captionPadding);
				{
					const Vec2 is = Vec2(s[1], s[1]);
					const Vec2 ip = Vec2(p[0] + s[0] - is[0], p[1]);
					emitElement(collapsed ? GuiElementTypeEnum::SpoilerIconCollapsed : GuiElementTypeEnum::SpoilerIconShown, mode(false, 0), ip, is);
				}
				if (hierarchy->text)
				{
					s[0] -= s[1];
					s[0] = max(0, s[0]);
					hierarchy->text->emit(p, s, widgetState.disabled);
				}
				hierarchy->childrenEmit();
			}

			static void collapse(HierarchyItem *item)
			{
				if (SpoilerImpl *b = dynamic_cast<SpoilerImpl *>(+item->item))
					b->data.collapsed = true;
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				CAGE_ASSERT(buttons != MouseButtonsFlags::None);
				hierarchy->impl->focusName = 0;
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
					play(skin->defaults.spoiler.clickSound);
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
