#include <cage-core/enumerate.h>

#include "../private.h"

namespace cage
{
	namespace
	{
		struct LayoutTableImpl : public LayoutItem
		{
			GuiLayoutTableComponent data; // may not be reference
			std::vector<real> widths;
			std::vector<real> heights;
			uint32 mws = 0, mhs = 0;

			LayoutTableImpl(HierarchyItem *hierarchy, bool justLine) : LayoutItem(hierarchy)
			{
				auto impl = hierarchy->impl;
				if (justLine)
				{
					GUI_COMPONENT(LayoutLine, l, hierarchy->ent);
					data.vertical = l.vertical;
					data.sections = 1;
				}
				else
				{
					GUI_COMPONENT(LayoutTable, t, hierarchy->ent);
					data = t;
				}
			}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				const uint32 childs = numeric_cast<uint32>(hierarchy->children.size());

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
				CAGE_ASSERT(mws * mhs >= childs);

				// allocate widths & heights
				widths.resize(mws);
				heights.resize(mhs);

				// populate widths & heights
				vec2 m;
				for (const auto &it : enumerate(hierarchy->children))
				{
					HierarchyItem *c = +*it;
					const uint32 idx = numeric_cast<uint32>(it.index);
					c->findRequestedSize();
					m = max(m, c->requestedSize);
					const uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
					const uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
					CAGE_ASSERT(wi < mws && hi < mhs);
					real &w = widths[wi];
					real &h = heights[hi];
					w = max(w, c->requestedSize[0]);
					h = max(h, c->requestedSize[1]);
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

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				const vec2 spacing = (update.renderSize - hierarchy->requestedSize) / vec2(mws, mhs);
				vec2 pos = update.renderPos;
				for (const auto &it : enumerate(hierarchy->children))
				{
					HierarchyItem *c = +*it;
					const uint32 idx = numeric_cast<uint32>(it.index);
					const uint32 wi = data.vertical ? (idx % data.sections) : (idx / data.sections);
					const uint32 hi = data.vertical ? (idx / data.sections) : (idx % data.sections);
					CAGE_ASSERT(wi < mws && hi < mhs);
					const vec2 s = vec2(widths[wi], heights[hi]);

					{
						FinalPosition u(update);
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
				}
			}
		};
	}

	void LayoutLineCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<LayoutTableImpl>(item, true).cast<BaseItem>();
	}

	void LayoutTableCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<LayoutTableImpl>(item, false).cast<BaseItem>();
	}
}
