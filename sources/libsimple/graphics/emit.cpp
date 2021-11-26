#include <cage-core/swapBufferGuard.h>
#include <cage-core/entitiesCopy.h>

#include "graphics.h"

namespace cage
{
	void Graphics::emit(uint64 time)
	{
		if (auto lock = emitBuffersGuard->write())
		{
			EntitiesCopyConfig cfg;
			cfg.source = engineEntities();
			cfg.destination = +emitBuffers[lock.index()].scene;
			entitiesCopy(cfg);
			emitBuffers[lock.index()].time = time;
		}
	}
}
