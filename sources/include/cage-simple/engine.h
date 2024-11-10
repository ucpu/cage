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
		// assets processing thread index: 0
	};
	EngineControlThread &controlThread();

	struct EngineGraphicsDispatchThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> dispatch;
		EventDispatcher<bool()> swap;
		// assets processing thread index: 1
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
		GuiManagerCreateConfig *gui = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
		bool virtualReality = false;
	};

	void engineInitialize(const EngineCreateConfig &config);
	void engineRun();
	void engineStop();
	void engineFinalize();

	AssetsManager *engineAssets();
	EntityManager *engineEntities();
	Window *engineWindow();
	EventDispatcher<bool(const GenericInput &)> &engineEvents();
	VirtualReality *engineVirtualReality();
	EntityManager *engineGuiEntities();
	GuiManager *engineGuiManager();
	Speaker *engineSpeaker();
	VoicesMixer *engineMasterMixer();
	VoicesMixer *engineEffectsMixer();
	VoicesMixer *engineGuiMixer();
	ProvisionalGraphics *engineProvisonalGraphics();
	uint64 engineControlTime();

	struct EngineDynamicResolution
	{
		uint32 targetFps = 30;
		Real minimumScale = 0.6;
		bool enabled = false;
	};
	EngineDynamicResolution &engineDynamicResolution();

	namespace detail
	{
		void enginePurgeAssetsOnDemandCache();
	}
}

#endif // guard_engine_asg4ukio4up897sdr
