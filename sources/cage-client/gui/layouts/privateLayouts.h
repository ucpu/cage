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
	struct manualLayoutCacheStruct : public controlCacheStruct
	{
		manualLayoutCacheStruct(guiImpl *context, uint32 entity);
		virtual void updatePositionSize();
		virtual void updateRequestedSize();
	};

	struct rowLayoutCacheStruct : public controlCacheStruct
	{
		rowLayoutCacheStruct(guiImpl *context, uint32 entity);
		virtual void updatePositionSize();
		virtual void updateRequestedSize();
	};

	struct columnLayoutCacheStruct : public controlCacheStruct
	{
		columnLayoutCacheStruct(guiImpl *context, uint32 entity);
		virtual void updatePositionSize();
		virtual void updateRequestedSize();
	};
}
