#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/assetManager.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	renderableBaseStruct::renderableBaseStruct() : next(nullptr)
	{}

	void renderableBaseStruct::setClip(const hierarchyItemStruct *item)
	{
		clipPos = item->clipPos * item->impl->pointsScale;
		clipSize = item->clipSize * item->impl->pointsScale;
	}

	void renderableBaseStruct::render(guiImpl *context)
	{}

	renderableElementStruct::elementStruct::elementStruct() : element(m), mode(m)
	{}

	renderableElementStruct::renderableElementStruct() : skinBuffer(nullptr), skinTexture(nullptr)
	{}

	renderableTextStruct::textStruct::textStruct() : glyphs(nullptr), font(nullptr), color(vec3::Nan()), cursor(m), count(0)
	{}

	void renderableTextStruct::textStruct::apply(const textFormatComponent &f, guiImpl *impl)
	{
		if (f.font)
			font = impl->assetMgr->tryGet<assetSchemeIndexFontFace, fontFace>(f.font);
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