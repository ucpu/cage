#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct comboBoxImpl;

		struct comboListImpl : public widgetItemStruct
		{
			comboBoxImpl *combo;

			comboListImpl(hierarchyItemStruct *hierarchy, comboBoxImpl *combo) : widgetItemStruct(hierarchy), combo(combo)
			{}

			virtual void initialize() override;
			virtual void findRequestedSize() override;
			virtual void findFinalPosition(const finalPositionStruct &update) override;
			virtual void emit() const override;
			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		};

		struct comboBoxImpl : public widgetItemStruct
		{
			comboBoxComponent &data;
			comboListImpl *list;
			uint32 count;
			uint32 selected;

			comboBoxImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(comboBox)), list(nullptr), count(0), selected(m)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->image, "combo box may not have image");
				CAGE_ASSERT(areChildrenValid(), "combo box children may not have other children, layouts, witgets or images and must have text");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.comboBox.placeholderFormat, hierarchy->impl);
				count = 0;
				hierarchyItemStruct *c = hierarchy->firstChild;
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
					hierarchyItemStruct *item = hierarchy->impl->itemsMemory.createObject<hierarchyItemStruct>(hierarchy->impl, hierarchy->ent);
					item->attachParent(hierarchy->impl->root);
					item->item = list = hierarchy->impl->itemsMemory.createObject<comboListImpl>(item, this);
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
				emitElement(elementTypeEnum::ComboBoxBase, mode(), p, s);
				offset(p, s, -skin->layouts[(uint32)elementTypeEnum::ComboBoxBase].border - skin->defaults.comboBox.basePadding);
				if (selected == m)
				{ // emit placeholder
					if (hierarchy->text)
						hierarchy->text->emit(p, s);
				}
				else
				{ // emit selected item
					hierarchyItemStruct *c = hierarchy->firstChild;
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
				hierarchyItemStruct *c = hierarchy->firstChild;
				while (c)
				{
					if (!c->ent)
						return false;
					if (c->item)
						return false;
					if (!c->text)
						return false;
					if (c->image)
						return false;
					if (c->firstChild)
						return false;
					c = c->nextSibling;
				}
				return true;
			}

			void consolidateSelection()
			{
				hierarchyItemStruct *c = hierarchy->firstChild;
				uint32 idx = 0;
				while (c)
				{
					if (selected == idx++)
						c->ent->add(hierarchy->impl->components.selectedItem);
					else
						c->ent->remove(hierarchy->impl->components.selectedItem);
					c = c->nextSibling;
				}
			}
		};

		void comboListImpl::initialize()
		{
			skin = combo->skin;
		}

		void comboListImpl::findRequestedSize()
		{
			hierarchy->requestedSize = vec2();
			offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)elementTypeEnum::ComboBoxList].border + skin->defaults.comboBox.listPadding);
			vec4 os = skin->layouts[(uint32)elementTypeEnum::ComboBoxItemUnchecked].border + skin->defaults.comboBox.itemPadding;
			hierarchyItemStruct *c = combo->hierarchy->firstChild;
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

		void comboListImpl::findFinalPosition(const finalPositionStruct &update)
		{
			vec4 m = skin->defaults.comboBox.baseMargin;
			hierarchy->renderSize = hierarchy->requestedSize;
			hierarchy->renderPos = combo->hierarchy->renderPos;
			hierarchy->renderPos[0] += m[0];
			hierarchy->renderPos[1] += combo->hierarchy->renderSize[1] + skin->defaults.comboBox.listOffset - m[3];
			vec2 p = hierarchy->renderPos;
			vec2 s = hierarchy->renderSize;
			offset(p, s, -skin->defaults.comboBox.baseMargin * vec4(1, 0, 1, 0) - skin->layouts[(uint32)elementTypeEnum::ComboBoxList].border - skin->defaults.comboBox.listPadding);
			real spacing = skin->defaults.comboBox.itemSpacing;
			hierarchyItemStruct *c = combo->hierarchy->firstChild;
			while (c)
			{
				c->renderPos = p;
				c->renderSize = vec2(s[0], c->requestedSize[1]);
				p[1] += c->renderSize[1] + spacing;
				c = c->nextSibling;
			}
		}

		void comboListImpl::emit() const
		{
			emitElement(elementTypeEnum::ComboBoxList, 0, hierarchy->renderPos, hierarchy->renderSize);
			vec4 itemFrame = -skin->layouts[(uint32)elementTypeEnum::ComboBoxItemUnchecked].border - skin->defaults.comboBox.itemPadding;
			hierarchyItemStruct *c = combo->hierarchy->firstChild;
			uint32 idx = 0;
			bool allowHover = true;
			while (c)
			{
				vec2 p = c->renderPos;
				vec2 s = c->renderSize;
				uint32 m = allowHover ? mode(p, s, 0) : 0;
				allowHover &= !m; // items may have small overlap, this will ensure that only one item has hover
				emitElement(idx == combo->selected ? elementTypeEnum::ComboBoxItemChecked : elementTypeEnum::ComboBoxItemUnchecked, m, p, s);
				offset(p, s, itemFrame);
				c->text->emit(p, s)->setClip(hierarchy);
				c = c->nextSibling;
				idx++;
			}
		}

		bool comboListImpl::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
		{
			makeFocused();
			if (buttons != mouseButtonsFlags::Left)
				return true;
			if (modifiers != modifiersFlags::None)
				return true;
			uint32 idx = 0;
			hierarchyItemStruct *c = combo->hierarchy->firstChild;
			while (c)
			{
				if (pointInside(c->renderPos, c->renderSize, point))
				{
					combo->data.selected = idx;
					hierarchy->impl->focusName = 0; // give up focus (this will close the popup)
					break;
				}
				idx++;
				c = c->nextSibling;
			}
			combo->consolidateSelection();
			hierarchy->fireWidgetEvent();
			return true;
		}
	}

	void comboBoxCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<comboBoxImpl>(item);
	}
}
