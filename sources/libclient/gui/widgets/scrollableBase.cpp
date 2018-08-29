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
		base->layout->findRequestedSize();
		offsetSize(base->requestedSize, defaults->contentPadding);
		if (elementCaption != elementTypeEnum::InvalidElement)
		{
			base->requestedSize[1] += defaults->captionHeight;
			if (base->text)
			{
				vec2 cs = base->text->findRequestedSize();
				offsetSize(cs, defaults->captionPadding);
				base->requestedSize[0] = max(base->requestedSize[0], cs[0]);
				// it is important to compare (text size + text padding) with (children size + children padding)
				// and only after that to add border and base margin
			}
		}
		if (elementBase != elementTypeEnum::InvalidElement)
			offsetSize(base->requestedSize, skin().layouts[(uint32)elementBase].border);
		offsetSize(base->requestedSize, defaults->baseMargin);
		// todo scrollbars
	}

	void scrollableBaseStruct::scrollableUpdateFinalPosition(const finalPositionStruct &update)
	{
		finalPositionStruct u(update);
		if (elementCaption != elementTypeEnum::InvalidElement)
			u.position[1] += defaults->captionHeight;
		if (elementBase != elementTypeEnum::InvalidElement)
			offset(u.position, u.size, -skin().layouts[(uint32)elementBase].border);
		offset(u.position, u.size, -defaults->baseMargin - defaults->contentPadding);
		// todo scrollbars
		base->layout->findFinalPosition(u);
	}

	void scrollableBaseStruct::scrollableEmit() const
	{
		vec2 p = base->position;
		vec2 s = base->size;
		offset(p, s, -defaults->baseMargin);
		if (elementBase != elementTypeEnum::InvalidElement)
			emitElement(elementBase, mode(false), p, s);
		if (elementCaption != elementTypeEnum::InvalidElement)
		{
			s = vec2(s[0], defaults->captionHeight);
			emitElement(elementCaption, mode(p, s), p, s);
			if (base->text)
			{
				offset(p, s, -skin().layouts[(uint32)elementCaption].border - defaults->captionPadding);
				base->text->emit(p, s);
			}
		}
		base->childrenEmit();
	}
}
