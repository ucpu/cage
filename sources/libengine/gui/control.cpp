#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferGuard.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "private.h"

#include <unordered_map>

namespace cage
{
#define GCHL_GENERATE(T) void CAGE_JOIN(T, Create)(hierarchyItemStruct *item);
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
	void textCreate(hierarchyItemStruct *item);
	void imageCreate(hierarchyItemStruct *item);
	void explicitSizeCreate(hierarchyItemStruct *item);
	void scrollbarsCreate(hierarchyItemStruct *item);
	void printDebug(guiImpl *impl);

	namespace
	{
		void sortHierarchy(hierarchyItemStruct *item)
		{
			if (!item->prevSibling)
				return;
			hierarchyItemStruct *a = item->prevSibling->prevSibling;
			hierarchyItemStruct *b = item->prevSibling;
			hierarchyItemStruct *c = item;
			hierarchyItemStruct *d = item->nextSibling;
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

		void attachHierarchy(hierarchyItemStruct *item, hierarchyItemStruct *parent)
		{
			CAGE_ASSERT(item && parent && !item->parent && !item->prevSibling && !item->nextSibling);
			item->parent = parent;
			if (!parent->lastChild)
			{
				CAGE_ASSERT(!parent->firstChild);
				parent->firstChild = parent->lastChild = item;
			}
			else
			{
				CAGE_ASSERT(parent->firstChild && parent->lastChild);
				CAGE_ASSERT(!parent->lastChild->nextSibling);
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
			hierarchyItemStruct *root = impl->itemsMemory.createObject<hierarchyItemStruct>(impl, nullptr);
			std::unordered_map<uint32, hierarchyItemStruct*> map;
			// create all items
			for (auto e : impl->entityMgr->entities())
			{
				uint32 name = e->name();
				CAGE_ASSERT(name != 0 && name != m, name);
				hierarchyItemStruct *item = impl->itemsMemory.createObject<hierarchyItemStruct>(impl, e);
				map[name] = item;
			}
			// attach all items into the hierarchy
			for (auto e : impl->entityMgr->entities())
			{
				uint32 name = e->name();
				hierarchyItemStruct *item = map[name];
				if (GUI_HAS_COMPONENT(Parent, e))
				{
					CAGE_COMPONENT_GUI(Parent, p, e);
					CAGE_ASSERT(p.parent != 0 && p.parent != m && p.parent != name, p.parent, name);
					CAGE_ASSERT(map.find(p.parent) != map.end(), p.parent, name);
					item->order = p.order;
					attachHierarchy(item, map[p.parent]);
				}
				else
					attachHierarchy(item, root);
			}
			// create overlays pre-root
			hierarchyItemStruct *preroot = impl->itemsMemory.createObject<hierarchyItemStruct>(impl, nullptr);
			root->attachParent(preroot);
			impl->root = preroot;
		}

		void generateItem(hierarchyItemStruct *item)
		{
			guiImpl *impl = item->impl;

			// explicit size
			if (GUI_HAS_COMPONENT(ExplicitSize, item->ent))
			{
				explicitSizeCreate(item);
				item = subsideItem(item);
			}

			// text and image
			if (GUI_HAS_COMPONENT(Text, item->ent))
				textCreate(item);
			if (GUI_HAS_COMPONENT(Image, item->ent))
				imageCreate(item);

			// counts
			uint32 wc = impl->entityWidgetsCount(item->ent);
			bool sc = GUI_HAS_COMPONENT(Scrollbars, item->ent);
			uint32 lc = impl->entityLayoutsCount(item->ent);
			CAGE_ASSERT(wc <= 1, item->ent->name());
			CAGE_ASSERT(lc <= 1, item->ent->name());

			// widget
			if (wc)
			{
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->ent)) { CAGE_ASSERT(!item->item, item->ent->name()); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
				CAGE_ASSERT(item->item);
				if (sc || !!lc)
					item = subsideItem(item);
			}

			// scrollbars
			if (sc)
			{
				scrollbarsCreate(item);
				CAGE_ASSERT(item->item);
				if (!!lc)
					item = subsideItem(item);
			}

			// layouter
			if (lc)
			{
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->ent)) { CAGE_ASSERT(!item->item, item->ent->name()); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
				CAGE_ASSERT(item->item);
			}
		}

		void generateItems(hierarchyItemStruct *item)
		{
			if (item->ent && !item->subsidedItem)
				generateItem(item);
			if (item->firstChild)
				generateItems(item->firstChild);
			if (item->nextSibling)
				generateItems(item->nextSibling);
		}

		void propagateWidgetState(const WidgetStateComponent &from, WidgetStateComponent &to)
		{
			to.disabled = to.disabled || from.disabled;
			if (from.skinIndex != m)
				to.skinIndex = from.skinIndex;
		}

		void propagateWidgetState(hierarchyItemStruct *item, const WidgetStateComponent &wsp)
		{
			guiImpl *impl = item->impl;
			{
				WidgetStateComponent ws(wsp);
				if (item->ent && GUI_HAS_COMPONENT(WidgetState, item->ent))
				{
					CAGE_COMPONENT_GUI(WidgetState, w, item->ent);
					propagateWidgetState(w, ws);
				}
				if (widgetItemStruct *wi = dynamic_cast<widgetItemStruct*>(item->item))
				{
					WidgetStateComponent &w = wi->widgetState;
					w = ws;
					CAGE_ASSERT(w.skinIndex < impl->skins.size(), w.skinIndex, impl->skins.size());
					wi->skin = &impl->skins[w.skinIndex];
				}
				if (item->firstChild)
					propagateWidgetState(item->firstChild, ws);
			}
			if (item->nextSibling)
				propagateWidgetState(item->nextSibling, wsp);
		}

		void callInitialize(hierarchyItemStruct *item)
		{
			item->initialize();
			if (item->firstChild)
				callInitialize(item->firstChild);
			if (item->nextSibling)
				callInitialize(item->nextSibling);
		}

		void validateHierarchy(hierarchyItemStruct *item)
		{
			if (item->prevSibling)
			{
				CAGE_ASSERT(item->prevSibling->nextSibling == item);
			}
			if (item->nextSibling)
			{
				CAGE_ASSERT(item->nextSibling->prevSibling == item);
			}
			CAGE_ASSERT(!!item->firstChild == !!item->lastChild);
			if (item->firstChild)
			{
				CAGE_ASSERT(item->firstChild->prevSibling == nullptr);
				hierarchyItemStruct *it = item->firstChild, *last = item->firstChild;
				while (it)
				{
					CAGE_ASSERT(it->parent == item);
					validateHierarchy(it);
					last = it;
					it = it->nextSibling;
				}
				CAGE_ASSERT(item->lastChild == last);
			}
			if (item->item)
			{
				CAGE_ASSERT(item->item->hierarchy == item);
			}
		}

		void validateHierarchy(guiImpl *impl)
		{
#ifdef CAGE_ASSERT_ENABLED
			validateHierarchy(impl->root);
#endif // CAGE_ASSERT_ENABLED
		}

		void findHover(guiImpl *impl)
		{
			impl->hover = nullptr;
			for (const auto &it : impl->mouseEventReceivers)
			{
				if (it.pointInside(impl->outputMouse))
				{
					impl->hover = it.widget;
					return;
				}
			}
		}
	}

	void Gui::controlUpdateStart()
	{
		guiImpl *impl = (guiImpl*)this;
		{ // clearing
			impl->mouseEventReceivers.clear();
			impl->root = nullptr;
			impl->itemsMemory.flush();
		}
		impl->eventsEnabled = true;
		generateHierarchy(impl);
		generateItems(impl->root);
		validateHierarchy(impl);
		{ // propagate widget state
			WidgetStateComponent ws;
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
		impl->root->generateEventReceivers();
		findHover(impl);
		printDebug(impl);
	}

	void Gui::controlUpdateDone()
	{
		guiImpl *impl = (guiImpl*)this;

		if (!impl->eventsEnabled)
			return;

		if (!impl->assetMgr->ready(HashString("cage/cage.pack")))
			return;

		for (auto &s : impl->skins)
		{
			if (impl->assetMgr->ready(s.textureName))
				s.texture = impl->assetMgr->get<assetSchemeIndexRenderTexture, Texture>(s.textureName);
			else
				s.texture = nullptr;
		}

		if (auto lock = impl->emitController->write())
		{
			impl->graphicsData.debugShader = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/debug.glsl"));
			impl->graphicsData.elementShader = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/element.glsl"));
			impl->graphicsData.fontShader = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
			impl->graphicsData.imageAnimatedShader = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?A"));
			impl->graphicsData.imageStaticShader = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?a"));
			impl->graphicsData.colorPickerShader[0] = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?F"));
			impl->graphicsData.colorPickerShader[1] = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?H"));
			impl->graphicsData.colorPickerShader[2] = impl->assetMgr->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?S"));
			impl->graphicsData.debugMesh = impl->assetMgr->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/guiWire.obj"));
			impl->graphicsData.elementMesh = impl->assetMgr->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/guiElement.obj"));
			impl->graphicsData.fontMesh = impl->assetMgr->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/square.obj"));
			impl->graphicsData.imageMesh = impl->graphicsData.fontMesh;

			impl->emitControl = &impl->emitData[lock.index()];
			auto *e = impl->emitControl;
			e->flush();
			e->first = e->last = e->memory.createObject<renderableBaseStruct>(); // create first dummy object

			impl->root->childrenEmit();
		}
		else
		{
			CAGE_LOG_DEBUG(SeverityEnum::Warning, "gui", "gui is skipping emit because control update is late");
		}
	}
}
