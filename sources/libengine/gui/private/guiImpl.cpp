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
	guiComponents::guiComponents(entityManager *ents) : guiGeneralComponents(ents), guiLayoutsComponents(ents), guiWidgetsComponents(ents)
	{}

	guiImpl::graphicsDataStruct::graphicsDataStruct() :
		debugShader(nullptr), elementShader(nullptr), fontShader(nullptr), imageAnimatedShader(nullptr), imageStaticShader(nullptr),
		colorPickerShader{nullptr, nullptr, nullptr},
		debugMesh(nullptr), elementMesh(nullptr), fontMesh(nullptr), imageMesh(nullptr)
	{}

	guiImpl::emitDataStruct::emitDataStruct(const guiManagerCreateConfig &config) :
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

	guiImpl::guiImpl(const guiManagerCreateConfig &config) :
		entityMgr(newEntityManager(config.entitiesConfig ? *config.entitiesConfig : entityManagerCreateConfig())), components(entityMgr.get()),
		itemsArena(config.itemsArenaSize), itemsMemory(&itemsArena), root(nullptr),
		emitData{config, config, config}, emitControl(nullptr),
		assetMgr(config.assetMgr),
		focusName(0), focusParts(0), hover(nullptr), eventsEnabled(false),
		zoom(1)
	{
		listeners.windowResize.bind<guiManager, &guiManager::windowResize>(this);
		listeners.mousePress.bind<guiManager, &guiManager::mousePress>(this);
		listeners.mouseDouble.bind<guiManager, &guiManager::mouseDouble>(this);
		listeners.mouseRelease.bind<guiManager, &guiManager::mouseRelease>(this);
		listeners.mouseMove.bind<guiManager, &guiManager::mouseMove>(this);
		listeners.mouseWheel.bind<guiManager, &guiManager::mouseWheel>(this);
		listeners.keyPress.bind<guiManager, &guiManager::keyPress>(this);
		listeners.keyRepeat.bind<guiManager, &guiManager::keyRepeat>(this);
		listeners.keyRelease.bind<guiManager, &guiManager::keyRelease>(this);
		listeners.keyChar.bind<guiManager, &guiManager::keyChar>(this);

		skins.resize(config.skinsCount);

		{
			swapBufferGuardCreateConfig cfg(3);
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

	uint32 guiImpl::entityWidgetsCount(entity *e)
	{
		guiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	uint32 guiImpl::entityLayoutsCount(entity *e)
	{
		guiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}
}