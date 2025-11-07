#include <atomic>
#include <exception>

#include "engine.h"

#include <cage-core/assetContext.h>
#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/assetsSchemes.h>
#include <cage-core/camera.h>
#include <cage-core/collider.h> // for sizeof in defineScheme
#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/entities.h>
#include <cage-core/files.h> // getExecutableName
#include <cage-core/hashString.h>
#include <cage-core/logger.h>
#include <cage-core/macros.h>
#include <cage-core/profiling.h>
#include <cage-core/scheduler.h>
#include <cage-core/skeletalAnimation.h> // for sizeof in defineScheme
#include <cage-core/string.h>
#include <cage-core/texts.h> // for sizeof in defineScheme
#include <cage-core/threadPool.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/font.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/keybinds.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneCustomDraw.h>
#include <cage-engine/scenePicking.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/shader.h>
#include <cage-engine/sound.h>
#include <cage-engine/soundsQueue.h>
#include <cage-engine/soundsVoices.h>
#include <cage-engine/speaker.h>
#include <cage-engine/texture.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/window.h>
#include <cage-simple/engine.h>
#include <cage-simple/statisticsGui.h>

namespace cage
{
	namespace detail
	{
		CAGE_API_IMPORT bool assetsListenDefault();
	}

	namespace
	{
		const ConfigBool confAutoAssetListen("cage/assets/listen", false);

		template<uint32 N>
		struct ScopedTimer : private Immovable
		{
			VariableSmoothingBuffer<uint64, N> &vsb;
			uint64 st = applicationTime();

			explicit ScopedTimer(VariableSmoothingBuffer<uint64, N> &vsb) : vsb(vsb) {}

			~ScopedTimer()
			{
				const uint64 et = applicationTime();
				vsb.add(et - st);
			}
		};

		struct EngineData
		{
			std::atomic<uint32> engineStarted = 0;
			std::atomic<bool> stopping = false;
			uint64 controlTime = 0;

			VariableSmoothingBuffer<Real, 60> profilingBufferDynamicResolution;
			VariableSmoothingBuffer<uint64, 60> profilingBufferGpuTime;
			VariableSmoothingBuffer<uint64, 60> profilingBufferFrameTime;
			VariableSmoothingBuffer<uint64, 60> profilingBufferDrawCalls;
			VariableSmoothingBuffer<uint64, 60> profilingBufferDrawPrimitives;
			VariableSmoothingBuffer<uint64, 30> profilingBufferEntities;

			Holder<EnginePrivateGraphics> privateGraphics;
			Holder<EnginePrivateSound> privateSound;

			Holder<AssetsManager> assets;
			Holder<AssetsOnDemand> onDemand;
			Holder<GraphicsDevice> device;
			Holder<Window> window;
			Holder<VirtualReality> virtualReality;
			Holder<Speaker> speaker;
			Holder<VoicesMixer> masterBus;
			Holder<VoicesMixer> sceneMixer;
			Holder<SoundsQueue> guiMixer;
			Holder<Voice> sceneVoice;
			Holder<Voice> guiVoice;
			Holder<GuiManager> gui;
			ExclusiveHolder<GuiRender> guiBundle;
			Holder<EntityManager> entities;

			EventListener<bool(const GenericInput &)> windowGuiEventsListener;

			Holder<Scheduler> controlScheduler;
			Holder<Schedule> controlUpdateSchedule;
			Holder<Schedule> controlInputSchedule;
			Holder<Scheduler> soundScheduler;
			Holder<Schedule> soundUpdateSchedule;

			Holder<Barrier> threadsStateBarier;
			Holder<Thread> graphicsThreadHolder;
			Holder<Thread> soundThreadHolder;

			//////////////////////////////////////
			// GRAPHICS
			//////////////////////////////////////

			void graphicsInitializeStage() { privateGraphics->initialize(); }

			void graphicsUpdate()
			{
				const ProfilingScope profiling("graphics", ProfilingFrameTag());
				ScopedTimer timing(profilingBufferFrameTime);
				{
					const ProfilingScope profiling("graphics callback");
					graphicsThread().graphics.dispatch();
				}
				{
					const ProfilingScope profiling("graphics dispatch");
					privateGraphics->dispatch(applicationTime(), guiBundle.get());
				}
				profilingBufferGpuTime.add(privateGraphics->gpuTime);
				profilingBufferDrawCalls.add(privateGraphics->drawCalls);
				profilingBufferDrawPrimitives.add(privateGraphics->drawPrimitives);
				profilingBufferDynamicResolution.add(privateGraphics->dynamicResolution);
			}

			void graphicsGameloopStage()
			{
				while (!stopping)
					graphicsUpdate();
			}

			void graphicsStopStage() {}

			void graphicsFinalizeStage() { privateGraphics->finalize(); }

			//////////////////////////////////////
			// SOUND
			//////////////////////////////////////

			void soundInitializeStage()
			{
				privateSound->initialize();
				speaker->start();
			}

			void soundUpdate()
			{
				ProfilingScope profiling("sound", ProfilingFrameTag());
				if (stopping)
				{
					soundScheduler->stop();
					return;
				}
				soundUpdateSchedule->period(controlUpdateSchedule->period()); // sound thread goes at same speed as control thread
				{
					ProfilingScope profiling("sound callback");
					soundThread().sound.dispatch();
				}
				{
					ProfilingScope profiling("sound dispatch");
					privateSound->dispatch(soundUpdateSchedule->time());
				}
			}

			void soundGameloopStage() { soundScheduler->run(); }

			void soundStopStage() { speaker->stop(); }

			void soundFinalizeStage() { privateSound->finalize(); }

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void updateComponents()
			{
				EntityComponent *curr = entities->component<TransformComponent>();
				EntityComponent *prev = entities->componentsByType(detail::typeIndex<TransformComponent>())[1];
				for (Entity *e : engineEntities()->component<TransformComponent>()->entities())
					e->value<TransformComponent>(prev) = e->value<TransformComponent>(curr);
				for (Entity *e : engineEntities()->entities())
				{
					if (!e->has<SpawnTimeComponent>())
						e->value<SpawnTimeComponent>().spawnTime = controlTime;
				}
			}

			void controlInputs()
			{
				ProfilingScope profiling("inputs");
				{
					ProfilingScope profiling("gui prepare");
					gui->outputResolution(window->resolution());
					gui->outputRetina(window->contentScaling());
					gui->prepare();
				}
				{
					ProfilingScope profiling("window events");
					window->processEvents();
				}
				{
					ProfilingScope profiling("gui finish");
					guiBundle.assign(gui->finish());
				}
			}

			void controlUpdate()
			{
				ProfilingScope profiling("control", ProfilingFrameTag());
				if (stopping)
				{
					controlScheduler->stop();
					return;
				}
				controlTime = controlUpdateSchedule->time();
				{
					ProfilingScope profiling("update components");
					updateComponents();
				}
				if (virtualReality)
				{
					ProfilingScope profiling("virtual reality events");
					virtualReality->processEvents();
					virtualRealitySceneUpdate(+entities);
				}
				{
					ProfilingScope profiling("control callback");
					controlThread().update.dispatch();
				}
				{
					ProfilingScope profiling("keybinds engine tick");
					engineEvents().dispatch(input::EngineTick());
				}
				{
					ProfilingScope profiling("sound emit");
					privateSound->emit();
				}
				{
					ProfilingScope profiling("graphics emit");
					privateGraphics->emit(controlTime);
				}
				profilingBufferEntities.add(entities->count());
				{
					ProfilingScope profiling("control assets");
					const uint64 start = applicationTime();
					onDemand->process();
					while (assets->processCustomThread(0))
					{
						if (applicationTime() > start + 5'000)
							break;
					}
				}
			}

			void controlGameloopStage() { controlScheduler->run(); }

			//////////////////////////////////////
			// ENTRY methods
			//////////////////////////////////////

#define GCHL_GENERATE_CATCH(NAME, STAGE) \
	catch (...) \
	{ \
		CAGE_LOG(SeverityEnum::Error, "engine", "unhandled exception in " CAGE_STRINGIZE(STAGE) " in " CAGE_STRINGIZE(NAME)); \
		detail::logCurrentCaughtException(); \
		engineStop(); \
	}
#define GCHL_GENERATE_ENTRY(NAME) \
	void CAGE_JOIN(NAME, Entry)() \
	{ \
		try \
		{ \
			CAGE_JOIN(NAME, InitializeStage)(); \
		} \
		GCHL_GENERATE_CATCH(NAME, initialization - engine); \
		{ \
			ScopeLock l(threadsStateBarier); \
		} \
		{ \
			ScopeLock l(threadsStateBarier); \
		} \
		try \
		{ \
			CAGE_JOIN(NAME, Thread)().initialize.dispatch(); \
		} \
		GCHL_GENERATE_CATCH(NAME, initialization - application); \
		{ \
			ScopeLock l(threadsStateBarier); \
		} \
		try \
		{ \
			CAGE_JOIN(NAME, GameloopStage)(); \
		} \
		GCHL_GENERATE_CATCH(NAME, gameloop); \
		CAGE_JOIN(NAME, StopStage)(); \
		{ \
			ScopeLock l(threadsStateBarier); \
		} \
		try \
		{ \
			CAGE_JOIN(NAME, Thread)().finalize.dispatch(); \
		} \
		GCHL_GENERATE_CATCH(NAME, finalization - application); \
		try \
		{ \
			CAGE_JOIN(NAME, FinalizeStage)(); \
		} \
		GCHL_GENERATE_CATCH(NAME, finalization - engine); \
	}
			CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphics, sound));
#undef GCHL_GENERATE_ENTRY

			//////////////////////////////////////
			// INITIALIZATION & finalization
			//////////////////////////////////////

			void initialize(const EngineCreateConfig &config)
			{
				CAGE_ASSERT(engineStarted == 0);
				engineStarted = 1;

				CAGE_LOG(SeverityEnum::Info, "engine", "initializing engine");

				{ // create private
					privateGraphics = newEnginePrivateGraphics(config);
					privateSound = newEnginePrivateSound(config);
				}

				{ // create schedules
					controlScheduler = newScheduler({});
					soundScheduler = newScheduler({});
					{
						ScheduleCreateConfig cfg;
						cfg.name = "control schedule";
						cfg.action = Delegate<void()>().bind<EngineData, &EngineData::controlUpdate>(this);
						cfg.period = 1'000'000 / 20;
						cfg.type = ScheduleTypeEnum::SteadyPeriodic;
						controlUpdateSchedule = controlScheduler->newSchedule(cfg);
					}
					{
						ScheduleCreateConfig cfg;
						cfg.name = "inputs schedule";
						cfg.action = Delegate<void()>().bind<EngineData, &EngineData::controlInputs>(this);
						cfg.period = 1'000'000 / 60;
						cfg.type = ScheduleTypeEnum::FreePeriodic;
						controlInputSchedule = controlScheduler->newSchedule(cfg);
					}
					{
						ScheduleCreateConfig c;
						c.name = "sound schedule";
						c.action = Delegate<void()>().bind<EngineData, &EngineData::soundUpdate>(this);
						c.period = 1'000'000 / 20;
						c.type = ScheduleTypeEnum::SteadyPeriodic;
						soundUpdateSchedule = soundScheduler->newSchedule(c);
					}
				}

				{ // create entities
					entities = newEntityManager();
					EntityManager *entityMgr = +entities;
					entityMgr->defineComponent(TransformComponent());
					entityMgr->defineComponent(TransformComponent());
					entityMgr->defineComponent(ColorComponent());
					entityMgr->defineComponent(SceneComponent());
					entityMgr->defineComponent(ShaderDataComponent());
					entityMgr->defineComponent(SpawnTimeComponent());
					entityMgr->defineComponent(AnimationSpeedComponent());
					entityMgr->defineComponent(SkeletalAnimationComponent());
					entityMgr->defineComponent(ModelComponent());
					entityMgr->defineComponent(IconComponent());
					entityMgr->defineComponent(TextComponent());
					entityMgr->defineComponent(TextValueComponent());
					entityMgr->defineComponent(LightComponent());
					entityMgr->defineComponent(ShadowmapComponent());
					entityMgr->defineComponent(CameraComponent());
					entityMgr->defineComponent(ScreenSpaceEffectsComponent());
					entityMgr->defineComponent(SoundComponent());
					entityMgr->defineComponent(ListenerComponent());
					entityMgr->defineComponent(PickableComponent());
					entityMgr->defineComponent(CustomDrawComponent());
					entityMgr->defineComponent(VrOriginComponent());
					entityMgr->defineComponent(VrCameraComponent());
					entityMgr->defineComponent(VrControllerComponent());
				}

				{ // create assets manager
					AssetManagerCreateConfig cfg;
					if (config.assets)
						cfg = *config.assets;
					cfg.schemesMaxCount = max(cfg.schemesMaxCount, 30u);
					cfg.customProcessingThreads = max(cfg.customProcessingThreads, 5u);
					assets = newAssetsManager(cfg);
					onDemand = newAssetsOnDemand(+assets);
				}

				{ // create window
					WindowCreateConfig cfg;
					if (config.window)
						cfg = *config.window;
					window = newWindow(cfg);
					window->events.merge(engineEvents());
				}

				{ // create device
					GraphicsDeviceCreateConfig cfg;
					cfg.compatibility = +window;
					cfg.vsync = config.vsync;
					if (config.virtualReality)
						cfg.vsync = false; // explicitly disable vsync for the window when virtual reality controls frame rate
					device = newGraphicsDevice(cfg);
				}

				{ // create virtual reality
					if (config.virtualReality)
					{
						virtualReality = newVirtualReality();
						virtualReality->events.merge(engineEvents());
						controlUpdateSchedule->period(virtualReality->targetFrameTiming());
					}
				}

				{ // create sound speaker
					masterBus = newVoicesMixer();
					SpeakerCreateConfig cfg;
					if (config.speaker)
						cfg = *config.speaker;
					if (cfg.sampleRate == 0)
						cfg.sampleRate = 48'000; // minimize sample rate conversions
					cfg.callback = Delegate<void(const SoundCallbackData &)>().bind<VoicesMixer, &VoicesMixer::process>(+masterBus);
					speaker = newSpeaker(cfg);
				}

				{ // create sound mixers
					sceneMixer = newVoicesMixer();
					sceneVoice = masterBus->newVoice();
					sceneVoice->callback.bind<VoicesMixer, &VoicesMixer::process>(+sceneMixer);
					guiMixer = newSoundsQueue(+assets);
					guiVoice = masterBus->newVoice();
					guiVoice->callback.bind<SoundsQueue, &SoundsQueue::process>(+guiMixer);
				}

				{ // create gui
					GuiManagerCreateConfig cfg;
					if (config.gui)
						cfg = *config.gui;
					cfg.assetManager = +assets;
					cfg.graphicsDevice = +device;
					cfg.soundsQueue = +guiMixer;
					gui = newGuiManager(cfg);
					gui->widgetEvent.merge(engineEvents());
					windowGuiEventsListener.attach(window->events, -1'000);
					windowGuiEventsListener.bind([gui = +gui](const GenericInput &in) { return gui->handleInput(in); });
				}

				{ // initialize asset schemes
					// core assets
					assets->defineScheme<AssetSchemeIndexPack, AssetPack>(genAssetSchemePack());
					assets->defineScheme<AssetSchemeIndexRaw, PointerRange<const char>>(genAssetSchemeRaw());
					assets->defineScheme<AssetSchemeIndexTexts, Texts>(genAssetSchemeTexts());
					assets->defineScheme<AssetSchemeIndexCollider, Collider>(genAssetSchemeCollider());
					assets->defineScheme<AssetSchemeIndexSkeletonRig, SkeletonRig>(genAssetSchemeSkeletonRig());
					assets->defineScheme<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(genAssetSchemeSkeletalAnimation());
					// engine assets
					assets->defineScheme<AssetSchemeIndexShader, MultiShader>(genAssetSchemeShader(+device));
					assets->defineScheme<AssetSchemeIndexTexture, Texture>(genAssetSchemeTexture(+device));
					assets->defineScheme<AssetSchemeIndexModel, Model>(genAssetSchemeModel(+device));
					assets->defineScheme<AssetSchemeIndexRenderObject, RenderObject>(genAssetSchemeRenderObject());
					assets->defineScheme<AssetSchemeIndexFont, Font>(genAssetSchemeFont());
					assets->defineScheme<AssetSchemeIndexSound, Sound>(genAssetSchemeSound());
					// cage pack
					assets->load(HashString("cage/cage.pack"));
				}

				{ // initialize assets change listening
					if (confAutoAssetListen || detail::assetsListenDefault())
					{
						CAGE_LOG(SeverityEnum::Info, "assets", "starting assets updates listening");
						assets->listen();
					}
				}

				{ // create threads
					threadsStateBarier = newBarrier(3);
					graphicsThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsEntry>(this), "engine graphics");
					soundThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::soundEntry>(this), "engine sound");
				}

				keybindsRegisterListeners(engineEvents());

				{
					ScopeLock l(threadsStateBarier);
				}
				CAGE_LOG(SeverityEnum::Info, "engine", "engine initialized");

				CAGE_ASSERT(engineStarted == 1);
				engineStarted = 2;
			}

			void run()
			{
				CAGE_ASSERT(engineStarted == 2);
				engineStarted = 3;

				try
				{
					controlThread().initialize.dispatch();
				}
				GCHL_GENERATE_CATCH(control, initialization - application);

				CAGE_LOG(SeverityEnum::Info, "engine", "starting engine");

				{
					ScopeLock l(threadsStateBarier);
				}
				{
					ScopeLock l(threadsStateBarier);
				}

				try
				{
					controlGameloopStage();
				}
				GCHL_GENERATE_CATCH(control, gameloop);

				CAGE_LOG(SeverityEnum::Info, "engine", "engine stopped");

				try
				{
					controlThread().finalize.dispatch();
				}
				GCHL_GENERATE_CATCH(control, finalization - application);

				CAGE_ASSERT(engineStarted == 3);
				engineStarted = 4;
			}

			void finalize()
			{
				CAGE_ASSERT(engineStarted == 4);
				engineStarted = 5;

				CAGE_LOG(SeverityEnum::Info, "engine", "finalizing engine");
				{
					ScopeLock l(threadsStateBarier);
				}

				{ // release resources held by gui
					if (gui)
						gui->cleanUp();
					if (guiMixer)
						guiMixer->purge();
					guiBundle.clear();
				}

				if (assets)
				{ // unload assets
					onDemand->clear();
					assets->unload(HashString("cage/cage.pack"));
					while (!assets->empty())
					{
						try
						{
							while (engineAssets()->processCustomThread(0))
								;
							controlThread().unload.dispatch();
						}
						GCHL_GENERATE_CATCH(control, unload);
						threadYield();
					}
				}

				{ // wait for threads to finish
					graphicsThreadHolder->wait();
					soundThreadHolder->wait();
				}

				{ // destroy entities
					entities.clear();
				}

				{ // destroy gui
					gui.clear();
				}

				{ // destroy sound
					speaker.clear();
					masterBus.clear();
					sceneVoice.clear();
					sceneMixer.clear();
					guiVoice.clear();
					guiMixer.clear();
				}

				{ // destroy graphics
					virtualReality.clear();
					window.clear();
					device.clear();
				}

				{ // destroy assets
					onDemand.clear();
					assets.clear();
				}

				{ // destroy private
					privateGraphics.clear();
					privateSound.clear();
				}

				CAGE_LOG(SeverityEnum::Info, "engine", "engine finalized");

				CAGE_ASSERT(engineStarted == 5);
				engineStarted = 6;
			}
		};

		Holder<EngineData> engineData;
	}

	Scheduler *EngineControlThread::scheduler()
	{
		return engineData->controlScheduler.get();
	}

	uint64 EngineControlThread::updatePeriod() const
	{
		return engineData->controlUpdateSchedule->period();
	}

	void EngineControlThread::updatePeriod(uint64 p)
	{
		engineData->controlUpdateSchedule->period(p);
	}

	uint64 EngineControlThread::inputPeriod() const
	{
		return engineData->controlInputSchedule->period();
	}

	void EngineControlThread::inputPeriod(uint64 p)
	{
		engineData->controlInputSchedule->period(p);
	}

	Scheduler *EngineSoundThread::scheduler()
	{
		return engineData->soundScheduler.get();
	}

	void engineInitialize(const EngineCreateConfig &config)
	{
		CAGE_ASSERT(!engineData);
		engineData = systemMemory().createHolder<EngineData>();
		engineData->initialize(config);
	}

	void engineRun()
	{
		CAGE_ASSERT(engineData);
		engineData->run();
	}

	void engineStop()
	{
		CAGE_ASSERT(engineData);
		if (!engineData->stopping.exchange(true))
		{
			CAGE_LOG(SeverityEnum::Info, "engine", "stopping engine");
		}
	}

	void engineFinalize()
	{
		CAGE_ASSERT(engineData);
		engineData->finalize();
		engineData.clear();
	}

	AssetsManager *engineAssets()
	{
		return +engineData->assets;
	}

	AssetsOnDemand *engineAssetsOnDemand()
	{
		return +engineData->onDemand;
	}

	EntityManager *engineEntities()
	{
		return +engineData->entities;
	}

	GraphicsDevice *engineGraphicsDevice()
	{
		CAGE_ASSERT(engineData && engineData->device);
		return +engineData->device;
	}

	Window *engineWindow()
	{
		CAGE_ASSERT(engineData && engineData->window);
		return +engineData->window;
	}

	VirtualReality *engineVirtualReality()
	{
		return +engineData->virtualReality;
	}

	EntityManager *engineGuiEntities()
	{
		return engineData->gui->entities();
	}

	GuiManager *engineGuiManager()
	{
		return +engineData->gui;
	}

	Speaker *engineSpeaker()
	{
		return +engineData->speaker;
	}

	VoicesMixer *engineMasterMixer()
	{
		return +engineData->masterBus;
	}

	VoicesMixer *engineSceneMixer()
	{
		return +engineData->sceneMixer;
	}

	SoundsQueue *engineGuiMixer()
	{
		return +engineData->guiMixer;
	}

	uint64 engineControlTime()
	{
		return engineData->controlTime;
	}

	Holder<Image> engineScreenshot()
	{
		return engineData->privateGraphics->screenshot();
	}

	uint64 engineStatisticsValues(StatisticsGuiFlags flags, StatisticsGuiModeEnum mode)
	{
		uint64 result = 0;

		if (any(flags & StatisticsGuiFlags::CpuUtilization))
			result += 100 * engineData->controlScheduler->utilization();

		if (any(flags & StatisticsGuiFlags::DynamicResolution))
		{
			switch (mode)
			{
				case StatisticsGuiModeEnum::Average:
					result += numeric_cast<uint32>(100 * engineData->profilingBufferDynamicResolution.smooth());
					break;
				case StatisticsGuiModeEnum::Maximum:
					// maximum/worst time corresponds to minimum dynamic resolution
					result += numeric_cast<uint32>(100 * engineData->profilingBufferDynamicResolution.min());
					break;
				case StatisticsGuiModeEnum::Latest:
					result += numeric_cast<uint32>(100 * engineData->profilingBufferDynamicResolution.current());
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid profiling mode enum");
			}
		}

		{
			const auto &add = [&](const auto &s)
			{
				switch (mode)
				{
					case StatisticsGuiModeEnum::Average:
						result += s.avgDuration;
						break;
					case StatisticsGuiModeEnum::Maximum:
						result += s.maxDuration;
						break;
					case StatisticsGuiModeEnum::Latest:
						result += s.latestDuration;
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid profiling mode enum");
				}
			};

			if (any(flags & StatisticsGuiFlags::ControlTime))
				add(engineData->controlUpdateSchedule->statistics());

			if (any(flags & StatisticsGuiFlags::SoundTime))
				add(engineData->soundUpdateSchedule->statistics());
		}

		{
			const auto &add = [&](const auto &buffer)
			{
				switch (mode)
				{
					case StatisticsGuiModeEnum::Average:
						result += buffer.smooth();
						break;
					case StatisticsGuiModeEnum::Maximum:
						result += buffer.max();
						break;
					case StatisticsGuiModeEnum::Latest:
						result += buffer.current();
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid profiling mode enum");
				}
			};

			if (any(flags & StatisticsGuiFlags::GpuTime))
				add(engineData->profilingBufferGpuTime);
			if (any(flags & StatisticsGuiFlags::FrameTime))
				add(engineData->profilingBufferFrameTime);
			if (any(flags & StatisticsGuiFlags::DrawCalls))
				add(engineData->profilingBufferDrawCalls);
			if (any(flags & StatisticsGuiFlags::DrawPrimitives))
				add(engineData->profilingBufferDrawPrimitives);
			if (any(flags & StatisticsGuiFlags::Entities))
				add(engineData->profilingBufferEntities);
		}
		return result;
	}
}
