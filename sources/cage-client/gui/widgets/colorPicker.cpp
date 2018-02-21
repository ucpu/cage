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
		struct colorPickerImpl : public widgetBaseStruct
		{
			colorPickerComponent &data;
			colorPickerImpl *small, *large;

			colorPickerImpl(guiItemStruct *base, colorPickerImpl *small = nullptr) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(colorPicker)), small(small), large(nullptr)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "color picker may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "color picker may not have layout");
				CAGE_ASSERT_RUNTIME(!base->text, "color picker may not have text");
				CAGE_ASSERT_RUNTIME(!base->image, "color picker may not have image");

				if (data.collapsible)
				{
					if (small)
					{ // this is the large popup
						CAGE_ASSERT_RUNTIME(small && small != this, small, this, large);
						large = this;
					}
					else
					{ // this is the small base
						if (hasFocus())
						{ // create popup
							guiItemStruct *item = base->impl->itemsMemory.createObject<guiItemStruct>(base->impl, base->entity);
							item->attachParent(base->impl->root);
							item->widget = large = base->impl->itemsMemory.createObject<colorPickerImpl>(item, this);
							large->widgetState = widgetState;
							small = this;
						}
						else
						{ // no popup
							small = this;
							large = nullptr;
						}
					}
				}
				else
				{ // the large base directly
					small = nullptr;
					large = this;
				}
				CAGE_ASSERT_RUNTIME(small != large);
			}

			virtual void updateRequestedSize() override
			{
				if (this == large)
					base->requestedSize = skin().defaults.colorPicker.fullSize;
				else if (this == small)
					base->requestedSize = skin().defaults.colorPicker.collapsedSize;
				else
					CAGE_ASSERT_RUNTIME(false);
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				if (this == large && small)
				{ // this is a popup
					base->size = base->requestedSize;
					base->position = small->base->position + small->base->size * 0.5 - base->size * 0.5;
					base->contentSize = base->size;
					base->contentPosition = base->position;
					base->moveToWindow(true, true);
				}
			}

			virtual void emit() const override
			{
				emitElement(elementTypeEnum::ColorPickerBase, 0, vec4()); // todo this is a temporary hack
			}

			void update(vec2 point)
			{
				// todo
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				if (this == large)
					update(point);
				makeFocused();
				return true;
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (hasFocus())
					return mousePress(buttons, modifiers, point);
				return false;
			}
		};
	}

	void colorPickerCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<colorPickerImpl>(item);
	}
}
