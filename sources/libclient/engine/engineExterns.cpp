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
	componentClass *animatedTextureComponent::component;
	componentClass *lightComponent::component;
	componentClass *shadowmapComponent::component;
	componentClass *cameraComponent::component;
	componentClass *voiceComponent::component;
	componentClass *listenerComponent::component;

	transformComponent::transformComponent(const transform &t) : transform(t) {}
	renderComponent::renderComponent() : color(0, 0, 0), object(0), renderMask(1) {}
	animatedSkeletonComponent::animatedSkeletonComponent() : startTime(0), name(0), speed(1), offset(0) {}
	animatedTextureComponent::animatedTextureComponent() : startTime(0), speed(1), offset(0) {}
	lightComponent::lightComponent() : color(1, 1, 1), attenuation(1, 0, 0.01), spotAngle(degs(40)), spotExponent(40), lightType(lightTypeEnum::Point) {}
	shadowmapComponent::shadowmapComponent() : worldSize(0, 0, 0), resolution(256) {}
	cameraEffectsStruct::cameraEffectsStruct() : ssaoWorldRadius(0.5), effects(cameraEffectsFlags::None) {}
	cameraComponent::cameraComponent() : viewportSize(1, 1), target(nullptr), perspectiveFov(degs(60)), near(1), far(100), zeroParallaxDistance(10), eyeSeparation(0.3), cameraOrder(0), renderMask(1), clear(cameraClearFlags::Depth | cameraClearFlags::Color), cameraType(cameraTypeEnum::Perspective) {}
	voiceComponent::voiceComponent() : input(nullptr), startTime(0), name(0), renderMask(1) {}
	listenerComponent::listenerComponent() : output(nullptr), renderMask(1), speedOfSound(343.3), dopplerEffect(false) {}

	engineCreateConfig engineConfig;

	engineCreateConfig::engineCreateConfig() :
		graphicsEmitMemory(1024 * 1024 * 32), graphicsDispatchMemory(1024 * 1024 * 32), soundEmitMemory(1024 * 1024 * 8),
		entities(nullptr), assets(nullptr), gui(nullptr), soundContext(nullptr), speaker(nullptr)
	{}

	controlThreadClass::controlThreadClass() : timePerTick(1000000 / 20)
	{}

	controlThreadClass &controlThread()
	{
		static controlThreadClass instance;
		return instance;
	}

	graphicsDispatchThreadClass::graphicsDispatchThreadClass()
	{}

	graphicsDispatchThreadClass &graphicsDispatchThread()
	{
		static graphicsDispatchThreadClass instance;
		return instance;
	}

	graphicsPrepareThreadClass::graphicsPrepareThreadClass() : stereoMode(stereoModeEnum::Mono)
	{}

	graphicsPrepareThreadClass &graphicsPrepareThread()
	{
		static graphicsPrepareThreadClass instance;
		return instance;
	}

	soundThreadClass::soundThreadClass() : timePerTick(1000000 / 40)
	{}

	soundThreadClass &soundThread()
	{
		static soundThreadClass instance;
		return instance;
	}
}
