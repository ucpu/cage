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
	namespace
	{
		struct comboBoxImpl;

		struct comboListImpl : public widgetBaseStruct
		{
			comboBoxImpl *combo;

			comboListImpl(guiItemStruct *base, comboBoxImpl *combo) : widgetBaseStruct(base), combo(combo)
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override;
			virtual void findFinalPosition(const finalPositionStruct &update) override;
			virtual void emit() const override;
			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override;
		};

		struct comboBoxImpl : public widgetBaseStruct
		{
			comboBoxComponent &data;
			comboListImpl *list;
			uint32 count;

			comboBoxImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(comboBox)), list(nullptr), count(0)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->layout, "combo box may not have layout");
				CAGE_ASSERT_RUNTIME(!base->image, "combo box may not have image");
				CAGE_ASSERT_RUNTIME(areChildrenValid(), "combo box children may not have other children, layouts, witgets or images and must have text");
				if (base->text)
					base->text->text.apply(skin().defaults.comboBox.placeholderFormat, base->impl);
				count = 0;
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					if (count == data.selected)
						c->text->text.apply(skin().defaults.comboBox.selectedFormat, base->impl);
					else
						c->text->text.apply(skin().defaults.comboBox.itemsFormat, base->impl);
					count++;
					c = c->nextSibling;
				}
				if (data.selected >= count)
					data.selected = -1;
				consolidateSelection();
				if (hasFocus())
				{
					guiItemStruct *item = base->impl->itemsMemory.createObject<guiItemStruct>(base->impl, base->entity);
					item->attachParent(base->impl->root);
					item->widget = list = base->impl->itemsMemory.createObject<comboListImpl>(item, this);
					list->widgetState = widgetState;
				}
			}

			virtual void findRequestedSize() override
			{
				base->requestedSize = skin().defaults.comboBox.size;
				offsetSize(base->requestedSize, skin().defaults.comboBox.baseMargin);
			}

			virtual void emit() const override
			{
				vec2 p = base->position;
				vec2 s = base->size;
				offset(p, s, -skin().defaults.comboBox.baseMargin);
				emitElement(elementTypeEnum::ComboBoxBase, mode(), p, s);
				offset(p, s, -skin().layouts[(uint32)elementTypeEnum::ComboBoxBase].border - skin().defaults.comboBox.basePadding);
				if (data.selected == -1)
				{ // emit placeholder
					if (base->text)
						base->text->emit(p, s);
				}
				else
				{ // emit selected item
					guiItemStruct *c = base->firstChild;
					uint32 idx = 0;
					while (c)
					{
						if (idx++ == data.selected)
						{
							c->text->emit(p, s);
							break;
						}
						c = c->nextSibling;
					}
				}
			}

			bool areChildrenValid()
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					if (!c->entity)
						return false;
					if (!c->text)
						return false;
					if (c->widget)
						return false;
					if (c->layout)
						return false;
					if (c->image)
						return false;
					c = c->nextSibling;
				}
				return true;
			}

			void consolidateSelection()
			{
				guiItemStruct *c = base->firstChild;
				uint32 idx = 0;
				while (c)
				{
					if (data.selected == idx++)
						c->entity->add(base->impl->components.selectedItem);
					else
						c->entity->remove(base->impl->components.selectedItem);
					c = c->nextSibling;
				}
			}
		};

		void comboListImpl::findRequestedSize()
		{
			base->requestedSize = vec2();
			offsetSize(base->requestedSize, skin().layouts[(uint32)elementTypeEnum::ComboBoxList].border + skin().defaults.comboBox.listPadding);
			vec4 os = skin().layouts[(uint32)elementTypeEnum::ComboBoxItem].border + skin().defaults.comboBox.itemPadding;
			guiItemStruct *c = combo->base->firstChild;
			while (c)
			{
				// todo limit text wrap width to the combo box item
				c->requestedSize = c->text->findRequestedSize();
				offsetSize(c->requestedSize, os);
				base->requestedSize[1] += c->requestedSize[1];
				c = c->nextSibling;
			}
			base->requestedSize[1] += skin().defaults.comboBox.itemSpacing * (max(combo->count, 1u) - 1);
			vec4 m = skin().defaults.comboBox.baseMargin;
			base->requestedSize[0] = combo->base->requestedSize[0] - m[0] - m[2];
		}

		void comboListImpl::findFinalPosition(const finalPositionStruct &update)
		{
			vec4 m = skin().defaults.comboBox.baseMargin;
			base->size = base->requestedSize;
			base->position = combo->base->position;
			base->position[0] += m[0];
			base->position[1] += combo->base->size[1] + skin().defaults.comboBox.listOffset - m[3];
			vec2 p = base->position;
			vec2 s = base->size;
			offset(p, s, -skin().defaults.comboBox.baseMargin * vec4(1, 0, 1, 0) - skin().layouts[(uint32)elementTypeEnum::ComboBoxList].border - skin().defaults.comboBox.listPadding);
			real spacing = skin().defaults.comboBox.itemSpacing;
			guiItemStruct *c = combo->base->firstChild;
			while (c)
			{
				c->position = p;
				c->size = vec2(s[0], c->requestedSize[1]);
				p[1] += c->size[1] + spacing;
				c = c->nextSibling;
			}
		}

		void comboListImpl::emit() const
		{
			emitElement(elementTypeEnum::ComboBoxList, 0, base->position, base->size);
			vec4 itemFrame = -skin().layouts[(uint32)elementTypeEnum::ComboBoxItem].border - skin().defaults.comboBox.itemPadding;
			guiItemStruct *c = combo->base->firstChild;
			uint32 idx = 0;
			while (c)
			{
				uint32 m = pointInside(c->position, c->size, base->impl->outputMouse) ? 2 : idx == combo->data.selected ? 1 : 0;
				emitElement(elementTypeEnum::ComboBoxItem, m, c->position, c->size);
				vec2 p = c->position;
				vec2 s = c->size;
				offset(p, s, itemFrame);
				c->text->emit(p, s);
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
			guiItemStruct *c = combo->base->firstChild;
			while (c)
			{
				if (pointInside(c->position, c->size, point))
				{
					combo->data.selected = idx;
					base->impl->focusName = 0; // give up focus (this will close the popup)
					break;
				}
				idx++;
				c = c->nextSibling;
			}
			combo->consolidateSelection();
			base->impl->widgetEvent.dispatch(base->entity->name());
			return true;
		}
	}

	void comboBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<comboBoxImpl>(item);
	}
}
