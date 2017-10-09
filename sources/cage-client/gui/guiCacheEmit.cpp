#include <map>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/utility/utf.h>
#include <cage-core/utility/textPack.h>

#include "controls/privateControls.h"
#include "layouts/privateLayouts.h"

#include <cage-client/assets.h>

namespace cage
{
	namespace
	{
		controlCacheStruct *createControl(guiImpl *context, entityClass *e)
		{
			controlCacheStruct *control = nullptr;
			GUI_GET_COMPONENT(control, c, e);
			switch (c.controlType)
			{
			case controlTypeEnum::Empty: control = context->memoryCache.createObject<emptyControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Panel: control = context->memoryCache.createObject<panelControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Button: control = context->memoryCache.createObject<buttonControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::RadioButton: control = context->memoryCache.createObject<radioButtonControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::RadioGroup: control = context->memoryCache.createObject<radioGroupControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Checkbox: control = context->memoryCache.createObject<checkboxControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Textbox: control = context->memoryCache.createObject<textboxControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Textarea: control = context->memoryCache.createObject<textareaControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Combobox: control = context->memoryCache.createObject<comboboxControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Listbox: control = context->memoryCache.createObject<listboxControlCacheStruct>(context, e->getName()); break;
			case controlTypeEnum::Slider: control = context->memoryCache.createObject<sliderControlCacheStruct>(context, e->getName()); break;
			default: CAGE_THROW_CRITICAL(exception, "invalid control type");
			}
			return control;
		}

		controlCacheStruct *createLayout(guiImpl *context, entityClass *e)
		{
			controlCacheStruct *layout = nullptr;
			GUI_GET_COMPONENT(layout, l, e);
			switch (l.layoutMode)
			{
			case layoutModeEnum::Manual: layout = context->memoryCache.createObject<manualLayoutCacheStruct>(context, e->getName()); break;
			case layoutModeEnum::Row: layout = context->memoryCache.createObject<rowLayoutCacheStruct>(context, e->getName()); break;
			case layoutModeEnum::Column: layout = context->memoryCache.createObject<columnLayoutCacheStruct>(context, e->getName()); break;
			default: CAGE_THROW_CRITICAL(exception, "invalid layout mode");
			}
			return layout;
		}

		void applyFormat(guiImpl *context, itemCacheStruct *item, formatComponent &f)
		{
			for (uint32 i = 0; i < 3; i++)
				item->color[i] = f.color[i].valid() ? f.color[i].value : item->color[i];
			item->font = f.fontName && context->assetManager->ready(f.fontName) ? context->assetManager->get<assetSchemeIndexFont, fontClass>(f.fontName) : item->font;
			item->align = f.align != (textAlignEnum)-1 ? f.align : item->align;
			item->lineSpacing = f.lineSpacing != (uint8)-1 ? f.lineSpacing : item->lineSpacing;
		}

		const string loadInternationalizedText(guiImpl *context, uint32 asset, uint32 text, string params)
		{
			auto a = context->assetManager->get<assetSchemeIndexTextPackage, textPackClass>(asset);
			if (a)
			{
				// todo split parameters
				return a->format(text, 0, nullptr);
			}
			else
				return "";
		}

		itemCacheStruct *createText(guiImpl *context, entityClass *e)
		{
			itemCacheStruct *item = context->memoryCache.createObject<itemCacheStruct>(context, itemCacheStruct::itText, e->getName());
			{ // apply values from parent
				if (e->hasComponent(context->components.parent))
				{
					GUI_GET_COMPONENT(parent, p, e);
					if (p.parent)
					{
						CAGE_ASSERT_RUNTIME(context->entityManager->hasEntity(p.parent), "text has invalid parent", p.parent);
						entityClass *pe = context->entityManager->getEntity(p.parent);
						if (pe->hasComponent(context->components.format))
						{
							GUI_GET_COMPONENT(format, pf, pe);
							applyFormat(context, item, pf);
						}
					}
				}
			}
			if (e->hasComponent(context->components.format))
			{ // apply values from own format
				GUI_GET_COMPONENT(format, ff, e);
				applyFormat(context, item, ff);
			}
			if (item->font)
			{
				GUI_GET_COMPONENT(text, t, e);
				string val;
				if (t.assetName && t.textName && context->assetManager->ready(t.assetName))
					val = loadInternationalizedText(context, t.assetName, t.textName, t.value);
				else
					val = t.value;
				item->font->transcript(val.c_str(), nullptr, item->translatedLength);
				item->translatedText = (uint32*)context->memoryCache.allocate(item->translatedLength * sizeof(uint32));
				item->font->transcript(val.c_str(), item->translatedText, item->translatedLength);
			}
			return item;
		}

		itemCacheStruct *createImage(guiImpl *context, entityClass *e)
		{
			itemCacheStruct *item = context->memoryCache.createObject<itemCacheStruct>(context, itemCacheStruct::itImage, e->getName());
			GUI_GET_COMPONENT(image, im, e);
			item->animationStart = im.animationStart;
			item->animationSpeed = im.animationSpeed.value;
			item->animationOffset = im.animationOffset.value;
			item->uvClip[0] = im.tx.value;
			item->uvClip[1] = im.ty.value;
			item->uvClip[2] = (im.tx + im.tw).value;
			item->uvClip[3] = (im.ty + im.th).value;
			if (im.textureName && context->assetManager->ready(im.textureName))
				item->texture = context->assetManager->get<assetSchemeIndexTexture, textureClass>(im.textureName);
			return item;
		}

		const sint32 getOrdering(guiImpl *context, uint32 name)
		{
			entityClass *e = context->entityManager->getEntity(name);
			if (e->hasComponent(context->components.parent))
			{
				GUI_GET_COMPONENT(parent, p, e);
				return p.ordering;
			}
			return 0;
		}

		void attachControl(guiImpl *context, controlCacheStruct *control, controlCacheStruct *parent, sint32 ordering)
		{
			CAGE_ASSERT_RUNTIME(!control->parent);
			control->parent = parent;
			if (!parent->firstChild)
			{
				parent->firstChild = control;
				return;
			}
			controlCacheStruct *i = parent->firstChild;
			if (getOrdering(context, i->entity) > ordering)
			{
				control->nextSibling = parent->firstChild;
				parent->firstChild = control;
				return;
			}
			while (i->nextSibling && getOrdering(context, i->nextSibling->entity) <= ordering)
				i = i->nextSibling;
			control->nextSibling = i->nextSibling;
			i->nextSibling = control;
		}

		void attachItem(guiImpl *context, itemCacheStruct *item, controlCacheStruct *control, sint32 ordering)
		{
			if (!control->firstItem)
			{
				control->firstItem = item;
				return;
			}
			itemCacheStruct *i = control->firstItem;
			if (getOrdering(context, i->entity) > ordering)
			{
				item->next = control->firstItem;
				control->firstItem = item;
				return;
			}
			while (i->next && getOrdering(context, i->next->entity) <= ordering)
				i = i->next;
			item->next = i->next;
			i->next = item;
		}

		void attachItem(guiImpl *context, entityClass *e, itemCacheStruct *item, std::map<uint32, controlCacheStruct *> &pointers)
		{
			if (e->hasComponent(context->components.control))
			{
				attachItem(context, item, pointers[e->getName()], 0);
				return;
			}
			CAGE_ASSERT_RUNTIME(e->hasComponent(context->components.parent), "an item (text or image) must be attached directly to a control, orr be a children of a control", e->getName());
			GUI_GET_COMPONENT(parent, p, e);
			CAGE_ASSERT_RUNTIME(pointers.find(p.parent) != pointers.end(), "an item (text or image) has invalid parent", e->getName());
			CAGE_ASSERT_RUNTIME(pointers[p.parent]->elementType != elementTypeEnum::InvalidElement, "an item (text or image) has parent, who is not a control", e->getName());
			attachItem(context, item, pointers[p.parent], p.ordering);
		}

		void makeReverseConnections(controlCacheStruct *control)
		{
			controlCacheStruct *prev = nullptr;
			while (control)
			{
				if (control->firstChild)
					makeReverseConnections(control->firstChild);
				control->prevSibling = prev;
				prev = control;
				control = control->nextSibling;
			}
			if (prev->parent)
				prev->parent->lastChild = prev;
		}

		void findHover(controlCacheStruct *control)
		{
			// search is in reverse order (front to back)
			while (control)
			{
				findHover(control->lastChild);
				if (control->controlMode == controlCacheStruct::cmNormal || control->controlMode == controlCacheStruct::cmFocus || control->controlMode == controlCacheStruct::cmHover)
				{
					if (control->context->hoverName == 0 && control->isPointInside(control->context->mousePosition))
					{
						control->context->hoverName = control->entity;
						control->controlMode = controlCacheStruct::cmHover;
					}
					else if (control->context->focusName == control->entity)
						control->controlMode = controlCacheStruct::cmFocus;
					else
						control->controlMode = controlCacheStruct::cmNormal;
				}
				control = control->prevSibling;
			}
		}
	}

	void guiImpl::performCacheCreate()
	{
		{ // clean up
			cacheRoot = nullptr;
			memoryCache.flush();
		}

		try
		{
			{ // generate the hierarchy
				std::map<uint32, controlCacheStruct *> pointers;
				cacheRoot = memoryCache.createObject<manualLayoutCacheStruct>(this, 0);
				controlCacheStruct *layout = memoryCache.createObject<manualLayoutCacheStruct>(this, 0);
				attachControl(this, layout, cacheRoot, 0);

				{ // generate all controls
					groupClass *all = components.control->getComponentEntities();
					entityClass *const *ents = all->entitiesArray();
					for (uint32 i = 0, e = all->entitiesCount(); i != e; i++)
					{
						CAGE_ASSERT_RUNTIME(ents[i]->getName() > 0);
						CAGE_ASSERT_RUNTIME(!ents[i]->hasComponent(components.layout), "no entity can be both layout and control", ents[i]);
						pointers[ents[i]->getName()] = createControl(this, ents[i]);
					}
				}

				{ // generate all layouts
					groupClass *all = components.layout->getComponentEntities();
					entityClass *const *ents = all->entitiesArray();
					for (uint32 i = 0, e = all->entitiesCount(); i != e; i++)
					{
						CAGE_ASSERT_RUNTIME(ents[i]->getName() > 0);
						CAGE_ASSERT_RUNTIME(!ents[i]->hasComponent(components.control), "no entity can be both layout and control", ents[i]);
						pointers[ents[i]->getName()] = createLayout(this, ents[i]);
					}
				}

				{ // create all texts
					groupClass *all = components.text->getComponentEntities();
					entityClass *const *ents = all->entitiesArray();
					for (uint32 i = 0, e = all->entitiesCount(); i != e; i++)
					{
						CAGE_ASSERT_RUNTIME(ents[i]->getName() > 0);
						attachItem(this, ents[i], createText(this, ents[i]), pointers);
					}
				}

				{ // create all images
					groupClass *all = components.image->getComponentEntities();
					entityClass *const *ents = all->entitiesArray();
					for (uint32 i = 0, e = all->entitiesCount(); i != e; i++)
					{
						CAGE_ASSERT_RUNTIME(ents[i]->getName() > 0);
						attachItem(this, ents[i], createImage(this, ents[i]), pointers);
					}
				}

				{ // make forward hierarchy connections
					for (auto it = pointers.begin(), et = pointers.end(); it != et; it++)
					{
						entityClass *e = entityManager->getEntity(it->second->entity);
						if (e->hasComponent(components.parent))
						{
							guiClass *context = this;
							GUI_GET_COMPONENT(parent, p, e);
							if (p.parent == 0)
								attachControl(this, it->second, layout, p.ordering);
							else
							{
								CAGE_ASSERT_RUNTIME(pointers.find(p.parent) != pointers.end(), "control (or layout) has invalid parent", it->first);
								attachControl(this, it->second, pointers[p.parent], p.ordering);
							}
						}
						else
							attachControl(this, it->second, layout, 0);
					}
				}
			}

			{ // make reverse hierarchy connections
				makeReverseConnections(cacheRoot);
			}

			{ // generate positions
				cacheRoot->updateRequestedSize();
				cacheRoot->pixelsEnvelopePosition = vec2();
				cacheRoot->pixelsEnvelopeSize = windowSize;
				cacheRoot->updatePositionSize();
			}

			performCacheUpdate();
		}
		catch (...)
		{
			cacheRoot = nullptr;
			throw;
		}
	}

	void guiImpl::performCacheUpdate()
	{
		if (!cacheRoot)
			return;
		hoverName = 0;
		findHover(cacheRoot);
	}
}