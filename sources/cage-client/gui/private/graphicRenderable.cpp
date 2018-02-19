#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
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

	renderableTextStruct::textStruct::textStruct() : glyphs(nullptr), font(nullptr), color(vec3::Nan), cursor(-1), count(0), posX(0), posY(0)
	{}

	void renderableTextStruct::textStruct::apply(const textFormatComponent &f, guiImpl *impl)
	{
		if (f.fontName)
			font = impl->assetManager->tryGet<assetSchemeIndexFont, fontClass>(f.fontName);
		if (f.color.valid())
			color = f.color;
		if (f.align != (textAlignEnum)-1)
			format.align = f.align;
		if (f.lineSpacing != detail::numeric_limits<sint16>::min())
			format.lineSpacing = f.lineSpacing;
	}

	renderableImageStruct::imageStruct::imageStruct() : texture(nullptr)
	{}
}
