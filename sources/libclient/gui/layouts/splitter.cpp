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
		struct layoutSplitterImpl : public layoutBaseStruct
		{
			layoutSplitterComponent &data;
			guiItemStruct *master, *slave;

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
				master = base->firstChild;
				slave = master->nextSibling;
				if (data.inverse)
					std::swap(master, slave);
			}

			virtual void updateRequestedSize() override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					c->updateRequestedSize();
					c->explicitPosition(c->requestedSize);
					c = c->nextSibling;
				}
				base->requestedSize = master->requestedSize;
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				uint32 axis = data.vertical ? 1 : 0;
				real m = master->requestedSize[axis];
				if (data.inverse)
				{ // bottom-up
					{ // slave
						updatePositionStruct u(update);
						u.size[axis] -= m;
						slave->updateFinalPosition(u);
					}
					{ // master
						updatePositionStruct u(update);
						u.position[axis] += base->size[axis] - m;
						u.size[axis] = m;
						master->updateFinalPosition(u);
					}
				}
				else
				{ // top-down
					{ // master
						updatePositionStruct u(update);
						u.size[axis] = m;
						master->updateFinalPosition(u);
					}
					{ // slave
						updatePositionStruct u(update);
						u.position[axis] += m;
						u.size[axis] -= m;
						slave->updateFinalPosition(u);
					}
				}
			}
		};
	}

	void layoutSplitterCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutSplitterImpl>(item);
	}
}
