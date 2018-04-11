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
		struct layoutTableImpl : public layoutBaseStruct
		{
			layoutTableComponent data; // may not be reference
			real *widths;
			real *heights;
			uint32 mws, mhs;
			uint32 childs;
			bool justLine;

			layoutTableImpl(guiItemStruct *base, bool justLine) : layoutBaseStruct(base), widths(nullptr), heights(nullptr), mws(0), mhs(0), childs(0), justLine(justLine)
			{
				auto impl = base->impl;
				if (justLine)
				{
					GUI_GET_COMPONENT(layoutLine, l, base->entity);
					(layoutLineComponent&)data = l;
				}
				else
				{
					GUI_GET_COMPONENT(layoutTable, t, base->entity);
					data = t;
				}
			}

			virtual void initialize() override
			{}

			virtual void updateRequestedSize() override
			{
				auto impl = base->impl;
				{ // count childs
					childs = 0;
					guiItemStruct *c = base->firstChild;
					while (c)
					{
						childs++;
						c = c->nextSibling;
					}
				}
				// update sections
				if (justLine)
					data.sections = 1;
				else if (data.sections == 0)
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
				// allocate widths & heights
				widths = (real*)impl->itemsMemory.allocate(mws * sizeof(real));
				heights = (real*)impl->itemsMemory.allocate(mhs * sizeof(real));
				detail::memset(widths, 0, mws * sizeof(real));
				detail::memset(heights, 0, mhs * sizeof(real));
				// populate widths & heights
				guiItemStruct *c = base->firstChild;
				uint32 idx = 0;
				vec2 m;
				while (c)
				{
					c->updateRequestedSize();
					c->explicitPosition(c->requestedSize);
					m = max(m, c->requestedSize);
					uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
					uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
					CAGE_ASSERT_RUNTIME(wi < mws && hi < mhs, wi, hi, mws, mhs, data.sections, data.vertical, idx);
					real &w = widths[wi];
					real &h = heights[hi];
					w = max(w, c->requestedSize[0]);
					h = max(h, c->requestedSize[1]);
					c = c->nextSibling;
					idx++;
				}
				// grid fitting
				if (data.grid)
				{
					for (uint32 x = 0; x < mws; x++)
						widths[x] = m[0];
					for (uint32 y = 0; y < mhs; y++)
						heights[y] = m[1];
					base->requestedSize = m * vec2(mws, mhs);
				}
				else
				{
					base->requestedSize = vec2();
					for (uint32 x = 0; x < mws; x++)
						base->requestedSize[0] += widths[x];
					for (uint32 y = 0; y < mhs; y++)
						base->requestedSize[1] += heights[y];
				}
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				guiItemStruct *c = base->firstChild;
				uint32 idx = 0;
				vec2 pos = update.position;
				vec2 spacing;
				if (data.addSpacingToFillArea)
					spacing = max(update.size - base->requestedSize, vec2()) / vec2(mws + 1, mhs + 1);
				while (c)
				{
					uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
					uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
					CAGE_ASSERT_RUNTIME(wi < mws && hi < mhs, wi, hi, mws, mhs, data.sections, data.vertical, idx);
					real w = widths[wi];
					real h = heights[hi];

					{
						updatePositionStruct u(update);
						u.position = pos;
						u.size = c->requestedSize;
						if (data.expandToSameWidth)
							u.size[0] = w;
						if (data.expandToSameHeight)
							u.size[1] = h;
						u.position += spacing * vec2(wi, hi);
						u.position += max(vec2(w, h) - u.size, vec2()) * data.cellsAnchor;
						c->updateFinalPosition(u);
					}

					// advance position
					if (data.vertical)
					{ // vertical
						if (wi == data.sections - 1)
						{
							pos[0] = update.position[0];
							pos[1] += heights[hi];
						}
						else
							pos[0] += w;
					}
					else
					{ // horizontal
						if (hi == data.sections - 1)
						{
							pos[0] += widths[wi];
							pos[1] = update.position[1];
						}
						else
							pos[1] += h;
					}
					c = c->nextSibling;
					idx++;
				}
			}
		};
	}

	void layoutLineCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutTableImpl>(item, true);
	}

	void layoutTableCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutTableImpl>(item, false);
	}
}
