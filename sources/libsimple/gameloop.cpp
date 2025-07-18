#include <atomic>
#include <exception>

#include "engine.h"

#include <cage-core/assetContext.h>
#include <cage-core/assetsManager.h>
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
#include <cage-engine/font.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/keybinds.h>
#include <cage-engine/model.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/shaderProgram.h>
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

		struct ScopedSemaphores : private Immovable
		{
			explicit ScopedSemaphores(Holder<Semaphore> &lock, Holder<Semaphore> &unlock) : sem(+unlock)
			{
				ProfilingScope profiling("semaphore waiting");
				lock->lock();
			}

			~ScopedSemaphores() { sem->unlock(); }

		private:
			Semaphore *sem = nullptr;
		};

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

		template<class T>
		struct ExclusiveHolder : private Immovable
		{
			void assign(Holder<T> &&value)
			{
				ScopeLock lock(mut);
				data = std::move(value);
			}

			Holder<T> get() const
			{
				ScopeLock lock(mut);
				if (data)
					return data.share();
				return {};
			}

			void clear()
			{
				Holder<T> tmp;
				{
					ScopeLock lock(mut);
					std::swap(tmp, data); // swap under lock
				}
				tmp.clear(); // clear outside lock
			}

		private:
			Holder<T> data;
			Holder<Mutex> mut = newMutex();
		};

		struct EngineData
		{
			VariableSmoothingBuffer<Real, 60> profilingBufferDynamicResolution;
			VariableSmoothingBuffer<uint64, 60> profilingBufferPrepareTime;
			VariableSmoothingBuffer<uint64, 60> profilingBufferGpuTime;
			VariableSmoothingBuffer<uint64, 60> profilingBufferFrameTime;
			VariableSmoothingBuffer<uint64, 60> profilingBufferDrawCalls;
			VariableSmoothingBuffer<uint64, 60> profilingBufferDrawPrimitives;
			VariableSmoothingBuffer<uint64, 30> profilingBufferEntities;

			Holder<AssetsManager> assets;
			Holder<Window> window;
			Holder<VirtualReality> virtualReality;
			Holder<Speaker> speaker;
			Holder<VoicesMixer> masterBus;
			Holder<VoicesMixer> sceneMixer;
			Holder<SoundsQueue> guiMixer;
			Holder<Voice> sceneVoice;
			Holder<Voice> guiVoice;
			Holder<GuiManager> gui;
			ExclusiveHolder<RenderQueue> guiRenderQueue;
			Holder<EntityManager> entities;
			Holder<ProvisionalGraphics> provisionalGraphics;

			Holder<Semaphore> graphicsSemaphore1;
			Holder<Semaphore> graphicsSemaphore2;
			Holder<Barrier> threadsStateBarier;
			Holder<Thread> graphicsDispatchThreadHolder;
			Holder<Thread> graphicsPrepareThreadHolder;
			Holder<Thread> soundThreadHolder;

			std::atomic<uint32> engineStarted = 0;
			std::atomic<bool> stopping = false;
			uint64 controlTime = 0;

			Holder<Scheduler> controlScheduler;
			Holder<Schedule> controlUpdateSchedule;
			Holder<Schedule> controlInputSchedule;
			Holder<Scheduler> soundScheduler;
			Holder<Schedule> soundUpdateSchedule;

			EventListener<bool(const GenericInput &)> windowGuiEventsListener;

			EngineData(const EngineCreateConfig &config);

			~EngineData();

			//////////////////////////////////////
			// graphics PREPARE
			//////////////////////////////////////

			void graphicsPrepareInitializeStage() {}

			void graphicsPrepareStep()
			{
				ProfilingScope profiling("graphics prepare", ProfilingFrameTag());
				{
					ProfilingScope profiling("graphics prepare callback");
					graphicsPrepareThread().prepare.dispatch();
				}
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore1, graphicsSemaphore2);
					if (stopping)
						return; // prevent getting stuck in virtual reality waiting for previous (already terminated) frame dispatch
					ProfilingScope profiling("graphics prepare run");
					ScopedTimer timing(profilingBufferPrepareTime);
					uint32 drawCalls = 0, drawPrimitives = 0;
					Real dynamicResolution;
					graphicsPrepare(applicationTime(), drawCalls, drawPrimitives, dynamicResolution);
					profilingBufferDrawCalls.add(drawCalls);
					profilingBufferDrawPrimitives.add(drawPrimitives);
					profilingBufferDynamicResolution.add(dynamicResolution);
				}
			}

			void graphicsPrepareGameloopStage()
			{
				while (!stopping)
					graphicsPrepareStep();
			}

			void graphicsPrepareStopStage() { graphicsSemaphore2->unlock(); }

			void graphicsPrepareFinalizeStage() {}

			//////////////////////////////////////
			// graphics DISPATCH
			//////////////////////////////////////

			void graphicsDispatchInitializeStage()
			{
				window->makeCurrent();
				graphicsInitialize();
			}

			void graphicsDispatchStep()
			{
				ProfilingScope profiling("graphics dispatch", ProfilingFrameTag());
				ScopedTimer timing(profilingBufferFrameTime);
				{
					ProfilingScope profiling("frame start");
					graphicsFrameStart();
				}
				{
					ProfilingScope profiling("graphics dispatch callback");
					graphicsDispatchThread().dispatch.dispatch();
				}
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore2, graphicsSemaphore1);
					ProfilingScope profiling("graphics dispatch run");
					graphicsDispatch();
				}
				{
					ProfilingScope profiling("dispatch gui");
					Holder<RenderQueue> grq = guiRenderQueue.get();
					if (grq)
						grq->dispatch();
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				{
					ProfilingScope profiling("frame finish");
					uint64 gpuTime = 0;
					graphicsFrameFinish(gpuTime);
					profilingBufferGpuTime.add(gpuTime);
				}
				{
					ProfilingScope profiling("swap callback");
					graphicsDispatchThread().swap.dispatch();
				}
				{
					ProfilingScope profiling("swap");
					graphicsSwap();
				}
				{
					ProfilingScope profiling("graphics assets");
					const uint64 start = applicationTime();
					while (assets->processCustomThread(1))
					{
						if (applicationTime() > start + 5'000)
							break;
					}
				}
			}

			void graphicsDispatchGameloopStage()
			{
				while (!stopping)
					graphicsDispatchStep();
			}

			void graphicsDispatchStopStage() { graphicsSemaphore1->unlock(); }

			void graphicsDispatchFinalizeStage()
			{
				graphicsFinalize();
				assets->unloadCustomThread(1);
				window->makeNotCurrent();
			}

			//////////////////////////////////////
			// SOUND
			//////////////////////////////////////

			void soundInitializeStage() { speaker->start(); }

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
					ProfilingScope profiling("sound run");
					soundTick(soundUpdateSchedule->time());
				}
				{
					ProfilingScope profiling("sound dispatch");
					soundDispatch();
				}
			}

			void soundGameloopStage() { soundScheduler->run(); }

			void soundStopStage() { speaker->stop(); }

			void soundFinalizeStage() { soundFinalize(); }

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void updateComponents()
			{
				for (Entity *e : engineEntities()->component<TransformComponent>()->entities())
				{
					TransformComponent &ts = e->value<TransformComponent>();
					TransformComponent &hs = e->value<TransformComponent>(transformHistoryComponent);
					hs = ts;
				}
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
					guiRenderQueue.assign(gui->finish());
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
					soundEmit(controlTime);
				}
				{
					ProfilingScope profiling("graphics emit");
					graphicsEmit(controlTime);
				}
				profilingBufferEntities.add(entities->count());
				{
					ProfilingScope profiling("control assets");
					const uint64 start = applicationTime();
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
		GCHL_GENERATE_CATCH(NAME, initialization(engine)); \
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
		GCHL_GENERATE_CATCH(NAME, initialization(application)); \
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
		GCHL_GENERATE_CATCH(NAME, finalization(application)); \
		try \
		{ \
			CAGE_JOIN(NAME, FinalizeStage)(); \
		} \
		GCHL_GENERATE_CATCH(NAME, finalization(engine)); \
	}
			CAGE_EVAL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphicsPrepare, graphicsDispatch, sound));
#undef GCHL_GENERATE_ENTRY

			void initialize(const EngineCreateConfig &config)
			{
				CAGE_ASSERT(engineStarted == 0);
				engineStarted = 1;

				CAGE_LOG(SeverityEnum::Info, "engine", "initializing engine");

				{
					SchedulerCreateConfig cfg;
					controlScheduler = newScheduler(cfg);
				}
				{
					ScheduleCreateConfig cfg;
					cfg.name = "control schedule";
					cfg.action = Delegate<void()>().bind<EngineData, &EngineData::controlUpdate>(this);
					cfg.period = 1000000 / 20;
					cfg.type = ScheduleTypeEnum::SteadyPeriodic;
					controlUpdateSchedule = controlScheduler->newSchedule(cfg);
				}
				{
					ScheduleCreateConfig cfg;
					cfg.name = "inputs schedule";
					cfg.action = Delegate<void()>().bind<EngineData, &EngineData::controlInputs>(this);
					cfg.period = 1000000 / 60;
					cfg.type = ScheduleTypeEnum::FreePeriodic;
					controlInputSchedule = controlScheduler->newSchedule(cfg);
				}

				soundScheduler = newScheduler({});
				{
					ScheduleCreateConfig c;
					c.name = "sound schedule";
					c.action = Delegate<void()>().bind<EngineData, &EngineData::soundUpdate>(this);
					c.period = 1000000 / 20;
					c.type = ScheduleTypeEnum::SteadyPeriodic;
					soundUpdateSchedule = soundScheduler->newSchedule(c);
				}

				{ // create entities
					entities = newEntityManager();
				}

				{ // create assets manager
					AssetManagerCreateConfig cfg;
					if (config.assets)
						cfg = *config.assets;
					cfg.schemesMaxCount = max(cfg.schemesMaxCount, 30u);
					cfg.customProcessingThreads = max(cfg.customProcessingThreads, 5u);
					assets = newAssetsManager(cfg);
				}

				{ // create graphics
					WindowCreateConfig cfg;
					if (config.window)
						cfg = *config.window;
					if (config.virtualReality)
						cfg.vsync = 0; // explicitly disable vsync for the window when virtual reality controls frame rate
					window = newWindow(cfg);
					window->events.merge(engineEvents());
					if (config.virtualReality)
					{
						virtualReality = newVirtualReality();
						virtualReality->events.merge(engineEvents());
						controlUpdateSchedule->period(virtualReality->targetFrameTiming());
					}
					window->makeNotCurrent();
					provisionalGraphics = newProvisionalGraphics();
				}

				{ // create sound speaker
					masterBus = newVoicesMixer();
					SpeakerCreateConfig cfg;
					if (config.speaker)
						cfg = *config.speaker;
					if (cfg.sampleRate == 0)
						cfg.sampleRate = 48000; // minimize sample rate conversions
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
					cfg.provisionalGraphics = +provisionalGraphics;
					cfg.soundsQueue = +guiMixer;
					gui = newGuiManager(cfg);
					gui->widgetEvent.merge(engineEvents());
					windowGuiEventsListener.attach(window->events, -1000);
					windowGuiEventsListener.bind([gui = +gui](const GenericInput &in) { return gui->handleInput(in); });
				}

				{ // create sync objects
					threadsStateBarier = newBarrier(4);
					graphicsSemaphore1 = newSemaphore(1, 1);
					graphicsSemaphore2 = newSemaphore(0, 1);
				}

				{ // create threads
					graphicsDispatchThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsDispatchEntry>(this), "engine graphics dispatch");
					graphicsPrepareThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsPrepareEntry>(this), "engine graphics prepare");
					soundThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::soundEntry>(this), "engine sound");
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
					assets->defineScheme<AssetSchemeIndexShaderProgram, MultiShaderProgram>(genAssetSchemeShaderProgram(1));
					assets->defineScheme<AssetSchemeIndexTexture, Texture>(genAssetSchemeTexture(1));
					assets->defineScheme<AssetSchemeIndexModel, Model>(genAssetSchemeModel(1));
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

				{ // initialize entity components
					EntityManager *entityMgr = +entities;
					entityMgr->defineComponent(TransformComponent());
					transformHistoryComponent = entityMgr->defineComponent(TransformComponent());
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
					entityMgr->defineComponent(VrOriginComponent());
					entityMgr->defineComponent(VrCameraComponent());
					entityMgr->defineComponent(VrControllerComponent());
				}

				keybindsRegisterListeners(engineEvents());

				{
					ScopeLock l(threadsStateBarier);
				}
				CAGE_LOG(SeverityEnum::Info, "engine", "engine initialized");

				CAGE_ASSERT(engineStarted == 1);
				engineStarted = 2;
			}

			void start()
			{
				CAGE_ASSERT(engineStarted == 2);
				engineStarted = 3;

				try
				{
					controlThread().initialize.dispatch();
				}
				GCHL_GENERATE_CATCH(control, initialization(application));

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
				GCHL_GENERATE_CATCH(control, finalization(application));

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
					guiRenderQueue.clear();
					if (gui)
						gui->cleanUp();
					if (provisionalGraphics)
						provisionalGraphics->purge();
					if (guiMixer)
						guiMixer->purge();
				}

				if (assets)
				{ // unload assets
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
					graphicsPrepareThreadHolder->wait();
					graphicsDispatchThreadHolder->wait();
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
					if (window)
						window->makeCurrent();
					virtualReality.clear();
					provisionalGraphics.clear();
					window.clear();
				}

				{ // destroy assets
					assets.clear();
				}

				CAGE_LOG(SeverityEnum::Info, "engine", "engine finalized");

				CAGE_ASSERT(engineStarted == 5);
				engineStarted = 6;
			}
		};

		Holder<EngineData> engineData;

		EngineData::EngineData(const EngineCreateConfig &config)
		{
			CAGE_LOG(SeverityEnum::Info, "engine", "creating engine");
			graphicsCreate(config);
			soundCreate(config);
			CAGE_LOG(SeverityEnum::Info, "engine", "engine created");
		}

		EngineData::~EngineData()
		{
			CAGE_LOG(SeverityEnum::Info, "engine", "destroying engine");
			soundDestroy();
			graphicsDestroy();
			CAGE_LOG(SeverityEnum::Info, "engine", "engine destroyed");
		}
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
		engineData = systemMemory().createHolder<EngineData>(config);
		engineData->initialize(config);
	}

	void engineRun()
	{
		CAGE_ASSERT(engineData);
		engineData->start();
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

	EntityManager *engineEntities()
	{
		return +engineData->entities;
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

	ProvisionalGraphics *engineProvisionalGraphics()
	{
		return +engineData->provisionalGraphics;
	}

	uint64 engineControlTime()
	{
		return engineData->controlTime;
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
					// maximum/worts time corresponds to minimum dynamic resolution
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

			if (any(flags & StatisticsGuiFlags::ControlThreadTime))
				add(engineData->controlUpdateSchedule->statistics());

			if (any(flags & StatisticsGuiFlags::SoundThreadTime))
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

			if (any(flags & StatisticsGuiFlags::PrepareThreadTime))
				add(engineData->profilingBufferPrepareTime);
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
