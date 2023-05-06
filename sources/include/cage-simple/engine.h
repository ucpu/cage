#ifndef guard_engine_asg4ukio4up897sdr
#define guard_engine_asg4ukio4up897sdr

#include <cage-core/events.h>
#include <cage-engine/core.h>

namespace cage
{
	class EntityManager;
	class Scheduler;
	class Window;
	class GuiManager;
	class Speaker;
	class VoicesMixer;
	class VirtualReality;
	class ProvisionalGraphics;
	struct WindowCreateConfig;
	struct AssetManagerCreateConfig;
	struct GuiManagerCreateConfig;
	struct SpeakerCreateConfig;
	struct VirtualRealityCreateConfig;

	struct EngineControlThread
	{
		EventDispatcher<bool()> initialize; // called once from engineRun()
		EventDispatcher<bool()> finalize; // called once from engineRun()
		EventDispatcher<bool()> update; // periodically called from engineRun()
		EventDispatcher<bool()> unload; // periodically called from engineFinalize()
		Scheduler *scheduler();
		uint64 updatePeriod() const;
		void updatePeriod(uint64 p);
		uint64 inputPeriod() const;
		void inputPeriod(uint64 p);
	};
	EngineControlThread &controlThread();

	struct EngineGraphicsDispatchThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> dispatch;
		EventDispatcher<bool()> swap;
		static constexpr uint32 threadIndex = 1; // used for loading graphical assets
	};
	EngineGraphicsDispatchThread &graphicsDispatchThread();

	struct EngineGraphicsPrepareThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> prepare;
	};
	EngineGraphicsPrepareThread &graphicsPrepareThread();

	struct EngineSoundThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> sound;
		Scheduler *scheduler();
	};
	EngineSoundThread &soundThread();

	struct EngineCreateConfig
	{
		AssetManagerCreateConfig *assets = nullptr;
		WindowCreateConfig *window = nullptr;
		VirtualRealityCreateConfig *virtualReality = nullptr;
		GuiManagerCreateConfig *gui = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
	};

	void engineInitialize(const EngineCreateConfig &config);
	void engineRun();
	void engineStop();
	void engineFinalize();

	AssetManager *engineAssets();
	EntityManager *engineEntities();
	Window *engineWindow();
	VirtualReality *engineVirtualReality();
	EntityManager *engineGuiEntities();
	GuiManager *engineGuiManager();
	Speaker *engineSpeaker();
	VoicesMixer *engineMasterMixer();
	VoicesMixer *engineEffectsMixer();
	VoicesMixer *engineGuiMixer();
	ProvisionalGraphics *engineProvisonalGraphics();
	uint64 engineControlTime();

	namespace detail
	{
		void purgeAssetsOnDemandCache();
	}
}

#endif // guard_engine_asg4ukio4up897sdr
