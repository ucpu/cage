#include "../private.h"

namespace cage
{
	namespace
	{
		void consolidateSelection(HierarchyItem *hierarchy, uint32 selected)
		{
			EntityComponent *sel = hierarchy->impl->entityMgr->component<GuiSelectedItemComponent>();
			uint32 idx = 0;
			for (const auto &c : hierarchy->children)
			{
				if (selected == idx++)
					c->ent->add(sel);
				else
					c->ent->remove(sel);
			}
		}

		struct ComboBoxImpl;

		struct ComboListImpl : public WidgetItem
		{
			ComboBoxImpl *combo = nullptr;

			ComboListImpl(HierarchyItem *hierarchy, ComboBoxImpl *combo) : WidgetItem(hierarchy), combo(combo) {}

			void initialize() override;
			void findRequestedSize() override;
			void findFinalPosition(const FinalPosition &update) override;
			void emit() override;
			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		};

		struct ComboOptionImpl : public WidgetItem
		{
			ComboBoxImpl *combo = nullptr;
			uint32 index = m;

			ComboOptionImpl(HierarchyItem *hierarchy, ComboBoxImpl *combo, uint32 index) : WidgetItem(hierarchy), combo(combo), index(index) {}

			void initialize() override;
			void findRequestedSize() override;
			void emit() override;
			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override;
		};

		struct ComboBoxImpl : public WidgetItem
		{
			GuiComboBoxComponent &data;
			ComboListImpl *list = nullptr;
			uint32 count = 0;
			uint32 selected = m; // must keep copy of data.selected here because data may not be access while emitting

			ComboBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ComboBox)) {}

			void initialize() override
			{
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT(areChildrenValid());
				count = numeric_cast<uint32>(hierarchy->children.size());
				if (data.selected >= count)
					data.selected = m;
				selected = data.selected;
				consolidateSelection(hierarchy, selected);
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.comboBox.placeholderFormat);
				for (const auto &c : hierarchy->children)
					c->text->apply(skin->defaults.comboBox.selectedFormat);
				if (hasFocus())
				{
					Holder<HierarchyItem> item = hierarchy->impl->memory->createHolder<HierarchyItem>(hierarchy->impl, hierarchy->ent);
					item->item = hierarchy->impl->memory->createHolder<ComboListImpl>(+item, this).cast<BaseItem>();
					list = class_cast<ComboListImpl *>(+item->item);
					list->widgetState = widgetState;
					list->skin = skin;
					hierarchy->impl->root->children.push_back(std::move(item));
				}
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.comboBox.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.comboBox.baseMargin);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.comboBox.baseMargin);
				emitElement(GuiElementTypeEnum::ComboBoxBase, mode(), p, s);
				offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxBase].border - skin->defaults.comboBox.basePadding);
				if (selected == m)
				{ // emit placeholder
					if (hierarchy->text)
						hierarchy->text->emit(p, s, widgetState.disabled);
				}
				else
				{ // emit selected item
					uint32 idx = 0;
					for (const auto &c : hierarchy->children)
					{
						if (idx++ == selected)
						{
							c->text->emit(p, s, widgetState.disabled).setClip(hierarchy);
							break;
						}
					}
				}
			}

			bool areChildrenValid()
			{
				for (const auto &c : hierarchy->children)
				{
					if (!c->ent)
						return false;
					if (c->item)
						return false;
					if (!c->text)
						return false;
					if (c->image)
						return false;
					if (!c->children.empty())
						return false;
				}
				return true;
			}
		};

		// list

		void ComboListImpl::initialize()
		{
			uint32 idx = 0;
			for (const auto &c : combo->hierarchy->children)
			{
				Holder<HierarchyItem> h = hierarchy->impl->memory->createHolder<HierarchyItem>(c->impl, c->ent);
				h->item = hierarchy->impl->memory->createHolder<ComboOptionImpl>(+h, combo, idx++).cast<BaseItem>();
				auto opt = class_cast<ComboOptionImpl *>(+h->item);
				opt->widgetState = widgetState;
				opt->skin = skin;
				h->text = c->text.share();
				const_cast<HierarchyItem *&>(h->text->hierarchy) = +h;
				const_cast<Entity *&>(c->ent) = nullptr;
				hierarchy->children.push_back(std::move(h));
			}
		}

		void ComboListImpl::findRequestedSize()
		{
			hierarchy->requestedSize = Vec2();
			offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border + skin->defaults.comboBox.listPadding);
			for (const auto &c : hierarchy->children)
			{
				c->findRequestedSize();
				hierarchy->requestedSize[1] += c->requestedSize[1];
			}
			hierarchy->requestedSize[1] += skin->defaults.comboBox.itemSpacing * (max(combo->count, 1u) - 1);
			const Vec4 margin = skin->defaults.comboBox.baseMargin;
			hierarchy->requestedSize[0] = combo->hierarchy->requestedSize[0] - margin[0] - margin[2];
			CAGE_ASSERT(hierarchy->requestedSize.valid());
		}

		void ComboListImpl::findFinalPosition(const FinalPosition &update)
		{
			hierarchy->renderSize = hierarchy->requestedSize;
			hierarchy->renderPos = combo->hierarchy->renderPos;
			const Vec4 margin = skin->defaults.comboBox.baseMargin;
			hierarchy->renderPos[0] += margin[0];
			hierarchy->renderPos[1] += combo->hierarchy->renderSize[1] + skin->defaults.comboBox.listOffset - margin[3];
			CAGE_ASSERT(hierarchy->renderSize.valid());
			CAGE_ASSERT(hierarchy->renderPos.valid());

			const Real spacing = skin->defaults.comboBox.itemSpacing;
			Vec2 p = hierarchy->renderPos;
			Vec2 s = hierarchy->renderSize;
			offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border - skin->defaults.comboBox.listPadding);
			for (const auto &c : hierarchy->children)
			{
				FinalPosition u(update);
				u.renderPos = p;
				u.renderSize = Vec2(s[0], c->requestedSize[1]);
				c->findFinalPosition(u);
				p[1] += c->requestedSize[1] + spacing;
			}
		}

		void ComboListImpl::emit()
		{
			emitElement(GuiElementTypeEnum::ComboBoxList, ElementModeEnum::Default, hierarchy->renderPos, hierarchy->renderSize);
			hierarchy->childrenEmit();
		}

		bool ComboListImpl::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
		{
			// does not take focus
			return true;
		}

		// option

		void ComboOptionImpl::initialize()
		{
			if (hierarchy->ent->has<GuiWidgetStateComponent>() && hierarchy->ent->value<GuiWidgetStateComponent>().disabled)
				widgetState.disabled = true;
			hierarchy->text->apply(index == combo->selected ? skin->defaults.comboBox.selectedFormat : skin->defaults.comboBox.itemsFormat);
		}

		void ComboOptionImpl::findRequestedSize()
		{
			hierarchy->requestedSize = hierarchy->text->findRequestedSize();
			const Vec4 itemFrame = skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border + skin->defaults.comboBox.itemPadding;
			offsetSize(hierarchy->requestedSize, itemFrame);
		}

		void ComboOptionImpl::emit()
		{
			Vec2 p = hierarchy->renderPos;
			Vec2 s = hierarchy->renderSize;
			emitElement(index == combo->selected ? GuiElementTypeEnum::ComboBoxItemChecked : GuiElementTypeEnum::ComboBoxItemUnchecked, mode(), p, s);
			const Vec4 itemFrame = -skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border - skin->defaults.comboBox.itemPadding;
			offset(p, s, itemFrame);
			hierarchy->text->emit(p, s, widgetState.disabled);
		}

		bool ComboOptionImpl::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
		{
			// does not take focus
			if (buttons != MouseButtonsFlags::Left)
				return true;
			if (modifiers != ModifiersFlags::None)
				return true;
			if (!widgetState.disabled && pointInside(hierarchy->renderPos, hierarchy->renderSize, point))
			{
				combo->data.selected = index;
				combo->selected = index;
				consolidateSelection(combo->list->hierarchy, combo->selected);
				hierarchy->impl->focusName = 0; // give up focus (this will close the popup)
				hierarchy->fireWidgetEvent();
				combo->hierarchy->fireWidgetEvent();
			}
			return true;
		}
	}

	void ComboBoxCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ComboBoxImpl>(item).cast<BaseItem>();
	}
}
