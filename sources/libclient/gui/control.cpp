#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/log.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "private.h"

#include <unordered_map>

namespace cage
{
#define GCHL_GENERATE(T) void CAGE_JOIN(T, Create)(guiItemStruct *item);
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
	void layoutZeroCreate(guiItemStruct *item);
	void layoutDefaultCreate(guiItemStruct *item);
	void textCreate(guiItemStruct *item);
	void imageCreate(guiItemStruct *item);

	namespace
	{
		void sortHierarchy(guiItemStruct *item)
		{
			if (!item->prevSibling)
				return;
			guiItemStruct *a = item->prevSibling->prevSibling;
			guiItemStruct *b = item->prevSibling;
			guiItemStruct *c = item;
			guiItemStruct *d = item->nextSibling;
			sint32 bo = b->order;
			sint32 co = c->order;
			if (co > bo)
				return;
			if (a)
				a->nextSibling = c;
			if (d)
				d->prevSibling = b;
			c->prevSibling = a;
			c->nextSibling = b;
			b->prevSibling = c;
			b->nextSibling = d;
			sortHierarchy(item);
		}

		void attachHierarchy(guiItemStruct *item, guiItemStruct *parent)
		{
			CAGE_ASSERT_RUNTIME(item && parent && !item->parent && !item->prevSibling && !item->nextSibling);
			item->parent = parent;
			if (!parent->lastChild)
			{
				CAGE_ASSERT_RUNTIME(!parent->firstChild);
				parent->firstChild = parent->lastChild = item;
			}
			else
			{
				CAGE_ASSERT_RUNTIME(parent->firstChild && parent->lastChild);
				CAGE_ASSERT_RUNTIME(!parent->lastChild->nextSibling);
				parent->lastChild->nextSibling = item;
				item->prevSibling = parent->lastChild;
				sortHierarchy(item);
				if (!item->prevSibling)
					parent->firstChild = item;
				if (!item->nextSibling)
					parent->lastChild = item;
			}
		}

		void generateHierarchy(guiImpl *impl)
		{
			guiItemStruct *root = impl->itemsMemory.createObject<guiItemStruct>(impl, nullptr);
			std::unordered_map<uint32, guiItemStruct*> map;
			// create all items
			for (auto e : impl->entityManager->getAllEntities()->entities())
			{
				uint32 name = e->getName();
				CAGE_ASSERT_RUNTIME(name != 0 && name != -1, name);
				guiItemStruct *item = impl->itemsMemory.createObject<guiItemStruct>(impl, e);
				map[name] = item;
			}
			// attach all items into the hierarchy
			for (auto e : impl->entityManager->getAllEntities()->entities())
			{
				uint32 name = e->getName();
				guiItemStruct *item = map[name];
				if (GUI_HAS_COMPONENT(parent, e))
				{
					GUI_GET_COMPONENT(parent, p, e);
					CAGE_ASSERT_RUNTIME(p.parent != 0 && p.parent != -1 && p.parent != name, p.parent, name);
					CAGE_ASSERT_RUNTIME(map.find(p.parent) != map.end(), p.parent, name);
					item->order = p.order;
					attachHierarchy(item, map[p.parent]);
				}
				else
					attachHierarchy(item, root);
			}
			// create overlays pre-root
			guiItemStruct *preroot = impl->itemsMemory.createObject<guiItemStruct>(impl, nullptr);
			root->attachParent(preroot);
			impl->root = preroot;
		}

		void generateWidgetsAndLayouts(guiItemStruct *item)
		{
			guiImpl *impl = item->impl;
			if (item->entity)
			{
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->entity)) { CAGE_ASSERT_RUNTIME(!item->widget, item->entity->getName()); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->entity)) { CAGE_ASSERT_RUNTIME(!item->layout, item->entity->getName()); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
				if (GUI_HAS_COMPONENT(text, item->entity))
				{
					CAGE_ASSERT_RUNTIME(!item->text);
					textCreate(item);
				}
				if (GUI_HAS_COMPONENT(image, item->entity))
				{
					CAGE_ASSERT_RUNTIME(!item->image);
					imageCreate(item);
				}
			}
			if (item->firstChild)
				generateWidgetsAndLayouts(item->firstChild);
			if (item->nextSibling)
				generateWidgetsAndLayouts(item->nextSibling);
		}

		void propagate(const widgetStateComponent &from, widgetStateComponent &to)
		{
			to.disabled = to.disabled || from.disabled;
			if (from.skinIndex != -1)
				to.skinIndex = from.skinIndex;
		}

		void propagateWidgetState(guiItemStruct *item, const widgetStateComponent &wsp)
		{
			guiImpl *impl = item->impl;
			{
				widgetStateComponent ws(wsp);
				if (item->widget)
				{
					widgetStateComponent &w = item->widget->widgetState;
					propagate(w, ws);
					propagate(ws, w);
				}
				else if (item->entity && GUI_HAS_COMPONENT(widgetState, item->entity))
				{
					GUI_GET_COMPONENT(widgetState, w, item->entity);
					propagate(w, ws);
				}
				if (item->firstChild)
					propagateWidgetState(item->firstChild, ws);
			}
			if (item->nextSibling)
				propagateWidgetState(item->nextSibling, wsp);
		}

		void callInitialize(guiItemStruct *item)
		{
			item->initialize();
			if (item->firstChild)
				callInitialize(item->firstChild);
			if (item->nextSibling)
				callInitialize(item->nextSibling);
		}

		void generateEventReceivers(guiItemStruct *item)
		{
			if (item->nextSibling)
				generateEventReceivers(item->nextSibling);
			if (item->firstChild)
				generateEventReceivers(item->firstChild);
			if (item->widget)
				item->impl->mouseEventReceivers.push_back(item->widget);
		}

		void findHover(guiImpl *impl)
		{
			impl->hover = nullptr;
			for (auto it : impl->mouseEventReceivers)
			{
				if (pointInside(it->base->position, it->base->size, impl->outputMouse))
				{
					impl->hover = it;
					return;
				}
			}
		}
	}

	void guiClass::controlUpdate()
	{
		guiImpl *impl = (guiImpl*)this;
		{ // clearing
			impl->mouseEventReceivers.clear();
			impl->root = nullptr;
			impl->itemsMemory.flush();
		}
		impl->eventsEnabled = true;
		generateHierarchy(impl);
		generateWidgetsAndLayouts(impl->root);
		layoutZeroCreate(impl->root);
		layoutDefaultCreate(impl->root->firstChild);
		{ // propagate widget state
			widgetStateComponent ws;
			ws.skinIndex = 0;
			propagateWidgetState(impl->root, ws);
		}
		callInitialize(impl->root);
		{ // layouting
			impl->root->updateRequestedSize();
			updatePositionStruct u;
			u.position = vec2();
			impl->root->updateFinalPosition(u);
		}
		generateEventReceivers(impl->root);
		findHover(impl);
	}

	void guiClass::controlEmit()
	{
		guiImpl *impl = (guiImpl*)this;
		if (!impl->assetManager->ready(hashString("cage/cage.pack")))
			return;
		for (auto &s : impl->skins)
		{
			s.texture = impl->assetManager->get<assetSchemeIndexTexture, textureClass>(s.textureName);
			if (!s.texture)
				return;
		}
		if ((impl->emitIndexControl + 1) % 3 == impl->emitIndexDispatch)
		{
			CAGE_LOG(severityEnum::Warning, "gui", "gui is skipping emit because dispatching is late");
			return;
		}

		impl->graphicData.guiShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/element.glsl"));
		impl->graphicData.fontShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/font.glsl"));
		impl->graphicData.imageAnimatedShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/image.glsl?A"));
		impl->graphicData.imageStaticShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/image.glsl?a"));
		impl->graphicData.colorPickerShader[0] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?F"));
		impl->graphicData.colorPickerShader[1] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?H"));
		impl->graphicData.colorPickerShader[2] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?S"));
		impl->graphicData.guiMesh = impl->assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/guiElement.obj"));
		impl->graphicData.fontMesh = impl->assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
		impl->graphicData.imageMesh = impl->graphicData.fontMesh;

		impl->emitIndexControl = (impl->emitIndexControl + 1) % 3;
		impl->emitControl = &impl->emitData[impl->emitIndexControl];
		auto *e = impl->emitControl;
		e->flush();
		e->first = e->last = e->memory.createObject<renderableBaseStruct>(); // create first dummy object

		impl->root->childrenEmit();
	}
}
