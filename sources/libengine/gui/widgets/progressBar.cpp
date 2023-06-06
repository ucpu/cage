#include "../private.h"

namespace cage
{
	void textCreate(HierarchyItem *item);
	void imageCreate(HierarchyItem *item);

	namespace
	{
		struct ProgressBarImpl : public WidgetItem
		{
			GuiProgressBarComponent &data;

			ProgressBarImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ProgressBar)) { data.progress = saturate(data.progress); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT(data.progress.valid());

				{
					imageCreate(hierarchy);
					hierarchy->image->skipInitialize = true;
					GuiImageComponent i;
					i.textureName = skin->defaults.progressBar.fillingTextureName;
					hierarchy->image->assign(i);
					hierarchy->image->format.animationSpeed = 0;
					hierarchy->image->format.animationOffset = data.progress;
				}

				if (data.showValue && !hierarchy->text)
				{
					textCreate(hierarchy);
				}
				if (hierarchy->text)
				{
					hierarchy->text->skipInitialize = true;
					hierarchy->text->apply(skin->defaults.progressBar.textFormat);
					if (data.showValue)
						hierarchy->text->transcript(Stringizer() + numeric_cast<uint32>(data.progress * 100));
					else
						hierarchy->text->transcript();
				}
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.progressBar.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.progressBar.baseMargin);
			}

			void emit() override
			{
				const ElementModeEnum md = widgetState.disabled ? ElementModeEnum::Disabled : ElementModeEnum::Default;
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.progressBar.baseMargin);
					emitElement(GuiElementTypeEnum::ProgressBar, md, p, s);
				}
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.progressBar.baseMargin - skin->layouts[(uint32)GuiElementTypeEnum::ProgressBar].border - skin->defaults.progressBar.fillingPadding);
					hierarchy->image->emit(p, s);
				}
				if (hierarchy->text)
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.progressBar.baseMargin - skin->layouts[(uint32)GuiElementTypeEnum::ProgressBar].border - skin->defaults.progressBar.textPadding);
					hierarchy->text->emit(p, s);
				}
			}
		};
	}

	void ProgressBarCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ProgressBarImpl>(item).cast<BaseItem>();
	}
}
