#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/hashString.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include <cage-client/opengl.h>
#include "private.h"

namespace cage
{
	void initializeElements(elementLayoutStruct elements[(uint32)elementTypeEnum::TotalElements]);
	void initializeComponents(componentsStruct &components, entityManagerClass *manager);

	guiImpl::guiImpl(const guiCreateConfig &config) :
		arenaCache(config.memoryLimitCache), arenaPrepare(config.memoryLimitPrepare),
		openglContext(nullptr), assetManager(config.assetManager),
		cacheRoot(nullptr), prepareCacheFirst(nullptr), prepareCacheLast(nullptr),
		focusName(0), hoverName(0), mouseButtons((mouseButtonsFlags)0), prepareTime(0),
		skinTexture(nullptr), guiShader(nullptr), fontShader(nullptr), imageAnimatedShader(nullptr), imageStaticShader(nullptr), guiMesh(nullptr), fontMesh(nullptr), imageMesh(nullptr)
	{
		skinName = hashString("cage/texture/gui.psd");
		defaultFontName = hashString("cage/font/roboto.ttf?14");
		memoryCache = memoryArena(&arenaCache);
		memoryPrepare = memoryArena(&arenaPrepare);
		entityManager = newEntityManager(config.entitiesConfig ? *config.entitiesConfig : entityManagerCreateConfig());
		initializeElements(elements);
		initializeComponents(components, entityManager.get());
#define GCHL_GENERATE(T) listeners.T.bind<guiClass, &guiClass::T>(this);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, windowResize, mousePress, mouseRelease, mouseMove, mouseWheel, keyPress, keyRepeat, keyRelease, keyChar));
#undef GCHL_GENERATE
	};

	guiImpl::~guiImpl()
	{
		CAGE_ASSERT_RUNTIME(openglContext == nullptr);
		cacheRoot = nullptr;
		prepareCacheFirst = prepareCacheLast = nullptr;
		focusName = hoverName = 0;
		skinTexture = nullptr;
		guiShader = fontShader = imageAnimatedShader = imageStaticShader = nullptr;
		guiMesh = fontMesh = imageMesh = nullptr;
		memoryCache.flush();
		memoryPrepare.flush();
	}

	vec4 guiImpl::pixelsToNdc(vec2 pixelPosition, vec2 pixelSize)
	{
		vec2 invScreenSize = 2 / windowSize;
		vec2 resPos = (pixelPosition - windowSize * 0.5) * invScreenSize;
		vec2 resSiz = pixelSize * invScreenSize;
		resPos[1] = -resPos[1] - resSiz[1];
		return vec4(resPos, resPos + resSiz);
	}

	real guiImpl::evaluateUnit(real value, unitsModeEnum unit, real defaul)
	{
		switch (unit)
		{
		case unitsModeEnum::Automatic: return defaul;
		case unitsModeEnum::Pixels: return value;
		case unitsModeEnum::ScreenWidth: return value * windowSize[0];
		case unitsModeEnum::ScreenHeight: return value * windowSize[1];
		default: CAGE_THROW_CRITICAL(exception, "invalid unit");
		}
	}

	vec2 guiImpl::evaluateUnit(vec2 value, unitsModeEnum unit, real defaul)
	{
		for (uint32 i = 0; i < 2; i++)
			value[i] = evaluateUnit(value[i], unit, defaul);
		return value;
	}

	vec4 guiImpl::evaluateUnit(vec4 value, unitsModeEnum unit, real defaul)
	{
		for (uint32 i = 0; i < 4; i++)
			value[i] = evaluateUnit(value[i], unit, defaul);
		return value;
	}

	void guiImpl::contentToInner(vec2 &pos, vec2 &size, elementTypeEnum element)
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements, element, elementTypeEnum::TotalElements);
		elementLayoutStruct &layout = elements[(uint32)element];
		vec4 padding = evaluateUnit(layout.padding, layout.paddingUnits);
		vec2 padding2 = vec2(padding[0] + padding[2], padding[1] + padding[3]);
		pos -= vec2(padding);
		size += padding2;
	}

	void guiImpl::contentToOuter(vec2 &pos, vec2 &size, elementTypeEnum element)
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements, element, elementTypeEnum::TotalElements);
		elementLayoutStruct &layout = elements[(uint32)element];
		vec4 border = evaluateUnit(layout.border, layout.borderUnits);
		vec4 padding = evaluateUnit(layout.padding, layout.paddingUnits);
		vec2 border2 = vec2(border[0] + border[2], border[1] + border[3]);
		vec2 padding2 = vec2(padding[0] + padding[2], padding[1] + padding[3]);
		pos -= vec2(border) + vec2(padding);
		size += border2 + padding2;
	}

	void guiImpl::contentToEnvelope(vec2 &pos, vec2 &size, elementTypeEnum element)
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements, element, elementTypeEnum::TotalElements);
		elementLayoutStruct &layout = elements[(uint32)element];
		vec4 margin = evaluateUnit(layout.margin, layout.marginUnits);
		vec4 border = evaluateUnit(layout.border, layout.borderUnits);
		vec4 padding = evaluateUnit(layout.padding, layout.paddingUnits);
		vec2 margin2 = vec2(margin[0] + margin[2], margin[1] + margin[3]);
		vec2 border2 = vec2(border[0] + border[2], border[1] + border[3]);
		vec2 padding2 = vec2(padding[0] + padding[2], padding[1] + padding[3]);
		pos -= vec2(margin) + vec2(border) + vec2(padding);
		size += margin2 + border2 + padding2;
	}

	void guiImpl::envelopeToContent(vec2 &pos, vec2 &size, elementTypeEnum element)
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements, element, elementTypeEnum::TotalElements);
		elementLayoutStruct &layout = elements[(uint32)element];
		vec4 margin = evaluateUnit(layout.margin, layout.marginUnits);
		vec4 border = evaluateUnit(layout.border, layout.borderUnits);
		vec4 padding = evaluateUnit(layout.padding, layout.paddingUnits);
		vec2 margin2 = vec2(margin[0] + margin[2], margin[1] + margin[3]);
		vec2 border2 = vec2(border[0] + border[2], border[1] + border[3]);
		vec2 padding2 = vec2(padding[0] + padding[2], padding[1] + padding[3]);
		pos += vec2(margin) + vec2(border) + vec2(padding);
		size = max(size - (margin2 + border2 + padding2), vec2());
	}

	entityManagerClass *guiClass::entities()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->entityManager.get();
	}

	assetManagerClass *guiClass::assets()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->assetManager;
	}

	void guiClass::controlEmit()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->performCacheCreate();
	}

	void guiClass::graphicPrepare(uint64 time)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->performGraphicPrepare(time);
	}

	void guiClass::graphicDispatch()
	{
		guiImpl *impl = (guiImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->openglContext != nullptr);
		impl->performGraphicDispatch();
	}

	void guiClass::soundRender()
	{
		//guiImpl *impl = (guiImpl*)this;
		// todo someday
	}

	void guiClass::setCursorPosition(const pointStruct &point)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->mousePosition = vec2(point.x, point.y);
		impl->performCacheUpdate();
	}

	void guiClass::graphicInitialize(windowClass *openglContext)
	{
		guiImpl *impl = (guiImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->openglContext == nullptr);
		impl->openglContext = openglContext;
		impl->elementsGpuBuffer = newUniformBuffer(impl->openglContext);
		impl->elementsGpuBuffer->writeWhole(nullptr, sizeof(elementLayoutStruct::textureUvStruct) * (uint32)elementTypeEnum::TotalElements, GL_STATIC_DRAW);
	}

	void guiClass::graphicFinalize()
	{
		guiImpl *impl = (guiImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->openglContext != nullptr);
		impl->elementsGpuBuffer.clear();
		impl->openglContext = nullptr;
	}

	void guiClass::setFocus(uint32 control)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->focusName = control;
	}

	uint32 guiClass::getFocus() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->focusName;
	}

	guiCreateConfig::guiCreateConfig() : assetManager(nullptr), entitiesConfig(nullptr),
		memoryLimitCache(1024 * 1024 * 4), memoryLimitPrepare(1024 * 1024 * 4)
	{}

	holder<guiClass> newGui(const guiCreateConfig &config)
	{
		return detail::systemArena().createImpl<guiClass, guiImpl>(config);
	}
}