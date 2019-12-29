#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/engine.h>

namespace cage
{
	EntityComponent *TransformComponent::component;
	EntityComponent *TransformComponent::componentHistory;
	EntityComponent *RenderComponent::component;
	EntityComponent *SkeletalAnimationComponent::component;
	EntityComponent *TextureAnimationComponent::component;
	EntityComponent *LightComponent::component;
	EntityComponent *ShadowmapComponent::component;
	EntityComponent *RenderTextComponent::component;
	EntityComponent *CameraComponent::component;
	EntityComponent *SoundComponent::component;
	EntityComponent *ListenerComponent::component;

	TransformComponent::TransformComponent(const transform &t) : transform(t) {}
	RenderComponent::RenderComponent() : color(real::Nan()), opacity(real::Nan()), object(0), sceneMask(1) {}
	SkeletalAnimationComponent::SkeletalAnimationComponent() : startTime(0), name(0), speed(real::Nan()), offset(real::Nan()) {}
	TextureAnimationComponent::TextureAnimationComponent() : startTime(0), speed(real::Nan()), offset(real::Nan()) {}
	LightComponent::LightComponent() : color(1), attenuation(1, 0, 3), spotAngle(degs(40)), spotExponent(80), sceneMask(1), lightType(LightTypeEnum::Point) {}
	ShadowmapComponent::ShadowmapComponent() : worldSize(0), resolution(256), sceneMask(1) {}
	RenderTextComponent::RenderTextComponent() : color(1), assetName(0), textName(0), font(0), sceneMask(1) {}
	CameraSsao::CameraSsao() : worldRadius(0.5), strength(3), bias(0.03), power(1.3), samplesCount(16), blurPasses(3) {}
	CameraBloom::CameraBloom() : blurPasses(5), threshold(1) {}
	CameraEyeAdaptation::CameraEyeAdaptation() : key(0.5), strength(1.0), darkerSpeed(0.1), lighterSpeed(1) {} // darker should take at least 5 times longer
	CameraTonemap::CameraTonemap() : shoulderStrength(0.22), linearStrength(0.30), linearAngle(0.10), toeStrength(0.20), toeNumerator(0.01), toeDenominator(0.30), white(11.2) {}
	CameraEffects::CameraEffects() : gamma(2.2), effects(CameraEffectsFlags::None) {}
	CameraComponent::CameraUnion::CameraUnion() : perspectiveFov(degs(60)) {}
	CameraComponent::CameraComponent() : viewportSize(1, 1), target(nullptr), near(1), far(100), zeroParallaxDistance(10), eyeSeparation(0.3), cameraOrder(0), sceneMask(1), clear(CameraClearFlags::Depth | CameraClearFlags::Color), cameraType(CameraTypeEnum::Perspective) {}
	SoundComponent::SoundComponent() : input(nullptr), startTime(0), name(0), sceneMask(1) {}
	ListenerComponent::ListenerComponent() : output(nullptr), sceneMask(1), speedOfSound(343.3), dopplerEffect(false) {}

	EngineCreateConfig::EngineCreateConfig() :
		graphicsEmitMemory(1024 * 1024 * 32), graphicsDispatchMemory(1024 * 1024 * 32), soundEmitMemory(1024 * 1024 * 8),
		entities(nullptr), assets(nullptr), gui(nullptr), soundContext(nullptr), speaker(nullptr)
	{}

	EngineControlThread::EngineControlThread() : timePerTick(1000000 / 20)
	{}

	EngineControlThread &controlThread()
	{
		static EngineControlThread instance;
		return instance;
	}

	EngineGraphicsDispatchThread::EngineGraphicsDispatchThread()
	{}

	EngineGraphicsDispatchThread &graphicsDispatchThread()
	{
		static EngineGraphicsDispatchThread instance;
		return instance;
	}

	EngineGraphicsPrepareThread::EngineGraphicsPrepareThread() : stereoMode(StereoModeEnum::Mono)
	{}

	EngineGraphicsPrepareThread &graphicsPrepareThread()
	{
		static EngineGraphicsPrepareThread instance;
		return instance;
	}

	EngineSoundThread::EngineSoundThread() : timePerTick(1000000 / 40)
	{}

	EngineSoundThread &soundThread()
	{
		static EngineSoundThread instance;
		return instance;
	}
}
