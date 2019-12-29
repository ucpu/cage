#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/window.h>
#include "private.h"

namespace cage
{
	void Gui::graphicsInitialize()
	{
		guiImpl *impl = (guiImpl*)this;

		// preallocate skins element buffers
		for (auto &s : impl->skins)
		{
			s.elementsGpuBuffer = newUniformBuffer();
			s.elementsGpuBuffer->bind();
			s.elementsGpuBuffer->writeWhole(nullptr, sizeof(GuiSkinElementLayout::TextureUv) * (uint32)ElementTypeEnum::TotalElements, GL_DYNAMIC_DRAW);
		}
	}

	void Gui::graphicsFinalize()
	{
		guiImpl *impl = (guiImpl*)this;
		for (auto &it : impl->skins)
			it.elementsGpuBuffer.clear();
	}

	void Gui::graphicsRender()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->graphicsDispatch();
	}
}
