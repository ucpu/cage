#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>
#include <cage-core/textPack.h>

#include <cage-engine/texture.h>

#include "private.h"

namespace cage
{
	HierarchyItem::HierarchyItem(GuiImpl *impl, Entity *ent) : impl(impl), ent(ent)
	{
		CAGE_ASSERT(impl);
	}

	void HierarchyItem::initialize()
	{
		if (item)
			item->initialize();
		if (text)
			text->initialize();
		if (image)
			image->initialize();
	}

	void HierarchyItem::findRequestedSize()
	{
		if (item)
			item->findRequestedSize();
		else if (text)
			requestedSize = text->findRequestedSize();
		else if (image)
			requestedSize = image->findRequestedSize();
		else
		{
			requestedSize = Vec2();
			for (const auto &c : children)
			{
				c->findRequestedSize();
				requestedSize = max(requestedSize, c->requestedSize);
			}
		}
		CAGE_ASSERT(requestedSize.valid());
	}

	void HierarchyItem::findFinalPosition(const FinalPosition &update)
	{
		CAGE_ASSERT(requestedSize.valid());
		CAGE_ASSERT(update.renderPos.valid());
		CAGE_ASSERT(update.renderSize.valid());
		CAGE_ASSERT(update.clipPos.valid());
		CAGE_ASSERT(update.clipSize.valid());

		renderPos = update.renderPos;
		renderSize = update.renderSize;

		FinalPosition u(update);
		clip(u.clipPos, u.clipSize, u.renderPos, u.renderSize); // update the clip rect to intersection with the render rect
		clipPos = u.clipPos;
		clipSize = u.clipSize;

		if (item)
			item->findFinalPosition(u);
		else for (const auto &c : children)
			c->findFinalPosition(u);

		CAGE_ASSERT(renderPos.valid());
		CAGE_ASSERT(renderSize.valid());
		CAGE_ASSERT(clipPos.valid());
		CAGE_ASSERT(clipSize.valid());
		for (uint32 a = 0; a < 2; a++)
		{
			CAGE_ASSERT(clipPos[a] >= u.clipPos[a]);
			CAGE_ASSERT(clipPos[a] + clipSize[a] <= u.clipPos[a] + u.clipSize[a]);
		}
	}

	void HierarchyItem::childrenEmit()
	{
		for (const auto &a : children)
		{
			if (a->item)
				a->item->emit();
			else
				a->childrenEmit();
		}
	}

	void HierarchyItem::generateEventReceivers() const
	{
		for (auto i = children.size(); i-- > 0;) // typographically pleasing iteration in reverse order ;)
			children[i]->generateEventReceivers();
		if (item)
			item->generateEventReceivers();
	}

	void HierarchyItem::moveToWindow(bool horizontal, bool vertical)
	{
		bool enabled[2] = { horizontal, vertical };
		for (uint32 i = 0; i < 2; i++)
		{
			if (!enabled[i])
				continue;
			Real offset = 0;
			if (renderPos[i] + renderSize[i] > impl->outputSize[i])
				offset = (impl->outputSize[i] - renderSize[i]) - renderPos[i];
			else if (renderPos[i] < 0)
				offset = -renderPos[i];
			renderPos[i] += offset;
		}
	}

	HierarchyItem *HierarchyItem::findParentOf(HierarchyItem *item)
	{
		for (const auto &it : children)
		{
			if (+it == item)
				return this;
			if (HierarchyItem *r = it->findParentOf(item))
				return r;
		}
		return nullptr;
	}

	BaseItem::BaseItem(HierarchyItem *hierarchy) : hierarchy(hierarchy)
	{}

	WidgetItem::WidgetItem(HierarchyItem *hierarchy) : BaseItem(hierarchy)
	{}

	ElementModeEnum WidgetItem::mode(bool hover, uint32 focusParts) const
	{
		if (widgetState.disabled)
			return ElementModeEnum::Disabled;
		if (hierarchy->impl->hover == this && hover)
			return ElementModeEnum::Hover;
		if (hasFocus(focusParts))
			return ElementModeEnum::Focus;
		return ElementModeEnum::Default;
	}

	ElementModeEnum WidgetItem::mode(const Vec2 &pos, const Vec2 &size, uint32 focusParts) const
	{
		return mode(pointInside(pos, size, hierarchy->impl->outputMouse), focusParts);
	}

	bool WidgetItem::hasFocus(uint32 parts) const
	{
		CAGE_ASSERT(hierarchy->ent);
		return hierarchy->impl->focusName && hierarchy->impl->focusName == hierarchy->ent->name() && (hierarchy->impl->focusParts & parts) > 0;
	}

	void WidgetItem::makeFocused(uint32 parts)
	{
		CAGE_ASSERT(parts != 0);
		CAGE_ASSERT(hierarchy->ent);
		CAGE_ASSERT(!widgetState.disabled);
		hierarchy->impl->focusName = hierarchy->ent->name();
		hierarchy->impl->focusParts = parts;
	}

	void WidgetItem::findFinalPosition(const FinalPosition &update)
	{
		// do nothing
	}

	void WidgetItem::generateEventReceivers()
	{
		EventReceiver e;
		e.widget = this;
		e.pos = hierarchy->renderPos;
		e.size = hierarchy->renderSize;
		if (clip(e.pos, e.size, hierarchy->clipPos, hierarchy->clipSize))
			hierarchy->impl->mouseEventReceivers.push_back(e);
	}

	RenderableElement WidgetItem::emitElement(GuiElementTypeEnum element, ElementModeEnum mode, Vec2 pos, Vec2 size)
	{
		return RenderableElement(this, element, mode, pos, size);
	}

	bool WidgetItem::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		makeFocused();
		return true;
	}

	bool WidgetItem::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point)
	{
		return true;
	}

	bool WidgetItem::keyPress(uint32 key, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyRepeat(uint32 key, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyRelease(uint32 key, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyChar(uint32 key)
	{
		return true;
	}

	LayoutItem::LayoutItem(HierarchyItem *hierarchy) : BaseItem(hierarchy)
	{}

	bool LayoutItem::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point)
	{
		return false;
	}

	bool LayoutItem::mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point)
	{
		return false;
	}

	bool LayoutItem::keyPress(uint32 key, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRepeat(uint32 key, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyRelease(uint32 key, ModifiersFlags modifiers)
	{
		return false;
	}

	bool LayoutItem::keyChar(uint32 key)
	{
		return false;
	}

	void LayoutItem::emit()
	{
		hierarchy->childrenEmit();
	}

	void LayoutItem::generateEventReceivers()
	{
		// do nothing
	}

	void textCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->text);
		item->text = item->impl->memory->createHolder<TextItem>(item);
	}

	TextItem::TextItem(HierarchyItem *hierarchy) : hierarchy(hierarchy)
	{}

	void TextItem::initialize()
	{
		if (skipInitialize)
			return;
		if (GUI_HAS_COMPONENT(TextFormat, hierarchy->ent))
		{
			GUI_COMPONENT(TextFormat, f, hierarchy->ent);
			apply(f);
		}
		transcript();
	}

	void TextItem::apply(const GuiTextFormatComponent &f)
	{
		if (f.font)
			font = hierarchy->impl->assetMgr->get<AssetSchemeIndexFont, Font>(f.font);
		if (f.size.valid())
			format.size = f.size;
		if (f.color.valid())
			color = f.color;
		if (f.align != (TextAlignEnum)m)
			format.align = f.align;
		if (f.lineSpacing.valid())
			format.lineSpacing = f.lineSpacing;
	}

	void TextItem::transcript()
	{
		GUI_COMPONENT(Text, t, hierarchy->ent);
		String value = loadFormattedString(hierarchy->impl->assetMgr, t.assetName, t.textName, t.value);
		transcript(value);
	}

	void TextItem::transcript(const String &value)
	{
		if (font)
			glyphs = font->transcript(value);
	}

	void TextItem::transcript(PointerRange<const char> value)
	{
		if (font)
			glyphs = font->transcript(value);
	}

	Vec2 TextItem::findRequestedSize()
	{
		if (font)
			return font->size(glyphs, format);
		return Vec2();
	}

	RenderableText TextItem::emit(Vec2 position, Vec2 size)
	{
		return RenderableText(this, position, size);
	}

	void TextItem::updateCursorPosition(Vec2 position, Vec2 size, Vec2 point, uint32 &cursor)
	{
		if (!font)
			return;
		FontFormat f = format;
		f.wrapWidth = size[0];
		font->size(glyphs, f, point - position, cursor);
	}

	void imageCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->image);
		item->image = item->impl->memory->createHolder<ImageItem>(item);
	}

	ImageItem::ImageItem(HierarchyItem *hierarchy) : hierarchy(hierarchy)
	{}

	void ImageItem::initialize()
	{
		if (skipInitialize)
			return;
		auto *impl = hierarchy->impl;
		if (GUI_HAS_COMPONENT(ImageFormat, hierarchy->ent))
		{
			GUI_COMPONENT(ImageFormat, f, hierarchy->ent);
			apply(f);
		}
		assign();
	}

	void ImageItem::assign()
	{
		GUI_COMPONENT(Image, i, hierarchy->ent);
		assign(i);
	}

	void ImageItem::assign(const GuiImageComponent &value)
	{
		image = value;
		texture = hierarchy->impl->assetOnDemand->get<AssetSchemeIndexTexture, Texture>(value.textureName);
	}

	void ImageItem::apply(const GuiImageFormatComponent &f)
	{
		format = f;
		// todo inherit only
	}

	Vec2 ImageItem::findRequestedSize()
	{
		if (texture)
			return Vec2(texture->resolution());
		return Vec2();
	}

	RenderableImage ImageItem::emit(Vec2 position, Vec2 size)
	{
		return RenderableImage(this, position, size);
	}
}
