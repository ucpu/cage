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
	struct AssetManagerCreateConfig;
	struct GuiManagerCreateConfig;
	struct SpeakerCreateConfig;

	struct EngineControlThread
	{
		EventDispatcher<bool()> initialize; // called once from engineStart()
		EventDispatcher<bool()> finalize; // called once from engineStart()
		EventDispatcher<bool()> update; // periodically called from engineStart()
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
		static constexpr uint32 threadIndex = 1;
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
		uint64 updatePeriod() const;
		void updatePeriod(uint64 p);
	};
	EngineSoundThread &soundThread();

	struct EngineCreateConfig
	{
		AssetManagerCreateConfig *assets = nullptr;
		GuiManagerCreateConfig *gui = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
	};

	void engineInitialize(const EngineCreateConfig &config);
	void engineStart();
	void engineStop();
	void engineFinalize();

	AssetManager *engineAssets();
	EntityManager *engineEntities();
	Window *engineWindow();
	EntityManager *engineGuiEntities();
	GuiManager *engineGuiManager();
	Speaker *engineSpeaker();
	VoicesMixer *engineMasterMixer();
	VoicesMixer *engineEffectsMixer();
	VoicesMixer *engineGuiMixer();
	uint64 engineControlTime();
}

#endif // guard_engine_asg4ukio4up897sdr
