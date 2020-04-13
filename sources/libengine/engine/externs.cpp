#include <cage-core/camera.h>

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
	EntityComponent *TextComponent::component;
	EntityComponent *CameraComponent::component;
	EntityComponent *SoundComponent::component;
	EntityComponent *ListenerComponent::component;

	EngineControlThread &controlThread()
	{
		static EngineControlThread instance;
		return instance;
	}

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

	EngineSoundThread &soundThread()
	{
		static EngineSoundThread instance;
		return instance;
	}
}
