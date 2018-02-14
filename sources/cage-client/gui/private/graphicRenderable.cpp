#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

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

	renderableImageStruct::imageStruct::imageStruct() : texture(nullptr)
	{}
}
