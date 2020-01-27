#include <cage-core/config.h>
#include <cage-core/color.h>

#include "../private.h"

namespace cage
{
	FinalPosition::FinalPosition() : renderPos(vec2::Nan()), renderSize(vec2::Nan()), clipPos(vec2::Nan()), clipSize(vec2::Nan())
	{}

	void offsetPosition(vec2 &position, const vec4 &offset)
	{
		CAGE_ASSERT(position.valid() && offset.valid(), position, offset);
		position -= vec2(offset);
	}

	void offsetSize(vec2 &size, const vec4 &offset)
	{
		CAGE_ASSERT(size.valid() && offset.valid(), size, offset);
		size += vec2(offset) + vec2(offset[2], offset[3]);
		size = max(size, vec2());
	}

	void offset(vec2 &position, vec2 &size, const vec4 &offset)
	{
		offsetPosition(position, offset);
		offsetSize(size, offset);
	}

	bool pointInside(vec2 pos, vec2 size, vec2 point)
	{
		CAGE_ASSERT(pos.valid() && size.valid() && point.valid(), pos, size, point);
		if (point[0] < pos[0] || point[1] < pos[1])
			return false;
		pos += size;
		if (point[0] > pos[0] || point[1] > pos[1])
			return false;
		return true;
	}

	bool clip(vec2 &pos, vec2 &size, vec2 clipPos, vec2 clipSize)
	{
		CAGE_ASSERT(clipPos.valid());
		CAGE_ASSERT(clipSize.valid() && clipSize[0] >= 0 && clipSize[1] >= 0, clipPos, clipSize);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid() && size[0] >= 0 && size[1] >= 0, pos, size);
		vec2 e = min(pos + size, clipPos + clipSize);
		pos = max(pos, clipPos);
		size = max(e - pos, vec2());
		return size[0] > 0 && size[1] > 0;
	}

	HierarchyItem *subsideItem(HierarchyItem *item)
	{
		HierarchyItem *n = item->impl->itemsMemory.createObject<HierarchyItem>(item->impl, item->ent);
		{
			HierarchyItem *i = item->firstChild;
			while (i)
			{
				CAGE_ASSERT(i->parent == item);
				i->parent = n;
				i = i->nextSibling;
			}
		}
		n->parent = item;
		n->firstChild = item->firstChild;
		n->lastChild = item->lastChild;
		item->firstChild = item->lastChild = n;
		n->subsidedItem = true;
		return n;
	}

	void ensureItemHasLayout(HierarchyItem *hierarchy)
	{
		if (hierarchy->impl->entityLayoutsCount(hierarchy->ent) == 0)
		{
			subsideItem(hierarchy); // fall back to default layouting
		}
	}
}
