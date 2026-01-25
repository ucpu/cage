#ifndef guard_engine_asg4ukio4up897sdr
#define guard_engine_asg4ukio4up897sdr

#include <cage-core/events.h>
#include <cage-engine/core.h>

namespace cage
{
	class AssetsOnDemand;
	class EntityManager;
	class GuiManager;
	class Scheduler;
	class Speaker;
	class VirtualReality;
	class VoicesMixer;
	class SoundsQueue;
	class Window;
	class GraphicsDevice;
	class Image;
	struct AssetsManagerCreateConfig;
	struct GuiManagerCreateConfig;
	struct SpeakerCreateConfig;
	struct WindowCreateConfig;

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

	struct EngineGraphicsThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> graphics;
		bool disableTimePassage = false;
	};
	EngineGraphicsThread &graphicsThread();

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
		AssetsManagerCreateConfig *assets = nullptr;
		WindowCreateConfig *window = nullptr;
		GuiManagerCreateConfig *gui = nullptr;
		SpeakerCreateConfig *speaker = nullptr;
		bool virtualReality = false;
		bool vsync = true;
	};

	void engineInitialize(const EngineCreateConfig &config);
	void engineRun();
	void engineStop();
	void engineFinalize();

	AssetsManager *engineAssets();
	AssetsOnDemand *engineAssetsOnDemand();
	EntityManager *engineEntities();
	GraphicsDevice *engineGraphicsDevice();
	Window *engineWindow();
	EventDispatcher<bool(const GenericInput &)> &engineEvents();
	VirtualReality *engineVirtualReality();
	EntityManager *engineGuiEntities();
	GuiManager *engineGuiManager();
	Speaker *engineSpeaker();
	VoicesMixer *engineMasterMixer();
	VoicesMixer *engineSceneMixer();
	SoundsQueue *engineGuiMixer();
	uint64 engineControlTime();

	struct EngineDynamicResolution
	{
		uint32 targetFps = 30;
		Real minimumScale = 0.6;
		bool enabled = false;
	};
	EngineDynamicResolution &engineDynamicResolution();

	Holder<Image> engineScreenshot();
}

#endif // guard_engine_asg4ukio4up897sdr
