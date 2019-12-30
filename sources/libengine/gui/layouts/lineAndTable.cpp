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
		struct layoutTableImpl : public layoutItemStruct
		{
			GuiLayoutTableComponent data; // may not be reference
			real *widths;
			real *heights;
			uint32 mws, mhs;
			uint32 childs;

			layoutTableImpl(hierarchyItemStruct *hierarchy, bool justLine) : layoutItemStruct(hierarchy), widths(nullptr), heights(nullptr), mws(0), mhs(0), childs(0)
			{
				auto impl = hierarchy->impl;
				if (justLine)
				{
					CAGE_COMPONENT_GUI(LayoutLine, l, hierarchy->ent);
					data.vertical = l.vertical;
					data.sections = 1;
				}
				else
				{
					CAGE_COMPONENT_GUI(LayoutTable, t, hierarchy->ent);
					data = t;
				}
			}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				auto impl = hierarchy->impl;
				{ // count childs
					childs = 0;
					hierarchyItemStruct *c = hierarchy->firstChild;
					while (c)
					{
						childs++;
						c = c->nextSibling;
					}
				}
				// update sections
				if (data.sections == 0)
				{
					uint32 cnt = numeric_cast<uint32>(round(sqrt(childs)));
					data.sections = max(cnt, 1u);
				}
				// mws & mhs
				if (data.vertical)
				{
					mws = data.sections;
					mhs = childs / data.sections;
					if (childs > mws * mhs)
						mhs++;
				}
				else
				{
					mws = childs / data.sections;
					mhs = data.sections;
					if (childs > mws * mhs)
						mws++;
				}
				CAGE_ASSERT(mws * mhs >= childs, mws, mhs, childs);
				// allocate widths & heights
				widths = (real*)impl->itemsMemory.allocate(mws * sizeof(real), sizeof(uintPtr));
				heights = (real*)impl->itemsMemory.allocate(mhs * sizeof(real), sizeof(uintPtr));
				detail::memset(widths, 0, mws * sizeof(real));
				detail::memset(heights, 0, mhs * sizeof(real));
				// populate widths & heights
				vec2 m;
				{
					hierarchyItemStruct *c = hierarchy->firstChild;
					uint32 idx = 0;
					while (c)
					{
						c->findRequestedSize();
						m = max(m, c->requestedSize);
						uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
						uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
						CAGE_ASSERT(wi < mws && hi < mhs, wi, hi, mws, mhs, data.sections, data.vertical, idx);
						real &w = widths[wi];
						real &h = heights[hi];
						w = max(w, c->requestedSize[0]);
						h = max(h, c->requestedSize[1]);
						c = c->nextSibling;
						idx++;
					}
				}
				// grid fitting
				if (data.grid)
				{
					for (uint32 x = 0; x < mws; x++)
						widths[x] = m[0];
					for (uint32 y = 0; y < mhs; y++)
						heights[y] = m[1];
					hierarchy->requestedSize = m * vec2(mws, mhs);
				}
				else
				{
					hierarchy->requestedSize = vec2();
					for (uint32 x = 0; x < mws; x++)
						hierarchy->requestedSize[0] += widths[x];
					for (uint32 y = 0; y < mhs; y++)
						hierarchy->requestedSize[1] += heights[y];
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				hierarchyItemStruct *c = hierarchy->firstChild;
				uint32 idx = 0;
				vec2 spacing = (update.renderSize - hierarchy->requestedSize) / vec2(mws, mhs);
				//spacing = max(spacing, vec2());
				vec2 pos = update.renderPos;
				while (c)
				{
					uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
					uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
					CAGE_ASSERT(wi < mws && hi < mhs, wi, hi, mws, mhs, data.sections, data.vertical, idx);
					vec2 s = vec2(widths[wi], heights[hi]);

					{
						finalPositionStruct u(update);
						u.renderPos = pos + vec2(wi, hi) * spacing;
						u.renderSize = max(s + spacing, 0);
						c->findFinalPosition(u);
					}

					// advance position
					if (data.vertical)
					{ // vertical
						if (wi == data.sections - 1)
						{
							pos[0] = update.renderPos[0];
							pos[1] += heights[hi];
						}
						else
							pos[0] += s[0];
					}
					else
					{ // horizontal
						if (hi == data.sections - 1)
						{
							pos[0] += widths[wi];
							pos[1] = update.renderPos[1];
						}
						else
							pos[1] += s[1];
					}
					c = c->nextSibling;
					idx++;
				}
			}
		};
	}

	void LayoutLineCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<layoutTableImpl>(item, true);
	}

	void LayoutTableCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<layoutTableImpl>(item, false);
	}
}
