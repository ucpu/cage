#include "../private.h"

namespace cage
{
	namespace
	{
		struct PanelImpl : public WidgetItem
		{
			PanelImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{
				ensureItemHasLayout(hierarchy);
			}

			virtual void initialize() override
			{
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.panel.textFormat);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->children[0]->findRequestedSize();
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				offsetSize(hierarchy->requestedSize, skin->defaults.panel.contentPadding);
				if (hierarchy->text)
				{
					hierarchy->requestedSize[1] += skin->defaults.panel.captionHeight;
					vec2 cs = hierarchy->text->findRequestedSize();
					offsetSize(cs, skin->defaults.panel.captionPadding);
					hierarchy->requestedSize[0] = max(hierarchy->requestedSize[0], cs[0]);
					// it is important to compare (text size + text padding) with (children size + children padding)
					// and only after that to add border and base margin
				}
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::PanelBase].border);
				offsetSize(hierarchy->requestedSize, skin->defaults.panel.baseMargin);
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				FinalPosition u(update);
				offset(u.renderPos, u.renderSize, -skin->defaults.panel.baseMargin);
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)GuiElementTypeEnum::PanelBase].border);
				if (hierarchy->text)
				{
					u.renderPos[1] += skin->defaults.panel.captionHeight;
					u.renderSize[1] -= skin->defaults.panel.captionHeight;
				}
				offset(u.renderPos, u.renderSize, -skin->defaults.panel.contentPadding);
				hierarchy->children[0]->findFinalPosition(u);
			}

			virtual void emit() override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.panel.baseMargin);
				emitElement(GuiElementTypeEnum::PanelBase, mode(false, 0), p, s);
				if (hierarchy->text)
				{
					s = vec2(s[0], skin->defaults.panel.captionHeight);
					emitElement(GuiElementTypeEnum::PanelCaption, mode(false, 0), p, s);
					offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::PanelCaption].border);
					offset(p, s, -skin->defaults.panel.captionPadding);
					hierarchy->text->emit(p, s);
				}
				hierarchy->childrenEmit();
			}
		};
	}

	void PanelCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<PanelImpl>(item).cast<BaseItem>();
	}
}
