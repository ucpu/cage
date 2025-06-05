#include <algorithm>
#include <unordered_map>

#include "private.h"

#include <cage-core/assetsOnDemand.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/macros.h>
#include <cage-core/memoryAllocators.h>
#include <cage-engine/renderQueue.h>

#define GCHL_GUI_COMMON_COMPONENTS Parent, Image, ImageFormat, Text, TextFormat, Selection, WidgetState, SelectedItem, LayoutScrollbars, LayoutAlignment, ExplicitSize, Event, Update, Tooltip, TooltipMarker
#define GCHL_GUI_WIDGET_COMPONENTS Label, Header, Separator, Button, Input, TextArea, CheckBox, RadioBox, ComboBox, ProgressBar, SliderBar, ColorPicker, SolidColor, Frame, Panel, Spoiler
#define GCHL_GUI_LAYOUT_COMPONENTS LayoutLine, LayoutSplit, LayoutTable

namespace cage
{
	GuiImpl::GuiImpl(const GuiManagerCreateConfig &config) : assetOnDemand(newAssetsOnDemand(config.assetManager)), assetMgr(config.assetManager), provisionalGraphics(config.provisionalGraphics), soundsQueue(config.soundsQueue), ttEnabled(config.tooltipsEnabled)
	{
#define GCHL_GENERATE(T) entityMgr->defineComponent(CAGE_JOIN(Gui, CAGE_JOIN(T, Component))());
		CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS));
		CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
		CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		entityMgr->defineComponent(GuiTooltipStringComponent());

		skins.reserve(config.skinsCount);
		for (uint32 i = 0; i < config.skinsCount; i++)
		{
			SkinData d;
			(GuiSkinConfig &)d = detail::guiSkinGenerate(GuiSkinIndex(i));
			skins.push_back(std::move(d));
		}

		memory = newMemoryAllocatorStream({});

		ttRemovedListener.attach(entityMgr->entityRemoved);
		ttRemovedListener.bind(
			[this](Entity *e)
			{
				if (e->has<GuiTooltipMarkerComponent>())
					this->tooltipRemoved(e);
			});
	}

	GuiImpl::~GuiImpl()
	{
		focusName = 0;
		hover = nullptr;
		hoverName = 0;
	}

	void GuiImpl::scaling()
	{
		pointsScale = zoom * retina;
		outputSize = Vec2(outputResolution) / pointsScale;
		outputMouse = Vec2(inputMouse) / pointsScale;
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

	void GuiManager::defocus()
	{
		focus(0);
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
		impl->clearTooltips();
	}

	bool GuiManager::coversCursor() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->testMouseCovered();
	}

	uint32 GuiManager::skinsCount() const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		return impl->skins.size();
	}

	GuiSkinConfig &GuiManager::skin(GuiSkinIndex index)
	{
		GuiImpl *impl = (GuiImpl *)this;
		CAGE_ASSERT(index.index < impl->skins.size());
		return impl->skins[index.index];
	}

	const GuiSkinConfig &GuiManager::skin(GuiSkinIndex index) const
	{
		const GuiImpl *impl = (const GuiImpl *)this;
		CAGE_ASSERT(index.index < impl->skins.size());
		return impl->skins[index.index];
	}

	EntityManager *GuiManager::entities()
	{
		GuiImpl *impl = (GuiImpl *)this;
		return +impl->entityMgr;
	}

	AssetsManager *GuiManager::assets()
	{
		GuiImpl *impl = (GuiImpl *)this;
		return +impl->assetMgr;
	}

	ProvisionalGraphics *GuiManager::graphics()
	{
		GuiImpl *impl = (GuiImpl *)this;
		return +impl->provisionalGraphics;
	}

	SoundsQueue *GuiManager::sounds()
	{
		GuiImpl *impl = (GuiImpl *)this;
		return +impl->soundsQueue;
	}

#define GCHL_GENERATE(T) void CAGE_JOIN(T, Create)(HierarchyItem * item);
	CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
	void textCreate(HierarchyItem *item);
	void imageCreate(HierarchyItem *item);
	void explicitSizeCreate(HierarchyItem *item);
	void scrollbarsCreate(HierarchyItem *item);
	void alignmentCreate(HierarchyItem *item);

	namespace
	{
		void sortChildren(HierarchyItem *item)
		{
#ifdef CAGE_DEBUG
			{ // randomize the order to explicitly reveal issues where the order is not correctly defined
				for (auto &it : item->children)
					std::swap(it, item->children[randomRange(std::size_t(), item->children.size())]);
			}
#endif // CAGE_DEBUG
			std::sort(item->children.begin(), item->children.end(), [](const Holder<HierarchyItem> &a, const Holder<HierarchyItem> &b) { return a->order < b->order; });
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
				const uint32 name = e->id();
				CAGE_ASSERT(name != 0 && name != m);
				map[name] = impl->memory->createHolder<HierarchyItem>(impl, e);
			}
			// attach all items to their parents
			for (auto e : impl->entityMgr->entities())
			{
				const uint32 name = e->id();
				Holder<HierarchyItem> item = map[name].share();
				if (e->has<GuiParentComponent>())
				{
					const GuiParentComponent &p = e->value<GuiParentComponent>();
					item->order = p.order;
					if (p.parent)
					{
						CAGE_ASSERT(p.parent != m && p.parent != name);
						CAGE_ASSERT(map.count(p.parent));
						map[p.parent]->children.push_back(std::move(item));
						continue;
					}
				}
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
			CAGE_ASSERT(entityWidgetsCount(item->ent) <= 1);
			CAGE_ASSERT(entityLayoutsCount(item->ent) <= 1);

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

			// widget
			{
#define GCHL_GENERATE(T) \
	if (GUI_HAS_COMPONENT(T, item->ent)) \
	{ \
		CAGE_JOIN(T, Create)(item); \
	}
				CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
			}

			// scrollbars
			if (GUI_HAS_COMPONENT(LayoutScrollbars, item->ent))
			{
				if (item->item)
					item = subsideItem(item);
				scrollbarsCreate(item);
				CAGE_ASSERT(item->item);
				item = subsideItem(item);
			}

			// alignment
			if (GUI_HAS_COMPONENT(LayoutAlignment, item->ent))
			{
				if (item->item)
					item = subsideItem(item);
				alignmentCreate(item);
				CAGE_ASSERT(item->item);
				item = subsideItem(item);
			}

			// layout
			if (entityLayoutsCount(item->ent))
			{
				if (item->item)
					item = subsideItem(item);
#define GCHL_GENERATE(T) \
	if (GUI_HAS_COMPONENT(T, item->ent)) \
	{ \
		CAGE_JOIN(T, Create)(item); \
	}
				CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
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
			if (valid(from.accent))
				to.accent = from.accent;
			if (from.skin.index != m)
				to.skin = from.skin;
			to.disabled = to.disabled || from.disabled;
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
				CAGE_ASSERT(w.skin.index < item->impl->skins.size());
				wi->skin = &item->impl->skins[w.skin.index];
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
					break;
				}
			}

			if (impl->hover)
			{
				CAGE_ASSERT(impl->hover->hierarchy);
				CAGE_ASSERT(impl->hover->hierarchy->ent);
				if (impl->hover->hierarchy->ent->id() != impl->hoverName)
				{
					if (!impl->hover->widgetState.disabled)
						impl->hover->playHoverSound();
				}
				impl->hoverName = impl->hover->hierarchy->ent->id();
			}
			else
				impl->hoverName = 0;
		}
	}

	void GuiImpl::prepareImplGeneration()
	{
		mouseEventReceivers.clear();
		root.clear();

		generateHierarchy(this);
		generateItems(+root);

		{ // propagate widget state
			GuiWidgetStateComponent ws;
			ws.accent = Vec4(0);
			ws.skin = GuiSkinDefault;
			propagateWidgetState(+root, ws);
		}

		{ // initialize
			root->initialize();
			// make sure that items added during initialization are initialized too
			std::size_t i = 0;
			while (i < root->children.size())
				callInitialize(+root->children[i++]);
		}

		{ // layouting
			root->findRequestedSize(outputSize[0]);
			FinalPosition u;
			u.renderPos = u.clipPos = Vec2();
			u.renderSize = u.clipSize = outputSize;
			root->findFinalPosition(u);
		}

		root->generateEventReceivers();
	}

	void GuiManager::prepare()
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->eventsEnabled = true;
		impl->prepareImplGeneration();
		impl->updateTooltips();
	}

	Holder<RenderQueue> GuiManager::finish()
	{
		GuiImpl *impl = (GuiImpl *)this;
		entitiesVisitor([&](Entity *e, const GuiUpdateComponent &u) { u.update(e); }, entities(), true);
		impl->prepareImplGeneration(); // entities may have been changed -> regenerate our cache
		findHover(impl);
		Holder<RenderQueue> q = impl->emit();
		impl->root.clear();
		return q;
	}

	void GuiManager::cleanUp()
	{
		GuiImpl *impl = (GuiImpl *)this;
		impl->root.clear();
		impl->assetOnDemand->clear();
	}

	Holder<GuiManager> newGuiManager(const GuiManagerCreateConfig &config)
	{
		return systemMemory().createImpl<GuiManager, GuiImpl>(config);
	}

	uint32 entityWidgetsCount(Entity *e)
	{
		uint32 result = 0;
#define GCHL_GENERATE(T) \
	if (GUI_HAS_COMPONENT(T, e)) \
		result++;
		CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	uint32 entityLayoutsCount(Entity *e)
	{
		uint32 result = 0;
#define GCHL_GENERATE(T) \
	if (GUI_HAS_COMPONENT(T, e)) \
		result++;
		CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
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

	namespace detail
	{
		void guiDestroyChildrenRecursively(Entity *root)
		{
			std::vector<Entity *> ents;
			const uint32 name = root->id();
			entitiesVisitor(
				[name, &ents](Entity *e, const GuiParentComponent &p)
				{
					if (p.parent == name)
						ents.push_back(e);
				},
				root->manager(), false);
			for (Entity *e : ents)
				guiDestroyEntityRecursively(e);
		}

		void guiDestroyEntityRecursively(Entity *root)
		{
			guiDestroyChildrenRecursively(root);
			root->destroy();
		}
	}
}
