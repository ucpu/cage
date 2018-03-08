#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
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

			virtual void updateRequestedSize() override;
			virtual void updateFinalPosition(const updatePositionStruct &update) override;
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

			virtual void updateRequestedSize() override
			{
				base->requestedSize = skin().defaults.comboBox.size;
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->updateContentPosition(skin().defaults.comboBox.baseMargin);
			}

			virtual void emit() const override
			{
				emitElement(elementTypeEnum::ComboBoxBase, 0);
				vec2 p = base->contentPosition;
				vec2 s = base->contentSize;
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
						c->entity->addComponent(base->impl->components.selectedItem);
					else
						c->entity->removeComponent(base->impl->components.selectedItem);
					c = c->nextSibling;
				}
			}
		};

		void comboListImpl::updateRequestedSize()
		{
			base->requestedSize = vec2(combo->base->requestedSize[0], 0);
			guiItemStruct *c = combo->base->firstChild;
			while (c)
			{
				c->requestedSize = c->text->updateRequestedSize();
				base->requestedSize[1] += c->requestedSize[1];
				c = c->nextSibling;
			}
			vec4 os = skin().layouts[(uint32)elementTypeEnum::ComboBoxItem].border + skin().defaults.comboBox.itemPadding;
			base->requestedSize[1] += (os[1] + os[3]) * combo->count;
			base->requestedSize[1] += skin().defaults.comboBox.itemSpacing * (max(combo->count, 1u) - 1);
			vec4 lp = skin().layouts[(uint32)elementTypeEnum::ComboBoxList].border + skin().defaults.comboBox.listPadding;
			base->requestedSize[1] += lp[1] + lp[3];
		}

		void comboListImpl::updateFinalPosition(const updatePositionStruct &update)
		{
			base->size = base->requestedSize;
			base->position = combo->base->position;
			base->position[1] += combo->base->size[1] + skin().defaults.comboBox.listOffset;
			base->updateContentPosition(skin().defaults.comboBox.baseMargin * vec4(1, 0, 1, 0));
			vec2 p = base->contentPosition;
			vec2 s = base->contentSize;
			offset(p, s, -skin().layouts[(uint32)elementTypeEnum::ComboBoxList].border - skin().defaults.comboBox.listPadding);
			guiItemStruct *c = combo->base->firstChild;
			vec4 os = skin().layouts[(uint32)elementTypeEnum::ComboBoxItem].border + skin().defaults.comboBox.itemPadding;
			while (c)
			{
				c->position = p;
				c->size = vec2(s[0], c->requestedSize[1] + os[1] + os[3]);
				c->contentPosition = vec2(p[0] + os[0], p[1] + os[1]);
				c->contentSize = vec2(s[0] - os[0] - os[2], c->requestedSize[1]);
				p[1] += c->size[1] + skin().defaults.comboBox.itemSpacing;
				c = c->nextSibling;
			}
		}

		void comboListImpl::emit() const
		{
			emitElement(elementTypeEnum::ComboBoxList, 0);
			guiItemStruct *c = combo->base->firstChild;
			uint32 idx = 0;
			while (c)
			{
				emitElement(idx++ == combo->data.selected ? elementTypeEnum::ComboBoxSelectedItem : elementTypeEnum::ComboBoxItem, 0, c->position, c->size);
				c->text->emit();
				c = c->nextSibling;
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
					base->impl->focusName = 0; // give up focus
					break;
				}
				idx++;
				c = c->nextSibling;
			}
			combo->consolidateSelection();
			return true;
		}
	}

	void comboBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<comboBoxImpl>(item);
	}
}
