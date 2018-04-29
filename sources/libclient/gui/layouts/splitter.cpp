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
				base->requestedSize = vec2();
				while (c)
				{
					c->updateRequestedSize();
					c->explicitPosition(c->requestedSize);
					{ // primary axis
						base->requestedSize[data.vertical] = base->requestedSize[data.vertical] + c->requestedSize[data.vertical];
					}
					{ // secondary axis
						if (data.allowSlaveResize)
							base->requestedSize[!data.vertical] = max(base->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
					}
					c = c->nextSibling;
				}
				if (!data.allowSlaveResize)
					base->requestedSize[!data.vertical] = master->requestedSize[!data.vertical];
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			void updateSecondary(updatePositionStruct &u, guiItemStruct *item)
			{
				uint32 axis = data.vertical ? 0 : 1;
				real r = item->requestedSize[axis]; // requested
				real g = u.size[axis]; // given
				if (!(item == master ? data.allowMasterResize : data.allowSlaveResize))
					return;
				u.position[axis] += (g - r) * data.anchor;
				u.size[axis] = r;
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
						updateSecondary(u, slave);
						slave->updateFinalPosition(u);
					}
					{ // master
						updatePositionStruct u(update);
						u.position[axis] += u.size[axis] - m;
						u.size[axis] = m;
						updateSecondary(u, master);
						master->updateFinalPosition(u);
					}
				}
				else
				{ // top-down
					{ // master
						updatePositionStruct u(update);
						u.size[axis] = m;
						updateSecondary(u, master);
						master->updateFinalPosition(u);
					}
					{ // slave
						updatePositionStruct u(update);
						u.position[axis] += m;
						u.size[axis] -= m;
						updateSecondary(u, slave);
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
