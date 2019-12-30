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
		struct layoutSplitterImpl : public layoutItemStruct
		{
			GuiLayoutSplitterComponent &data;

			layoutSplitterImpl(hierarchyItemStruct *hierarchy) : layoutItemStruct(hierarchy), data(GUI_REF_COMPONENT(LayoutSplitter))
			{}

			virtual void initialize() override
			{
				uint32 chc = 0;
				hierarchyItemStruct *c = hierarchy->firstChild;
				while (c)
				{
					chc++;
					c = c->nextSibling;
				}
				CAGE_ASSERT(chc == 2, chc, "splitter layout must have exactly two children");
			}

			virtual void findRequestedSize() override
			{
				hierarchyItemStruct *c = hierarchy->firstChild;
				hierarchy->requestedSize = vec2();
				while (c)
				{
					c->findRequestedSize();
					hierarchy->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					hierarchy->requestedSize[!data.vertical] = max(hierarchy->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
					c = c->nextSibling;
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				hierarchyItemStruct *a = hierarchy->firstChild, *b = hierarchy->firstChild->nextSibling;
				uint32 axis = data.vertical ? 1 : 0;
				finalPositionStruct u(update);
				real split = data.inverse ? max(update.renderSize[axis] - b->requestedSize[axis], 0) : a->requestedSize[axis];
				u.renderPos[axis] += 0;
				u.renderSize[axis] = split;
				a->findFinalPosition(u);
				u.renderPos[axis] += split;
				u.renderSize[axis] = max(update.renderSize[axis] - split, 0);
				b->findFinalPosition(u);
			}
		};
	}

	void LayoutSplitterCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<layoutSplitterImpl>(item);
	}
}
