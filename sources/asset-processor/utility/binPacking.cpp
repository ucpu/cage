#include <cage-core/core.h>
#include <cage-core/math.h>
#include "binPacking.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>

namespace cage
{
	namespace
	{
		struct Rect
		{
			uint32 id;
			uint32 w, h;
			uint32 x, y;
		};

		struct Comparator
		{
			const bool operator() (const Rect &a, const Rect &b) const
			{
				if (a.h == b.h)
					return a.w > b.w;
				return a.h > b.h;
			}
		};

		class BinPackingImpl : public BinPacking
		{
		public:
			std::vector<Rect> rects;

			BinPackingImpl()
			{
				rects.reserve(1000);
			}

			const bool solve(uint32 width, uint32 height)
			{
				std::sort(rects.begin(), rects.end(), Comparator());
				uint32 y = 0;
				uint32 x = width + 1;
				uint32 ny = 0;
				for (std::vector<Rect>::iterator it = rects.begin(), et = rects.end(); it != et; it++)
				{
					Rect &r = *it;
					CAGE_ASSERT(r.w < width && r.h < height, r.w, r.h, width, height, "impossibly large rectangle");
					if (x + r.w < width)
					{
						r.x = x;
						r.y = y;
						x += r.w + 1;
					}
					else if (ny + r.h < height)
					{
						r.x = 0;
						r.y = y = ny;
						ny = y + r.h + 1;
						x = r.w;
					}
					else
						return false;
				}
				return true;
			}
		};
	}

	void BinPacking::add(uint32 id, uint32 width, uint32 height)
	{
		BinPackingImpl *impl = (BinPackingImpl*)this;
		Rect r;
		r.id = id;
		r.w = width;
		r.h = height;
		r.x = r.y = 0;
		impl->rects.push_back(r);
	}

	bool BinPacking::solve(uint32 width, uint32 height)
	{
		return ((BinPackingImpl*)this)->solve(width, height);
	}

	void BinPacking::get(uint32 index, uint32 &id, uint32 & x, uint32 & y) const
	{
		BinPackingImpl *impl = (BinPackingImpl*)this;
		CAGE_ASSERT(index < impl->rects.size(), index, impl->rects.size());
		Rect &r = impl->rects[index];
		id = r.id;
		x = r.x;
		y = r.y;
	}

	uint32 BinPacking::count() const
	{
		BinPackingImpl *impl = (BinPackingImpl*)this;
		return numeric_cast<uint32>(impl->rects.size());
	}

	Holder<BinPacking> newBinPacking()
	{
		return detail::systemArena().createImpl<BinPacking, BinPackingImpl>();
	}
}
