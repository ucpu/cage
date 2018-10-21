#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/color.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	finalPositionStruct::finalPositionStruct() : renderPos(vec2::Nan), renderSize(vec2::Nan), clipPos(vec2::Nan), clipSize(vec2::Nan)
	{}

	void offsetPosition(vec2 &position, const vec4 &offset)
	{
		CAGE_ASSERT_RUNTIME(position.valid() && offset.valid(), position, offset);
		position -= vec2(offset);
	}

	void offsetSize(vec2 &size, const vec4 &offset)
	{
		CAGE_ASSERT_RUNTIME(size.valid() && offset.valid(), size, offset);
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
		CAGE_ASSERT_RUNTIME(pos.valid() && size.valid() && point.valid(), pos, size, point);
		if (point[0] < pos[0] || point[1] < pos[1])
			return false;
		pos += size;
		if (point[0] > pos[0] || point[1] > pos[1])
			return false;
		return true;
	}

	bool clip(vec2 &pos, vec2 &size, vec2 clipPos, vec2 clipSize)
	{
		CAGE_ASSERT_RUNTIME(clipPos.valid());
		CAGE_ASSERT_RUNTIME(clipSize.valid() && clipSize[0] >= 0 && clipSize[1] >= 0, clipPos, clipSize);
		CAGE_ASSERT_RUNTIME(pos.valid());
		CAGE_ASSERT_RUNTIME(size.valid() && size[0] >= 0 && size[1] >= 0, pos, size);
		vec2 e = min(pos + size, clipPos + clipSize);
		pos = max(pos, clipPos);
		size = max(e - pos, vec2());
		return size[0] > 0 && size[1] > 0;
	}

	hierarchyItemStruct *subsideItem(hierarchyItemStruct *item)
	{
		hierarchyItemStruct *n = item->impl->itemsMemory.createObject<hierarchyItemStruct>(item->impl, item->entity);
		{
			hierarchyItemStruct *i = item->firstChild;
			while (i)
			{
				CAGE_ASSERT_RUNTIME(i->parent == item);
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

	void ensureItemHasLayout(hierarchyItemStruct *hierarchy)
	{
		if (hierarchy->impl->entityLayoutsCount(hierarchy->entity) == 0)
		{
			subsideItem(hierarchy); // fall back to default layouting
		}
	}
}
