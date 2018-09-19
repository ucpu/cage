#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/swapBufferController.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	componentsStruct::componentsStruct(entityManagerClass *ents) : generalComponentsStruct(ents), layoutsComponentsStruct(ents), widgetsComponentsStruct(ents)
	{}

	guiImpl::graphicDataStruct::graphicDataStruct() :
		debugShader(nullptr), elementShader(nullptr), fontShader(nullptr), imageAnimatedShader(nullptr), imageStaticShader(nullptr),
		colorPickerShader{nullptr, nullptr, nullptr},
		debugMesh(nullptr), elementMesh(nullptr), fontMesh(nullptr), imageMesh(nullptr)
	{}

	guiImpl::emitDataStruct::emitDataStruct(const guiCreateConfig &config) :
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

	guiImpl::guiImpl(const guiCreateConfig &config) :
		entityManager(newEntityManager(config.entitiesConfig ? *config.entitiesConfig : entityManagerCreateConfig())), components(entityManager.get()),
		itemsArena(config.itemsArenaSize), itemsMemory(&itemsArena), root(nullptr),
		emitData{config, config, config}, emitControl(nullptr),
		openglContext(nullptr), assetManager(config.assetManager),
		focusName(0), hover(nullptr), eventsEnabled(false),
		zoom(1)
	{
		listeners.windowResize.bind<guiClass, &guiClass::windowResize>(this);
		listeners.mousePress.bind<guiClass, &guiClass::mousePress>(this);
		listeners.mouseDouble.bind<guiClass, &guiClass::mouseDouble>(this);
		listeners.mouseRelease.bind<guiClass, &guiClass::mouseRelease>(this);
		listeners.mouseMove.bind<guiClass, &guiClass::mouseMove>(this);
		listeners.mouseWheel.bind<guiClass, &guiClass::mouseWheel>(this);
		listeners.keyPress.bind<guiClass, &guiClass::keyPress>(this);
		listeners.keyRepeat.bind<guiClass, &guiClass::keyRepeat>(this);
		listeners.keyRelease.bind<guiClass, &guiClass::keyRelease>(this);
		listeners.keyChar.bind<guiClass, &guiClass::keyChar>(this);

		skins.resize(config.skinsCount);

		{
			swapBufferControllerCreateConfig cfg(3);
			cfg.repeatedReads = true;
			emitController = newSwapBufferController(cfg);
		}
	};

	guiImpl::~guiImpl()
	{
		CAGE_ASSERT_RUNTIME(openglContext == nullptr);
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
		CAGE_ASSERT_RUNTIME(size[0] >= 0 && size[1] >= 0);
		position *= pointsScale;
		size *= pointsScale;
		vec2 os = outputSize * pointsScale;
		vec2 invScreenSize = 2 / os;
		vec2 resPos = (position - os * 0.5) * invScreenSize;
		vec2 resSiz = size * invScreenSize;
		resPos[1] = -resPos[1] - resSiz[1];
		return vec4(resPos, resPos + resSiz);
	}

	real guiImpl::eval(real val, unitEnum unit, real defaul)
	{
		switch (unit)
		{
		case unitEnum::None: return defaul;
		case unitEnum::Points: return val;
		case unitEnum::Pixels: return val / pointsScale;
		case unitEnum::ScreenWidth: return val * outputSize[0];
		case unitEnum::ScreenHeight: return val * outputSize[1];
		case unitEnum::ScreenShorter: return val * min(outputSize[0], outputSize[1]);
		case unitEnum::ScreenLonger: return val * max(outputSize[0], outputSize[1]);
		default:
			CAGE_THROW_CRITICAL(exception, "invalid unit enum");
		}
	}
}
