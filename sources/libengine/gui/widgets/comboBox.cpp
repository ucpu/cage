#include "../private.h"

namespace cage
{
	namespace
	{
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

		struct ComboBoxImpl : public WidgetItem
		{
			GuiComboBoxComponent &data;
			ComboListImpl *list = nullptr;
			uint32 count = 0;
			uint32 selected = m;

			ComboBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ComboBox)) {}

			void initialize() override
			{
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT(areChildrenValid());
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.comboBox.placeholderFormat);
				count = 0;
				for (const auto &c : hierarchy->children)
				{
					if (count == data.selected)
						c->text->apply(skin->defaults.comboBox.selectedFormat);
					else
						c->text->apply(skin->defaults.comboBox.itemsFormat);
					count++;
				}
				if (data.selected >= count)
					data.selected = m;
				selected = data.selected;
				consolidateSelection();
				if (hasFocus())
				{
					Holder<HierarchyItem> item = hierarchy->impl->memory->createHolder<HierarchyItem>(hierarchy->impl, hierarchy->ent);
					item->item = hierarchy->impl->memory->createHolder<ComboListImpl>(+item, this).cast<BaseItem>();
					list = class_cast<ComboListImpl *>(+item->item);
					list->widgetState = widgetState;
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
						hierarchy->text->emit(p, s);
				}
				else
				{ // emit selected item
					uint32 idx = 0;
					for (const auto &c : hierarchy->children)
					{
						if (idx++ == selected)
						{
							c->text->emit(p, s).setClip(hierarchy);
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

			void consolidateSelection()
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
		};

		void ComboListImpl::initialize()
		{
			skin = combo->skin;
		}

		void ComboListImpl::findRequestedSize()
		{
			hierarchy->requestedSize = Vec2();
			offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border + skin->defaults.comboBox.listPadding);
			const Vec4 os = skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border + skin->defaults.comboBox.itemPadding;
			for (const auto &c : combo->hierarchy->children)
			{
				// todo limit text wrap width to the combo box item
				c->requestedSize = c->text->findRequestedSize();
				offsetSize(c->requestedSize, os);
				hierarchy->requestedSize[1] += c->requestedSize[1];
			}
			hierarchy->requestedSize[1] += skin->defaults.comboBox.itemSpacing * (max(combo->count, 1u) - 1);
			const Vec4 margin = skin->defaults.comboBox.baseMargin;
			hierarchy->requestedSize[0] = combo->hierarchy->requestedSize[0] - margin[0] - margin[2];
		}

		void ComboListImpl::findFinalPosition(const FinalPosition &update)
		{
			const Vec4 margin = skin->defaults.comboBox.baseMargin;
			const Real spacing = skin->defaults.comboBox.itemSpacing;
			hierarchy->renderSize = hierarchy->requestedSize;
			hierarchy->renderPos = combo->hierarchy->renderPos;
			hierarchy->renderPos[0] += margin[0];
			hierarchy->renderPos[1] += combo->hierarchy->renderSize[1] + skin->defaults.comboBox.listOffset - margin[3];
			Vec2 p = hierarchy->renderPos;
			Vec2 s = hierarchy->renderSize;
			offset(p, s, -skin->defaults.comboBox.baseMargin * Vec4(1, 0, 1, 0) - skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border - skin->defaults.comboBox.listPadding);
			for (const auto &c : combo->hierarchy->children)
			{
				c->renderPos = p;
				c->renderSize = Vec2(s[0], c->requestedSize[1]);
				p[1] += c->renderSize[1] + spacing;
			}
		}

		void ComboListImpl::emit()
		{
			emitElement(GuiElementTypeEnum::ComboBoxList, ElementModeEnum::Default, hierarchy->renderPos, hierarchy->renderSize);
			const Vec4 itemFrame = -skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border - skin->defaults.comboBox.itemPadding;
			uint32 idx = 0;
			bool allowHover = true;
			for (const auto &c : combo->hierarchy->children)
			{
				Vec2 p = c->renderPos;
				Vec2 s = c->renderSize;
				const ElementModeEnum md = allowHover ? mode(p, s, 0) : ElementModeEnum::Default;
				allowHover &= md == ElementModeEnum::Default; // items may have small overlap, this will ensure that only one item has hover
				const bool disabled = c->ent->has<GuiWidgetStateComponent>() && c->ent->value<GuiWidgetStateComponent>().disabled;
				emitElement(idx == combo->selected ? GuiElementTypeEnum::ComboBoxItemChecked : GuiElementTypeEnum::ComboBoxItemUnchecked, disabled ? ElementModeEnum::Disabled : md, p, s);
				offset(p, s, itemFrame);
				c->text->emit(p, s).setClip(hierarchy);
				idx++;
			}
		}

		bool ComboListImpl::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
		{
			makeFocused();
			if (buttons != MouseButtonsFlags::Left)
				return true;
			if (modifiers != ModifiersFlags::None)
				return true;
			uint32 idx = 0;
			HierarchyItem *newlySelected = nullptr;
			for (const auto &c : combo->hierarchy->children)
			{
				const bool disabled = c->ent->has<GuiWidgetStateComponent>() && c->ent->value<GuiWidgetStateComponent>().disabled;
				if (!disabled && pointInside(c->renderPos, c->renderSize, point))
				{
					combo->data.selected = idx;
					newlySelected = +c;
					hierarchy->impl->focusName = 0; // give up focus (this will close the popup)
					break;
				}
				idx++;
			}
			combo->consolidateSelection();
			if (newlySelected)
			{
				newlySelected->fireWidgetEvent();
				hierarchy->fireWidgetEvent();
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
