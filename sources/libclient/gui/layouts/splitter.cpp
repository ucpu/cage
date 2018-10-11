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
		struct layoutSplitterImpl : public layoutBaseStruct
		{
			layoutSplitterComponent &data;

			layoutSplitterImpl(guiItemStruct *base) : layoutBaseStruct(base), data(GUI_REF_COMPONENT(layoutSplitter))
			{}

			virtual void initialize() override
			{
				uint32 chc = 0;
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					chc++;
					c = c->nextSibling;
				}
				CAGE_ASSERT_RUNTIME(chc == 2, chc, "splitter layout must have exactly two children");
			}

			virtual void findRequestedSize() override
			{
				guiItemStruct *c = base->firstChild;
				base->requestedSize = vec2();
				while (c)
				{
					c->findRequestedSize();
					c->checkExplicitPosition(c->requestedSize);
					base->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					base->requestedSize[!data.vertical] = max(base->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
					c = c->nextSibling;
				}
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				guiItemStruct *a = base->firstChild, *b = base->firstChild->nextSibling;
				uint32 axis = data.vertical ? 1 : 0;
				finalPositionStruct u(update);
				real split = data.inverse ? update.renderSize[axis] - b->requestedSize[axis] : a->requestedSize[axis];
				u.renderPos[axis] += 0;
				u.renderSize[axis] = split;
				a->findFinalPosition(u);
				u.renderPos[axis] += split;
				u.renderSize[axis] = max(update.renderSize[axis] - split, 0);
				b->findFinalPosition(u);
			}
		};
	}

	void layoutSplitterCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutSplitterImpl>(item);
	}
}
