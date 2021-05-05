#include <cage-core/assetManager.h>

#include "../private.h"

namespace cage
{
	RenderableBase::RenderableBase() : next(nullptr)
	{}

	RenderableBase::~RenderableBase()
	{}

	void RenderableBase::setClip(const HierarchyItem *item)
	{
		clipPos = item->clipPos * item->impl->pointsScale;
		clipSize = item->clipSize * item->impl->pointsScale;
	}

	void RenderableBase::render(GuiImpl *context)
	{}

	RenderableElement::Element::Element() : element(m), mode(m)
	{}

	RenderableElement::RenderableElement()
	{}

	RenderableText::Text::Text() : glyphs(nullptr), font(nullptr), color(vec3::Nan()), cursor(m), count(0)
	{}

	void RenderableText::Text::apply(const GuiTextFormatComponent &f, GuiImpl *impl)
	{
		if (f.font)
			font = impl->assetMgr->tryGetRaw<AssetSchemeIndexFont, Font>(f.font);
		if (f.size.valid())
			format.size = f.size;
		if (f.color.valid())
			color = f.color;
		if (f.align != (TextAlignEnum)-1)
			format.align = f.align;
		if (f.lineSpacing.valid())
			format.lineSpacing = f.lineSpacing;
	}

	RenderableImage::Image::Image() : texture(nullptr)
	{}
}
