#include "../private.h"

namespace cage
{
	namespace
	{
		struct ComboBoxImpl;

		struct ComboListImpl : public WidgetItem
		{
			ComboBoxImpl *combo;

			ComboListImpl(HierarchyItem *hierarchy, ComboBoxImpl *combo) : WidgetItem(hierarchy), combo(combo)
			{}

			virtual void initialize() override;
			virtual void findRequestedSize() override;
			virtual void findFinalPosition(const FinalPosition &update) override;
			virtual void emit() const override;
			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override;
		};

		struct ComboBoxImpl : public WidgetItem
		{
			GuiComboBoxComponent &data;
			ComboListImpl *list;
			uint32 count;
			uint32 selected;

			ComboBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ComboBox)), list(nullptr), count(0), selected(m)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->Image, "combo box may not have image");
				CAGE_ASSERT(areChildrenValid(), "combo box children may not have other children, layouts, witgets or images and must have text");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.comboBox.placeholderFormat, hierarchy->impl);
				count = 0;
				HierarchyItem *c = hierarchy->firstChild;
				while (c)
				{
					if (count == data.selected)
						c->text->text.apply(skin->defaults.comboBox.selectedFormat, hierarchy->impl);
					else
						c->text->text.apply(skin->defaults.comboBox.itemsFormat, hierarchy->impl);
					count++;
					c = c->nextSibling;
				}
				if (data.selected >= count)
					data.selected = m;
				selected = data.selected;
				consolidateSelection();
				if (hasFocus())
				{
					HierarchyItem *item = hierarchy->impl->itemsMemory.createObject<HierarchyItem>(hierarchy->impl, hierarchy->ent);
					item->attachParent(hierarchy->impl->root);
					item->item = list = hierarchy->impl->itemsMemory.createObject<ComboListImpl>(item, this);
					list->widgetState = widgetState;
				}
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.comboBox.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.comboBox.baseMargin);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
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
					HierarchyItem *c = hierarchy->firstChild;
					uint32 idx = 0;
					while (c)
					{
						if (idx++ == selected)
						{
							c->text->emit(p, s)->setClip(hierarchy);
							break;
						}
						c = c->nextSibling;
					}
				}
			}

			bool areChildrenValid()
			{
				HierarchyItem *c = hierarchy->firstChild;
				while (c)
				{
					if (!c->ent)
						return false;
					if (c->item)
						return false;
					if (!c->text)
						return false;
					if (c->Image)
						return false;
					if (c->firstChild)
						return false;
					c = c->nextSibling;
				}
				return true;
			}

			void consolidateSelection()
			{
				HierarchyItem *c = hierarchy->firstChild;
				EntityComponent *sel = hierarchy->impl->components.SelectedItem;
				uint32 idx = 0;
				while (c)
				{
					if (selected == idx++)
						c->ent->add(sel);
					else
						c->ent->remove(sel);
					c = c->nextSibling;
				}
			}
		};

		void ComboListImpl::initialize()
		{
			skin = combo->skin;
		}

		void ComboListImpl::findRequestedSize()
		{
			hierarchy->requestedSize = vec2();
			offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border + skin->defaults.comboBox.listPadding);
			vec4 os = skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border + skin->defaults.comboBox.itemPadding;
			HierarchyItem *c = combo->hierarchy->firstChild;
			while (c)
			{
				// todo limit text wrap width to the combo box item
				c->requestedSize = c->text->findRequestedSize();
				offsetSize(c->requestedSize, os);
				hierarchy->requestedSize[1] += c->requestedSize[1];
				c = c->nextSibling;
			}
			hierarchy->requestedSize[1] += skin->defaults.comboBox.itemSpacing * (max(combo->count, 1u) - 1);
			vec4 m = skin->defaults.comboBox.baseMargin;
			hierarchy->requestedSize[0] = combo->hierarchy->requestedSize[0] - m[0] - m[2];
		}

		void ComboListImpl::findFinalPosition(const FinalPosition &update)
		{
			vec4 m = skin->defaults.comboBox.baseMargin;
			hierarchy->renderSize = hierarchy->requestedSize;
			hierarchy->renderPos = combo->hierarchy->renderPos;
			hierarchy->renderPos[0] += m[0];
			hierarchy->renderPos[1] += combo->hierarchy->renderSize[1] + skin->defaults.comboBox.listOffset - m[3];
			vec2 p = hierarchy->renderPos;
			vec2 s = hierarchy->renderSize;
			offset(p, s, -skin->defaults.comboBox.baseMargin * vec4(1, 0, 1, 0) - skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxList].border - skin->defaults.comboBox.listPadding);
			real spacing = skin->defaults.comboBox.itemSpacing;
			HierarchyItem *c = combo->hierarchy->firstChild;
			while (c)
			{
				c->renderPos = p;
				c->renderSize = vec2(s[0], c->requestedSize[1]);
				p[1] += c->renderSize[1] + spacing;
				c = c->nextSibling;
			}
		}

		void ComboListImpl::emit() const
		{
			emitElement(GuiElementTypeEnum::ComboBoxList, 0, hierarchy->renderPos, hierarchy->renderSize);
			vec4 itemFrame = -skin->layouts[(uint32)GuiElementTypeEnum::ComboBoxItemUnchecked].border - skin->defaults.comboBox.itemPadding;
			HierarchyItem *c = combo->hierarchy->firstChild;
			uint32 idx = 0;
			bool allowHover = true;
			while (c)
			{
				vec2 p = c->renderPos;
				vec2 s = c->renderSize;
				uint32 m = allowHover ? mode(p, s, 0) : 0;
				allowHover &= !m; // items may have small overlap, this will ensure that only one item has hover
				emitElement(idx == combo->selected ? GuiElementTypeEnum::ComboBoxItemChecked : GuiElementTypeEnum::ComboBoxItemUnchecked, m, p, s);
				offset(p, s, itemFrame);
				c->text->emit(p, s)->setClip(hierarchy);
				c = c->nextSibling;
				idx++;
			}
		}

		bool ComboListImpl::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
		{
			makeFocused();
			if (buttons != MouseButtonsFlags::Left)
				return true;
			if (modifiers != ModifiersFlags::None)
				return true;
			uint32 idx = 0;
			HierarchyItem *c = combo->hierarchy->firstChild;
			HierarchyItem *newlySelected = nullptr;
			while (c)
			{
				if (pointInside(c->renderPos, c->renderSize, point))
				{
					combo->data.selected = idx;
					newlySelected = c;
					hierarchy->impl->focusName = 0; // give up focus (this will close the popup)
					break;
				}
				idx++;
				c = c->nextSibling;
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
		item->item = item->impl->itemsMemory.createObject<ComboBoxImpl>(item);
	}
}
