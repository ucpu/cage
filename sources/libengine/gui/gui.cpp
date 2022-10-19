#include <cage-core/memoryAllocators.h>
#include <cage-core/macros.h>

#include <cage-engine/renderQueue.h>

#include "private.h"

#include <unordered_map>
#include <algorithm>

namespace cage
{
	GuiImpl::GuiImpl(const GuiManagerCreateConfig &config) : assetMgr(config.assetMgr), provisionalGraphics(config.provisionalGraphics)
	{
#define GCHL_GENERATE(T) entityMgr->defineComponent(CAGE_JOIN(Gui, CAGE_JOIN(T, Component))());
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE

		inputsListeners.attach(&inputsDispatchers);
		inputsListeners.windowResize.bind<GuiImpl, &GuiImpl::windowResize>(this);
		inputsListeners.mousePress.bind<GuiImpl, &GuiImpl::mousePress>(this);
		inputsListeners.mouseDoublePress.bind<GuiImpl, &GuiImpl::mouseDoublePress>(this);
		inputsListeners.mouseRelease.bind<GuiImpl, &GuiImpl::mouseRelease>(this);
		inputsListeners.mouseMove.bind<GuiImpl, &GuiImpl::mouseMove>(this);
		inputsListeners.mouseWheel.bind<GuiImpl, &GuiImpl::mouseWheel>(this);
		inputsListeners.keyPress.bind<GuiImpl, &GuiImpl::keyPress>(this);
		inputsListeners.keyRepeat.bind<GuiImpl, &GuiImpl::keyRepeat>(this);
		inputsListeners.keyRelease.bind<GuiImpl, &GuiImpl::keyRelease>(this);
		inputsListeners.keyChar.bind<GuiImpl, &GuiImpl::keyChar>(this);

		skins.resize(config.skinsCount);

		memory = newMemoryAllocatorStream({});
	}

	GuiImpl::~GuiImpl()
	{
		focusName = 0;
		hover = nullptr;
	}

	void GuiImpl::scaling()
	{
		pointsScale = zoom * retina;
		outputSize = Vec2(outputResolution[0], outputResolution[1]) / pointsScale;
		outputMouse = Vec2(inputMouse[0], inputMouse[1]) / pointsScale;
	}

	Vec4 GuiImpl::pointsToNdc(Vec2 position, Vec2 size) const
	{
		CAGE_ASSERT(size[0] >= 0 && size[1] >= 0);
		position *= pointsScale;
		size *= pointsScale;
		Vec2 os = outputSize * pointsScale;
		Vec2 invScreenSize = 2 / os;
		Vec2 resPos = (position - os * 0.5) * invScreenSize;
		Vec2 resSiz = size * invScreenSize;
		resPos[1] = -resPos[1] - resSiz[1];
		return Vec4(resPos, resPos + resSiz);
	}

	void GuiManager::outputResolution(Vec2i resolution)
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->outputResolution = resolution;
		impl->scaling();
	}

	Vec2i GuiManager::outputResolution() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->outputResolution;
	}

	void GuiManager::outputRetina(Real retina)
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->retina = retina;
		impl->scaling();
	}

	Real GuiManager::outputRetina() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->retina;
	}

	void GuiManager::zoom(Real zoom)
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->zoom = zoom;
		impl->scaling();
	}

	Real GuiManager::zoom() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->zoom;
	}

	void GuiManager::focus(uint32 widget)
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->focusName = widget;
		impl->focusParts = 1;
	}

	uint32 GuiManager::focus() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->focusName;
	}

	void GuiManager::invalidateInputs()
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->eventsEnabled = false;
	}

	Vec2i GuiManager::inputResolution() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->inputResolution;
	}

	GuiSkinConfig &GuiManager::skin(uint32 index)
	{
		GuiImpl *impl = (GuiImpl *)this;
		return impl->skins[index];
	}

	const GuiSkinConfig &GuiManager::skin(uint32 index) const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->skins[index];
	}

	EntityManager *GuiManager::entities()
	{
		GuiImpl *impl = (GuiImpl *)this;
		return impl->entityMgr.get();
	}

#define GCHL_GENERATE(T) void CAGE_JOIN(T, Create)(HierarchyItem *item);
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
	void textCreate(HierarchyItem *item);
	void imageCreate(HierarchyItem *item);
	void explicitSizeCreate(HierarchyItem *item);
	void scrollbarsCreate(HierarchyItem *item);

	namespace
	{
		void sortChildren(HierarchyItem *item)
		{
			std::sort(item->children.begin(), item->children.end(), [](const Holder<HierarchyItem> &a, const Holder<HierarchyItem> &b) {
				return a->order < b->order;
				});
			for (const auto &it : item->children)
				sortChildren(+it);
		}

		void generateHierarchy(GuiImpl *impl)
		{
			Holder<HierarchyItem> head = impl->memory->createHolder<HierarchyItem>(impl, nullptr);
			std::unordered_map<uint32, Holder<HierarchyItem>> map;
			// create all items
			for (auto e : impl->entityMgr->entities())
			{
				const uint32 name = e->name();
				CAGE_ASSERT(name != 0 && name != m);
				map[name] = impl->memory->createHolder<HierarchyItem>(impl, e);
			}
			// attach all items to their parents
			for (auto e : impl->entityMgr->entities())
			{
				const uint32 name = e->name();
				Holder<HierarchyItem> item = map[name].share();
				if (GUI_HAS_COMPONENT(Parent, e))
				{
					GUI_COMPONENT(Parent, p, e);
					CAGE_ASSERT(p.parent != 0 && p.parent != m && p.parent != name);
					CAGE_ASSERT(map.count(p.parent));
					item->order = p.order;
					map[p.parent]->children.push_back(std::move(item));
				}
				else
					head->children.push_back(std::move(item));
			}
			// create overlays pre-root
			impl->root = impl->memory->createHolder<HierarchyItem>(impl, nullptr);
			impl->root->children.push_back(std::move(head));
			// sort children
			sortChildren(+impl->root);
		}

		void generateItem(HierarchyItem *item)
		{
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
			const uint32 wc = entityWidgetsCount(item->ent);
			const bool sc = GUI_HAS_COMPONENT(Scrollbars, item->ent);
			const uint32 lc = entityLayoutsCount(item->ent);
			CAGE_ASSERT(wc <= 1);
			CAGE_ASSERT(lc <= 1);

			// widget
			if (wc)
			{
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->ent)) { CAGE_ASSERT(!item->item); CAGE_JOIN(T, Create)(item); }
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
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, item->ent)) { CAGE_ASSERT(!item->item); CAGE_JOIN(T, Create)(item); }
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
				CAGE_ASSERT(item->item);
			}
		}

		void generateItems(HierarchyItem *item)
		{
			if (item->ent && !item->subsidedItem)
				generateItem(item);
			for (const auto &it : item->children)
				generateItems(+it);
		}

		void propagateWidgetState(const GuiWidgetStateComponent &from, GuiWidgetStateComponent &to)
		{
			to.disabled = to.disabled || from.disabled;
			if (from.skinIndex != m)
				to.skinIndex = from.skinIndex;
		}

		void propagateWidgetState(HierarchyItem *item, const GuiWidgetStateComponent &wsp)
		{
			GuiWidgetStateComponent ws = wsp;
			if (item->ent && GUI_HAS_COMPONENT(WidgetState, item->ent))
			{
				GUI_COMPONENT(WidgetState, w, item->ent);
				propagateWidgetState(w, ws);
			}
			if (WidgetItem *wi = dynamic_cast<WidgetItem *>(+item->item))
			{
				GuiWidgetStateComponent &w = wi->widgetState;
				w = ws;
				CAGE_ASSERT(w.skinIndex < item->impl->skins.size());
				wi->skin = &item->impl->skins[w.skinIndex];
			}
			for (const auto &it : item->children)
				propagateWidgetState(+it, ws);
		}

		void callInitialize(HierarchyItem *item)
		{
			item->initialize();
			for (const auto &it : item->children)
				callInitialize(+it);
		}

		void findHover(GuiImpl *impl)
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

	void GuiManager::prepare()
	{
		GuiImpl *impl = (GuiImpl *)this;

		impl->mouseEventReceivers.clear();
		impl->root.clear();
		impl->eventsEnabled = true;

		generateHierarchy(impl);
		generateItems(+impl->root);

		{ // propagate widget state
			GuiWidgetStateComponent ws;
			ws.skinIndex = 0;
			propagateWidgetState(+impl->root, ws);
		}

		{ // initialize
			impl->root->initialize();
			// make sure that items added during initialization are initialized too
			std::size_t i = 0;
			while (i < impl->root->children.size())
				callInitialize(+impl->root->children[i++]);
		}

		{ // layouting
			impl->root->findRequestedSize();
			FinalPosition u;
			u.renderPos = u.clipPos = Vec2();
			u.renderSize = u.clipSize = impl->outputSize;
			impl->root->findFinalPosition(u);
		}

		impl->root->generateEventReceivers();

		findHover(impl);
	}

	Holder<RenderQueue> GuiManager::finish()
	{
		GuiImpl *impl = (GuiImpl *)this;
		Holder<RenderQueue> q = impl->emit();
		cleanUp();
		return q;
	}

	void GuiManager::cleanUp()
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->root.clear();
	}

	Holder<GuiManager> newGuiManager(const GuiManagerCreateConfig &config)
	{
		return systemMemory().createImpl<GuiManager, GuiImpl>(config);
	}

	uint32 entityWidgetsCount(Entity *e)
	{
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	uint32 entityLayoutsCount(Entity *e)
	{
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	void offsetPosition(Vec2 &position, Vec4 offset)
	{
		CAGE_ASSERT(position.valid() && offset.valid());
		position -= Vec2(offset);
	}

	void offsetSize(Vec2 &size, Vec4 offset)
	{
		CAGE_ASSERT(size.valid() && offset.valid());
		size += Vec2(offset) + Vec2(offset[2], offset[3]);
		size = max(size, Vec2());
	}

	void offset(Vec2 &position, Vec2 &size, Vec4 offset)
	{
		offsetPosition(position, offset);
		offsetSize(size, offset);
	}

	bool pointInside(Vec2 pos, Vec2 size, Vec2 point)
	{
		CAGE_ASSERT(pos.valid() && size.valid() && point.valid());
		if (point[0] < pos[0] || point[1] < pos[1])
			return false;
		pos += size;
		if (point[0] > pos[0] || point[1] > pos[1])
			return false;
		return true;
	}

	bool clip(Vec2 &pos, Vec2 &size, Vec2 clipPos, Vec2 clipSize)
	{
		CAGE_ASSERT(clipPos.valid());
		CAGE_ASSERT(clipSize.valid() && clipSize[0] >= 0 && clipSize[1] >= 0);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid() && size[0] >= 0 && size[1] >= 0);
		Vec2 e = min(pos + size, clipPos + clipSize);
		pos = max(pos, clipPos);
		size = max(e - pos, Vec2());
		return size[0] > 0 && size[1] > 0;
	}

	HierarchyItem *subsideItem(HierarchyItem *item)
	{
		Holder<HierarchyItem> n = item->impl->memory->createHolder<HierarchyItem>(item->impl, item->ent);
		std::swap(n->children, item->children);
		n->subsidedItem = true;
		CAGE_ASSERT(item->children.empty());
		item->children.push_back(std::move(n));
		return +item->children[0];
	}

	void ensureItemHasLayout(HierarchyItem *hierarchy)
	{
		if (entityLayoutsCount(hierarchy->ent) == 0)
			subsideItem(hierarchy); // fall back to default layouting
	}
}
