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
	componentClass *renderTextComponent::component;
	componentClass *cameraComponent::component;
	componentClass *voiceComponent::component;
	componentClass *listenerComponent::component;

	transformComponent::transformComponent(const transform &t) : transform(t) {}
	renderComponent::renderComponent() : color(0, 0, 0), opacity(1), object(0), renderMask(1) {}
	animatedSkeletonComponent::animatedSkeletonComponent() : startTime(0), name(0), speed(1), offset(0) {}
	animatedTextureComponent::animatedTextureComponent() : startTime(0), speed(1), offset(0) {}
	lightComponent::lightComponent() : color(1, 1, 1), attenuation(1, 0, 3), spotAngle(degs(40)), spotExponent(80), lightType(lightTypeEnum::Point) {}
	shadowmapComponent::shadowmapComponent() : worldSize(0, 0, 0), resolution(256) {}
	renderTextComponent::renderTextComponent() : color(1, 1, 1), assetName(0), textName(0), font(0), renderMask(1) {}
	cameraSsaoStruct::cameraSsaoStruct() : worldRadius(0.5), blurRadius(1), strength(1), bias(0), power(0.5), samplesCount(20) {}
	cameraEyeAdaptationStruct::cameraEyeAdaptationStruct() : key(0.5), strength(1.0), darkerSpeed(0.1), lighterSpeed(1) {} // darker should take at least 5 times longer
	cameraTonemapStruct::cameraTonemapStruct() : shoulderStrength(0.22), linearStrength(0.30), linearAngle(0.10), toeStrength(0.20), toeNumerator(0.01), toeDenominator(0.30), white(11.2) {}
	cameraEffectsStruct::cameraEffectsStruct() : gamma(2.2), effects(cameraEffectsFlags::None) {}
	cameraComponent::CameraUnion::CameraUnion() : perspectiveFov(degs(60)) {}
	cameraComponent::cameraComponent() : viewportSize(1, 1), target(nullptr), near(1), far(100), zeroParallaxDistance(10), eyeSeparation(0.3), cameraOrder(0), renderMask(1), clear(cameraClearFlags::Depth | cameraClearFlags::Color), cameraType(cameraTypeEnum::Perspective) {}
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
