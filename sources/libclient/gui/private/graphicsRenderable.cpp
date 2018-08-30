#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	renderableBaseStruct::renderableBaseStruct() : next(nullptr)
	{}

	void renderableBaseStruct::render(guiImpl *context)
	{}

	renderableElementStruct::elementStruct::elementStruct() : element(-1), mode(-1)
	{}

	renderableElementStruct::renderableElementStruct() : skinBuffer(nullptr), skinTexture(nullptr)
	{}

	renderableTextStruct::textStruct::textStruct() : glyphs(nullptr), font(nullptr), color(vec3::Nan), cursor(-1), count(0)
	{}

	void renderableTextStruct::textStruct::apply(const textFormatComponent &f, guiImpl *impl)
	{
		if (f.font)
			font = impl->assetManager->tryGet<assetSchemeIndexFont, fontClass>(f.font);
		if (f.size.valid())
			format.size = f.size;
		if (f.color.valid())
			color = f.color;
		if (f.align != (textAlignEnum)-1)
			format.align = f.align;
		if (f.lineSpacing.valid())
			format.lineSpacing = f.lineSpacing;
	}

	renderableImageStruct::imageStruct::imageStruct() : texture(nullptr)
	{}
}
