#include <cage-core/core.h>
#include <cage-core/math.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/engine.h>

namespace cage
{
	componentClass *transformComponent::component;
	componentClass *transformComponent::componentHistory;
	componentClass *renderComponent::component;
	componentClass *animatedSkeletonComponent::component;
	componentClass *configuredSkeletonComponent::component;
	componentClass *configuredSkeletonComponent::componentHistory;
	componentClass *animatedTextureComponent::component;
	componentClass *lightComponent::component;
	componentClass *shadowmapComponent::component;
	componentClass *cameraComponent::component;
	componentClass *voiceComponent::component;
	componentClass *listenerComponent::component;

	transformComponent::transformComponent(const transform &t) : transform(t) {}
	renderComponent::renderComponent() : color(0, 0, 0), object(0), renderMask(1) {}
	animatedSkeletonComponent::animatedSkeletonComponent() : startTime(0), name(0), speed(1), offset(0) {}
	configuredSkeletonComponent::configuredSkeletonComponent() {}
	animatedTextureComponent::animatedTextureComponent() : startTime(0), speed(1), offset(0) {}
	lightComponent::lightComponent() : color(1, 1, 1), attenuation(1, 0, 0.01), spotAngle(degs(60)), spotExponent(1.5), lightType(lightTypeEnum::Point) {}
	shadowmapComponent::shadowmapComponent() : worldRadius(0, 0, 0), resolution(256) {}
	cameraComponent::cameraComponent() : viewportSize(1, 1), target(nullptr), perspectiveFov(degs(60)), near(1), far(100), zeroParallaxDistance(10), eyeSeparation(0.3), cameraOrder(0), renderMask(1), clear(cameraClearFlags::Depth | cameraClearFlags::Color), cameraType(cameraTypeEnum::Perspective) {}
	voiceComponent::voiceComponent() : input(nullptr), startTime(0), name(0), renderMask(1) {}
	listenerComponent::listenerComponent() : output(nullptr), renderMask(1), speedOfSound(343.3), dopplerEffect(false) {}

	engineCreateConfig engineConfig;

	engineCreateConfig::engineCreateConfig() :
		graphicEmitMemory(1024 * 1024 * 32), graphicDispatchMemory(1024 * 1024 * 32), soundEmitMemory(1024 * 1024 * 8),
		entities(nullptr), assets(nullptr), gui(nullptr), soundContext(nullptr), speaker(nullptr)
	{}

	namespace controlThread
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> update;
		eventDispatcher<bool()> assets;
		uint64 timePerTick = 1000000 / 20;
	}

	namespace graphicsDispatchThread
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> render;
		eventDispatcher<bool()> swap;
	}

	namespace graphicsPrepareThread
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> prepare;
		stereoModeEnum stereoMode = stereoModeEnum::Mono;
	}

	namespace soundThread
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> sound;
		uint64 timePerTick = 1000000 / 40;
	}
}
