#include <set>
#include <map>
#include <vector>
#include <algorithm>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include "binPacking.h"

namespace cage
{
	namespace
	{
		struct rectStruct
		{
			uint32 id;
			uint32 w, h;
			uint32 x, y;
		};

		struct comparatorStruct
		{
			const bool operator() (const rectStruct &a, const rectStruct &b) const
			{
				if (a.h == b.h)
					return a.w > b.w;
				return a.h > b.h;
			}
		};

		class binPackingImpl : public binPackingClass
		{
		public:
			std::vector<rectStruct> rects;

			binPackingImpl()
			{
				rects.reserve(1000);
			}

			const bool solve(uint32 width, uint32 height)
			{
				std::sort(rects.begin(), rects.end(), comparatorStruct());
				uint32 y = 0;
				uint32 x = width + 1;
				uint32 ny = 0;
				for (std::vector<rectStruct>::iterator it = rects.begin(), et = rects.end(); it != et; it++)
				{
					rectStruct &r = *it;
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

	void binPackingClass::add(uint32 id, uint32 width, uint32 height)
	{
		binPackingImpl *impl = (binPackingImpl*)this;
		rectStruct r;
		r.id = id;
		r.w = width;
		r.h = height;
		r.x = r.y = 0;
		impl->rects.push_back(r);
	}

	bool binPackingClass::solve(uint32 width, uint32 height)
	{
		return ((binPackingImpl*)this)->solve(width, height);
	}

	void binPackingClass::get(uint32 index, uint32 &id, uint32 & x, uint32 & y) const
	{
		binPackingImpl *impl = (binPackingImpl*)this;
		CAGE_ASSERT(index < impl->rects.size(), index, impl->rects.size());
		rectStruct &r = impl->rects[index];
		id = r.id;
		x = r.x;
		y = r.y;
	}

	uint32 binPackingClass::count() const
	{
		binPackingImpl *impl = (binPackingImpl*)this;
		return numeric_cast<uint32>(impl->rects.size());
	}

	holder<binPackingClass> newBinPacking()
	{
		return detail::systemArena().createImpl<binPackingClass, binPackingImpl>();
	}
}