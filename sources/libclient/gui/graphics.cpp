#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "private.h"

namespace cage
{
	void guiManager::graphicsInitialize()
	{
		guiImpl *impl = (guiImpl*)this;

		// preallocate skins element buffers
		for (auto &s : impl->skins)
		{
			s.elementsGpuBuffer = newUniformBuffer();
			s.elementsGpuBuffer->bind();
			s.elementsGpuBuffer->writeWhole(nullptr, sizeof(guiSkinElementLayout::textureUvStruct) * (uint32)elementTypeEnum::TotalElements);
		}
	}

	void guiManager::graphicsFinalize()
	{
		guiImpl *impl = (guiImpl*)this;
		for (auto &it : impl->skins)
			it.elementsGpuBuffer.clear();
	}

	void guiManager::graphicsRender()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->graphicsDispatch();
	}
}
