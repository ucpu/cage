#include <cage-engine/opengl.h>
#include "private.h"

namespace cage
{
	void Gui::graphicsInitialize()
	{
		GuiImpl *impl = (GuiImpl*)this;

		// preallocate skins element buffers
		for (auto &s : impl->skins)
		{
			s.elementsGpuBuffer = newUniformBuffer();
			s.elementsGpuBuffer->bind();
			PointerRange<const char> pr = { (char*)0, (char*)0 + sizeof(GuiSkinElementLayout::TextureUv) * (uint32)GuiElementTypeEnum::TotalElements };
			s.elementsGpuBuffer->writeWhole(pr, GL_DYNAMIC_DRAW);
		}
	}

	void Gui::graphicsFinalize()
	{
		GuiImpl *impl = (GuiImpl*)this;
		for (auto &it : impl->skins)
			it.elementsGpuBuffer.clear();
	}

	void Gui::graphicsRender()
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->graphicsDispatch();
	}
}
