#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/swapBufferGuard.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	GuiComponents::GuiComponents(EntityManager *ents) : GuiGeneralComponents(ents), GuiLayoutsComponents(ents), GuiWidgetsComponents(ents)
	{}

	guiImpl::graphicsDataStruct::graphicsDataStruct() :
		debugShader(nullptr), elementShader(nullptr), fontShader(nullptr), imageAnimatedShader(nullptr), imageStaticShader(nullptr),
		colorPickerShader{nullptr, nullptr, nullptr},
		debugMesh(nullptr), elementMesh(nullptr), fontMesh(nullptr), imageMesh(nullptr)
	{}

	guiImpl::emitDataStruct::emitDataStruct(const GuiCreateConfig &config) :
		arena(config.emitArenaSize), memory(&arena), first(nullptr), last(nullptr)
	{}

	guiImpl::emitDataStruct::~emitDataStruct()
	{
		flush();
	}

	void guiImpl::emitDataStruct::flush()
	{
		first = last = nullptr;
		memory.flush();
	}

	guiImpl::guiImpl(const GuiCreateConfig &config) :
		entityMgr(newEntityManager(config.entitiesConfig ? *config.entitiesConfig : EntityManagerCreateConfig())), components(entityMgr.get()),
		itemsArena(config.itemsArenaSize), itemsMemory(&itemsArena), root(nullptr),
		emitData{config, config, config}, emitControl(nullptr),
		assetMgr(config.assetMgr),
		focusName(0), focusParts(0), hover(nullptr), eventsEnabled(false),
		zoom(1)
	{
		listeners.windowResize.bind<Gui, &Gui::windowResize>(this);
		listeners.mousePress.bind<Gui, &Gui::mousePress>(this);
		listeners.mouseDouble.bind<Gui, &Gui::mouseDouble>(this);
		listeners.mouseRelease.bind<Gui, &Gui::mouseRelease>(this);
		listeners.mouseMove.bind<Gui, &Gui::mouseMove>(this);
		listeners.mouseWheel.bind<Gui, &Gui::mouseWheel>(this);
		listeners.keyPress.bind<Gui, &Gui::keyPress>(this);
		listeners.keyRepeat.bind<Gui, &Gui::keyRepeat>(this);
		listeners.keyRelease.bind<Gui, &Gui::keyRelease>(this);
		listeners.keyChar.bind<Gui, &Gui::keyChar>(this);

		skins.resize(config.skinsCount);

		{
			SwapBufferGuardCreateConfig cfg(3);
			cfg.repeatedReads = true;
			emitController = newSwapBufferGuard(cfg);
		}
	}

	guiImpl::~guiImpl()
	{
		focusName = 0;
		hover = nullptr;
		itemsMemory.flush();
	}

	void guiImpl::scaling()
	{
		pointsScale = zoom * retina;
		outputSize = vec2(outputResolution.x, outputResolution.y) / pointsScale;
		outputMouse = vec2(inputMouse.x, inputMouse.y) / pointsScale;
	}

	vec4 guiImpl::pointsToNdc(vec2 position, vec2 size)
	{
		CAGE_ASSERT(size[0] >= 0 && size[1] >= 0);
		position *= pointsScale;
		size *= pointsScale;
		vec2 os = outputSize * pointsScale;
		vec2 invScreenSize = 2 / os;
		vec2 resPos = (position - os * 0.5) * invScreenSize;
		vec2 resSiz = size * invScreenSize;
		resPos[1] = -resPos[1] - resSiz[1];
		return vec4(resPos, resPos + resSiz);
	}

	uint32 guiImpl::entityWidgetsCount(Entity *e)
	{
		guiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	uint32 guiImpl::entityLayoutsCount(Entity *e)
	{
		guiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}
}
