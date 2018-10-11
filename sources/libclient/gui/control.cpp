#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/log.h>
#include <cage-core/assets.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferController.h>

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
	void layoutRootCreate(guiItemStruct *item);
	void layoutDefaultCreate(guiItemStruct *item);
	void textCreate(guiItemStruct *item);
	void imageCreate(guiItemStruct *item);
	void scrollbarsCreate(guiItemStruct *item);

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
			for (auto e : impl->entityManager->entities())
			{
				uint32 name = e->name();
				CAGE_ASSERT_RUNTIME(name != 0 && name != -1, name);
				guiItemStruct *item = impl->itemsMemory.createObject<guiItemStruct>(impl, e);
				map[name] = item;
			}
			// attach all items into the hierarchy
			for (auto e : impl->entityManager->entities())
			{
				uint32 name = e->name();
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
			if (item->entity && !item->subsidedItem)
			{
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->entity)) { CAGE_ASSERT_RUNTIME(!item->widget, item->entity->name()); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->entity)) { CAGE_ASSERT_RUNTIME(!item->layout, item->entity->name()); CAGE_JOIN(T, Create)(item); }
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
				// scrollbars are allowed together with some widgets
				if (GUI_HAS_COMPONENT(scrollbars, item->entity))
				{
					if (item->widget)
						CAGE_ASSERT_RUNTIME(item->widget->canBeMergedWithScrollbars(), item->entity->name());
					scrollbarsCreate(item);
				}
			}
			if (item->firstChild)
				generateWidgetsAndLayouts(item->firstChild);
			if (item->nextSibling)
				generateWidgetsAndLayouts(item->nextSibling);
		}

		void propagateWidgetState(const widgetStateComponent &from, widgetStateComponent &to)
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
					propagateWidgetState(w, ws);
					propagateWidgetState(ws, w);
				}
				else if (item->entity && GUI_HAS_COMPONENT(widgetState, item->entity))
				{
					GUI_GET_COMPONENT(widgetState, w, item->entity);
					propagateWidgetState(w, ws);
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

		void validateHierarchy(guiItemStruct *item)
		{
			if (item->prevSibling)
				CAGE_ASSERT_RUNTIME(item->prevSibling->nextSibling == item);
			if (item->nextSibling)
				CAGE_ASSERT_RUNTIME(item->nextSibling->prevSibling == item);
			CAGE_ASSERT_RUNTIME(!!item->firstChild == !!item->lastChild);
			if (item->firstChild)
			{
				CAGE_ASSERT_RUNTIME(item->firstChild->prevSibling == nullptr);
				guiItemStruct *it = item->firstChild, *last = item->firstChild;
				while (it)
				{
					CAGE_ASSERT_RUNTIME(it->parent == item);
					validateHierarchy(it);
					last = it;
					it = it->nextSibling;
				}
				CAGE_ASSERT_RUNTIME(item->lastChild == last);
			}
			if (item->widget)
				CAGE_ASSERT_RUNTIME(item->widget->base == item);
			if (item->layout)
				CAGE_ASSERT_RUNTIME(item->layout->base == item);
		}

		void validateHierarchy(guiImpl *impl)
		{
#ifdef CAGE_ASSERT_ENABLED
			validateHierarchy(impl->root);
#endif // CAGE_ASSERT_ENABLED
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
				if (pointInside(it->base->renderPos, it->base->renderSize, impl->outputMouse))
				{
					impl->hover = it;
					return;
				}
			}
		}
	}

	void guiClass::controlUpdateStart()
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
		validateHierarchy(impl);
		layoutRootCreate(impl->root);
		layoutDefaultCreate(impl->root->firstChild);
		{ // propagate widget state
			widgetStateComponent ws;
			ws.skinIndex = 0;
			propagateWidgetState(impl->root, ws);
		}
		callInitialize(impl->root);
		validateHierarchy(impl);
		{ // layouting
			impl->root->findRequestedSize();
			finalPositionStruct u;
			u.renderPos = u.clipPos = vec2();
			u.renderSize = u.clipSize = impl->outputSize;
			impl->root->findFinalPosition(u);
		}
		generateEventReceivers(impl->root);
		findHover(impl);
	}

	void guiClass::controlUpdateDone()
	{
		guiImpl *impl = (guiImpl*)this;

		if (!impl->eventsEnabled)
			return;

		if (!impl->assetManager->ready(hashString("cage/cage.pack")))
			return;

		for (auto &s : impl->skins)
		{
			if (impl->assetManager->ready(s.textureName))
				s.texture = impl->assetManager->get<assetSchemeIndexTexture, textureClass>(s.textureName);
			else
				s.texture = nullptr;
		}

		if (auto lock = impl->emitController->write())
		{
			impl->graphicsData.debugShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/debug.glsl"));
			impl->graphicsData.elementShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/element.glsl"));
			impl->graphicsData.fontShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/font.glsl"));
			impl->graphicsData.imageAnimatedShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/image.glsl?A"));
			impl->graphicsData.imageStaticShader = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/image.glsl?a"));
			impl->graphicsData.colorPickerShader[0] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?F"));
			impl->graphicsData.colorPickerShader[1] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?H"));
			impl->graphicsData.colorPickerShader[2] = impl->assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/colorPicker.glsl?S"));
			impl->graphicsData.debugMesh = impl->assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/guiWire.obj"));
			impl->graphicsData.elementMesh = impl->assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/guiElement.obj"));
			impl->graphicsData.fontMesh = impl->assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
			impl->graphicsData.imageMesh = impl->graphicsData.fontMesh;

			impl->emitControl = &impl->emitData[lock.index()];
			auto *e = impl->emitControl;
			e->flush();
			e->first = e->last = e->memory.createObject<renderableBaseStruct>(); // create first dummy object

			impl->root->childrenEmit();
		}
		else
		{
			CAGE_LOG_DEBUG(severityEnum::Warning, "gui", "gui is skipping emit because control update is late");
		}
	}
}
