#include "scrollableBase.h"

namespace cage
{
	void layoutDefaultCreate(guiItemStruct *item);

	scrollableBaseStruct::scrollableBaseStruct(guiItemStruct * base) : widgetBaseStruct(base), scrollable(nullptr), defaults(nullptr), elementBase(elementTypeEnum::InvalidElement), elementCaption(elementTypeEnum::InvalidElement)
	{}

	void scrollableBaseStruct::scrollableInitialize()
	{
		CAGE_ASSERT_RUNTIME(scrollable);
		CAGE_ASSERT_RUNTIME(defaults);
		if (base->text)
			base->text->text.apply(defaults->textFormat, base->impl);
		if (!base->layout)
			layoutDefaultCreate(base);
	}

	void scrollableBaseStruct::scrollableUpdateRequestedSize()
	{
		base->layout->updateRequestedSize();
		if (base->text)
		{
			vec2 cs;
			base->text->updateRequestedSize(cs);
			base->requestedSize[0] = max(base->requestedSize[0], cs[0]);
		}
		frame = vec4();
		if (elementBase != elementTypeEnum::InvalidElement)
		{
			frame += skin().layouts[(uint32)elementBase].border;
		}
		if (elementCaption != elementTypeEnum::InvalidElement)
		{
			frame[1] += defaults->captionHeight;
		}
		frame += defaults->baseMargin;
		frame += defaults->contentPadding;
		// todo scrollbars
		offsetSize(base->requestedSize, frame);
	}

	void scrollableBaseStruct::scrollableUpdateFinalPosition(const updatePositionStruct &update)
	{
		base->updateContentPosition(frame);
		updatePositionStruct u(update);
		u.position = base->contentPosition;
		u.size = base->contentSize;
		base->layout->updateFinalPosition(u);
	}

	void scrollableBaseStruct::scrollableEmit() const
	{
		if (elementBase != elementTypeEnum::InvalidElement)
		{
			vec2 p = base->position;
			vec2 s = base->size;
			offset(p, s, -defaults->baseMargin);
			emitElement(elementBase, 0, p, s);
		}
		if (elementCaption != elementTypeEnum::InvalidElement)
		{
			vec2 p = base->position;
			vec2 s = vec2(base->size[0], defaults->captionHeight);
			offset(p, s, -defaults->baseMargin * vec4(1,1,1,0));
			emitElement(elementCaption, 0, p, s);
			if (base->text)
			{
				offset(p, s, -defaults->captionPadding - skin().layouts[(uint32)elementCaption].border);
				base->text->emit(p, s);
			}
		}
		base->childrenEmit();
	}
}
