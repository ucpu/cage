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

#include <unordered_map>

namespace cage
{
	void imageCreate(guiItemStruct *item)
	{
		item->image = item->impl->itemsMemory.createObject<imageItemStruct>(item);
	}

	imageItemStruct::imageItemStruct(guiItemStruct *base) : base(base)
	{}

	void imageItemStruct::initialize()
	{

	}

	void imageItemStruct::updateRequestedSize(vec2 &size)
	{

	}

	renderableImageStruct *imageItemStruct::emit() const
	{
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableImageStruct>();
		// todo
		e->last->next = t;
		e->last = t;
		return t;
	}
}
