#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h> // getExecutableName
#include <cage-core/skeletalAnimation.h> // for sizeof in defineScheme
#include <cage-core/collider.h> // for sizeof in defineScheme
#include <cage-core/textPack.h> // for sizeof in defineScheme
#include <cage-core/threadPool.h>
#include <cage-core/scheduler.h>
#include <cage-core/debug.h>
#include <cage-core/macros.h>
#include <cage-core/logger.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/profiling.h>
#include <cage-core/entities.h>

#include <cage-engine/graphicsError.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/font.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/window.h>
#include <cage-engine/sound.h>
#include <cage-engine/speaker.h>
#include <cage-engine/voices.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/virtualReality.h>

#include <cage-simple/statisticsGui.h>

#include "engine.h"

#include <atomic>
#include <exception>

namespace cage
{
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

			~ScopedSemaphores()
			{
				sem->unlock();
			}

		private:
			Semaphore *sem;
		};

		struct ScopedTimer : private Immovable
		{
			VariableSmoothingBuffer<uint64, 100> &vsb;
			uint64 st;

			explicit ScopedTimer(VariableSmoothingBuffer<uint64, 100> &vsb) : vsb(vsb), st(applicationTime())
			{}

			~ScopedTimer()
			{
				uint64 et = applicationTime();
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
			VariableSmoothingBuffer<uint64, 100> profilingBufferGraphicsPrepare;
			VariableSmoothingBuffer<uint64, 100> profilingBufferGraphicsDispatch;
			VariableSmoothingBuffer<uint64, 100> profilingBufferFrameTime;
			VariableSmoothingBuffer<uint64, 100> profilingBufferDrawCalls;
			VariableSmoothingBuffer<uint64, 100> profilingBufferDrawPrimitives;
			VariableSmoothingBuffer<uint64, 100> profilingBufferEntities;

			Holder<AssetManager> assets;
			Holder<Window> window;
			Holder<VirtualReality> virtualReality;
			Holder<Speaker> speaker;
			Holder<VoicesMixer> masterBus;
			Holder<VoicesMixer> effectsBus;
			Holder<VoicesMixer> guiBus;
			Holder<Voice> effectsVoice;
			Holder<Voice> guiVoice;
			Holder<GuiManager> gui;
			ExclusiveHolder<RenderQueue> guiRenderQueue;
			Holder<EntityManager> entities;

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
			EventListener<void(const GenericInput &)> virtualRealityEventsListener;

			EngineData(const EngineCreateConfig &config);

			~EngineData();

			//////////////////////////////////////
			// graphics PREPARE
			//////////////////////////////////////

			void graphicsPrepareInitializeStage()
			{}

			void graphicsPrepareStep()
			{
				ProfilingScope profiling("graphics prepare", ProfilingFrameTag());
				{
					ProfilingScope profiling("graphics prepare callback");
					graphicsPrepareThread().prepare.dispatch();
				}
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore1, graphicsSemaphore2);
					ProfilingScope profiling("graphics prepare run");
					ScopedTimer timing(profilingBufferGraphicsPrepare);
					uint32 drawCalls = 0, drawPrimitives = 0;
					graphicsPrepare(applicationTime(), drawCalls, drawPrimitives);
					profilingBufferDrawCalls.add(drawCalls);
					profilingBufferDrawPrimitives.add(drawPrimitives);
				}
			}

			void graphicsPrepareGameloopStage()
			{
				while (!stopping)
					graphicsPrepareStep();
			}

			void graphicsPrepareStopStage()
			{
				graphicsSemaphore2->unlock();
			}

			void graphicsPrepareFinalizeStage()
			{}

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
					ProfilingScope profiling("graphics dispatch callback");
					graphicsDispatchThread().dispatch.dispatch();
				}
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore2, graphicsSemaphore1);
					ProfilingScope profiling("graphics dispatch run");
					ScopedTimer timing(profilingBufferGraphicsDispatch);
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
					ProfilingScope profiling("graphics assets");
					while (assets->processCustomThread(EngineGraphicsDispatchThread::threadIndex));
				}
				{
					ProfilingScope profiling("swap callback");
					graphicsDispatchThread().swap.dispatch();
				}
				{
					ProfilingScope profiling("swap");
					graphicsSwap();
				}
			}

			void graphicsDispatchGameloopStage()
			{
				while (!stopping)
					graphicsDispatchStep();
			}

			void graphicsDispatchStopStage()
			{
				graphicsSemaphore1->unlock();
			}

			void graphicsDispatchFinalizeStage()
			{
				graphicsFinalize();
				assets->unloadCustomThread(graphicsDispatchThread().threadIndex);
			}

			//////////////////////////////////////
			// SOUND
			//////////////////////////////////////

			void soundInitializeStage()
			{
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
				{
					ProfilingScope profiling("sound callback");
					soundThread().sound.dispatch();
				}
				{
					ProfilingScope profiling("sound run");
					soundTick(soundUpdateSchedule->time());
				}
			}

			void soundGameloopStage()
			{
				soundScheduler->run();
			}

			void soundStopStage()
			{
				speaker->stop();
			}

			void soundFinalizeStage()
			{
				soundFinalize();
			}

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void updateHistoryComponents()
			{
				for (Entity *e : engineEntities()->component<TransformComponent>()->entities())
				{
					TransformComponent &ts = e->value<TransformComponent>();
					TransformComponent &hs = e->value<TransformComponent>(transformHistoryComponent);
					hs = ts;
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
				if (virtualReality)
				{
					ProfilingScope profiling("virtual reality events");
					virtualReality->processEvents();
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
					ProfilingScope profiling("update history components");
					updateHistoryComponents();
				}
				{
					ProfilingScope profiling("control callback");
					controlThread().update.dispatch();
				}
				{
					ProfilingScope profiling("sound emit");
					soundEmit(controlTime);
				}
				{
					ProfilingScope profiling("graphics emit");
					graphicsEmit(controlTime);
				}
				profilingBufferEntities.add(entities->group()->count());
			}

			void controlGameloopStage()
			{
				controlScheduler->run();
			}

			//////////////////////////////////////
			// ENTRY methods
			//////////////////////////////////////

#define GCHL_GENERATE_CATCH(NAME, STAGE) \
			catch (...) { CAGE_LOG(SeverityEnum::Error, "engine", "unhandled exception in " CAGE_STRINGIZE(STAGE) " in " CAGE_STRINGIZE(NAME)); detail::logCurrentCaughtException(); engineStop(); }
#define GCHL_GENERATE_ENTRY(NAME) \
			void CAGE_JOIN(NAME, Entry)() \
			{ \
				try { CAGE_JOIN(NAME, InitializeStage)(); } \
				GCHL_GENERATE_CATCH(NAME, initialization (engine)); \
				{ ScopeLock l(threadsStateBarier); } \
				{ ScopeLock l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().initialize.dispatch(); } \
				GCHL_GENERATE_CATCH(NAME, initialization (application)); \
				{ ScopeLock l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, GameloopStage)(); } \
				GCHL_GENERATE_CATCH(NAME, gameloop); \
				CAGE_JOIN(NAME, StopStage)(); \
				{ ScopeLock l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().finalize.dispatch(); } \
				GCHL_GENERATE_CATCH(NAME, finalization (application)); \
				try { CAGE_JOIN(NAME, FinalizeStage)(); } \
				GCHL_GENERATE_CATCH(NAME, finalization (engine)); \
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphicsPrepare, graphicsDispatch, sound));
#undef GCHL_GENERATE_ENTRY

			void initialize(const EngineCreateConfig &config)
			{
				CAGE_ASSERT(engineStarted == 0);
				engineStarted = 1;

				CAGE_LOG(SeverityEnum::Info, "engine", "initializing engine");

				controlUpdateSchedule->period(1000000 / (config.virtualReality ? 90 : 20));
				controlInputSchedule->period(1000000 / (config.virtualReality ? 90 : 60));

				{ // create entities
					entities = newEntityManager();
				}

				{ // create assets manager
					AssetManagerCreateConfig c;
					if (config.assets)
						c = *config.assets;
					c.schemesMaxCount = max(c.schemesMaxCount, 30u);
					c.customProcessingThreads = max(c.customProcessingThreads, 5u);
					assets = newAssetManager(c);
				}

				{ // create graphics
					WindowCreateConfig cfg;
					if (config.window)
						cfg = *config.window;
					if (config.virtualReality)
						cfg.vsync = 0; // explicitly disable vsync for the window when virtual reality controls frame rate
					window = newWindow(cfg);
					if (config.virtualReality)
					{
						virtualReality = newVirtualReality(*config.virtualReality);
						virtualRealityEventsListener.attach(virtualReality->events, -123456);
						virtualRealityEventsListener.bind<EntityManager *, virtualRealitySceneUpdate>(engineEntities());
					}
					window->makeNotCurrent();
				}

				{ // create sound
					masterBus = newVoicesMixer({});
					effectsBus = newVoicesMixer({});
					guiBus = newVoicesMixer({});
					effectsVoice = masterBus->newVoice();
					guiVoice = masterBus->newVoice();
					effectsVoice->callback.bind<VoicesMixer, &VoicesMixer::process>(+effectsBus);
					guiVoice->callback.bind<VoicesMixer, &VoicesMixer::process>(+guiBus);
					SpeakerCreateConfig scc;
					scc.sampleRate = 48000; // minimize sample rate conversions
					speaker = newSpeaker(config.speaker ? *config.speaker : scc, Delegate<void(const SoundCallbackData &)>().bind<VoicesMixer, &VoicesMixer::process>(+masterBus));
				}

				{ // create gui
					GuiManagerCreateConfig c;
					if (config.gui)
						c = *config.gui;
					c.assetMgr = +assets;
					gui = newGuiManager(c);
					windowGuiEventsListener.attach(window->events);
					windowGuiEventsListener.bind<GuiManager, &GuiManager::handleInput>(+gui);
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
					assets->defineScheme<AssetSchemeIndexTextPack, TextPack>(genAssetSchemeTextPack());
					assets->defineScheme<AssetSchemeIndexCollider, Collider>(genAssetSchemeCollider());
					assets->defineScheme<AssetSchemeIndexSkeletonRig, SkeletonRig>(genAssetSchemeSkeletonRig());
					assets->defineScheme<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(genAssetSchemeSkeletalAnimation());
					// engine assets
					assets->defineScheme<AssetSchemeIndexShaderProgram, MultiShaderProgram>(genAssetSchemeShaderProgram(EngineGraphicsDispatchThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexTexture, Texture>(genAssetSchemeTexture(EngineGraphicsDispatchThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexModel, Model>(genAssetSchemeModel(EngineGraphicsDispatchThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexRenderObject, RenderObject>(genAssetSchemeRenderObject());
					assets->defineScheme<AssetSchemeIndexFont, Font>(genAssetSchemeFont(EngineGraphicsDispatchThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexSound, Sound>(genAssetSchemeSound());
					// cage pack
					assets->add(HashString("cage/cage.pack"));
				}

				{ // initialize assets change listening
					if (confAutoAssetListen)
					{
						CAGE_LOG(SeverityEnum::Info, "assets", "starting assets updates listening");
						assets->listen();
					}
				}

				{ // initialize entity components
					EntityManager *entityMgr = entities.get();
					entityMgr->defineComponent(TransformComponent());
					transformHistoryComponent = entityMgr->defineComponent(TransformComponent());
					entityMgr->defineComponent(RenderComponent());
					entityMgr->defineComponent(TextureAnimationComponent());
					entityMgr->defineComponent(SkeletalAnimationComponent());
					entityMgr->defineComponent(LightComponent());
					entityMgr->defineComponent(ShadowmapComponent());
					entityMgr->defineComponent(TextComponent());
					entityMgr->defineComponent(CameraComponent());
					entityMgr->defineComponent(ScreenSpaceEffectsComponent());
					entityMgr->defineComponent(SoundComponent());
					entityMgr->defineComponent(ListenerComponent());
					entityMgr->defineComponent(VrOriginComponent());
					entityMgr->defineComponent(VrCameraComponent());
					entityMgr->defineComponent(VrControllerComponent());
				}

				{ ScopeLock l(threadsStateBarier); }
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
				GCHL_GENERATE_CATCH(control, initialization (application));

				CAGE_LOG(SeverityEnum::Info, "engine", "starting engine");

				{ ScopeLock l(threadsStateBarier); }
				{ ScopeLock l(threadsStateBarier); }

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
				GCHL_GENERATE_CATCH(control, finalization (application));

				CAGE_ASSERT(engineStarted == 3);
				engineStarted = 4;
			}

			void finalize()
			{
				CAGE_ASSERT(engineStarted == 4);
				engineStarted = 5;

				CAGE_LOG(SeverityEnum::Info, "engine", "finalizing engine");
				{ ScopeLock l(threadsStateBarier); }

				{ // release resources held by gui
					guiRenderQueue.clear();
					if (gui)
						gui->cleanUp();
				}

				{ // unload assets
					assets->remove(HashString("cage/cage.pack"));
					while (!assets->unloaded())
					{
						try
						{
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
					effectsVoice.clear();
					guiVoice.clear();
					masterBus.clear();
					effectsBus.clear();
					guiBus.clear();
				}

				{ // destroy graphics
					if (window)
						window->makeCurrent();
					if (virtualReality)
						virtualReality.clear();
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

			controlScheduler = newScheduler({});
			{
				ScheduleCreateConfig c;
				c.name = "control schedule";
				c.action = Delegate<void()>().bind<EngineData, &EngineData::controlUpdate>(this);
				c.type = ScheduleTypeEnum::SteadyPeriodic;
				controlUpdateSchedule = controlScheduler->newSchedule(c);
			}
			{
				ScheduleCreateConfig c;
				c.name = "inputs schedule";
				c.action = Delegate<void()>().bind<EngineData, &EngineData::controlInputs>(this);
				c.type = ScheduleTypeEnum::FreePeriodic;
				controlInputSchedule = controlScheduler->newSchedule(c);
			}

			soundScheduler = newScheduler({});
			{
				ScheduleCreateConfig c;
				c.name = "sound schedule";
				c.action = Delegate<void()>().bind<EngineData, &EngineData::soundUpdate>(this);
				c.period = 1000000 / 40;
				c.type = ScheduleTypeEnum::SteadyPeriodic;
				soundUpdateSchedule = soundScheduler->newSchedule(c);
			}

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

	uint64 EngineSoundThread::updatePeriod() const
	{
		return engineData->soundUpdateSchedule->period();
	}

	void EngineSoundThread::updatePeriod(uint64 p)
	{
		engineData->soundUpdateSchedule->period(p);
	}

	void engineInitialize(const EngineCreateConfig &config)
	{
		CAGE_ASSERT(!engineData);
		engineData = systemMemory().createHolder<EngineData>(config);
		engineData->initialize(config);
	}

	void engineStart()
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

	AssetManager *engineAssets()
	{
		return +engineData->assets;
	}

	EntityManager *engineEntities()
	{
		return +engineData->entities;
	}

	Window *engineWindow()
	{
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

	VoicesMixer *engineEffectsMixer()
	{
		return +engineData->effectsBus;
	}

	VoicesMixer *engineGuiMixer()
	{
		return +engineData->guiBus;
	}

	uint64 engineControlTime()
	{
		return engineData->controlTime;
	}

	uint64 engineStatisticsValues(StatisticsGuiFlags flags, StatisticsGuiModeEnum mode)
	{
		uint64 result = 0;

		const auto &add = [&](const auto &buffer)
		{
			switch (mode)
			{
			case StatisticsGuiModeEnum::Average: result += buffer.smooth(); break;
			case StatisticsGuiModeEnum::Maximum: result += buffer.max(); break;
			case StatisticsGuiModeEnum::Last: result += buffer.current(); break;
			default: CAGE_THROW_CRITICAL(Exception, "invalid profiling mode enum");
			}
		};

		if (any(flags & StatisticsGuiFlags::Control))
			add(engineData->controlUpdateSchedule->statistics().durations);
		if (any(flags & StatisticsGuiFlags::Sound))
			add(engineData->soundUpdateSchedule->statistics().durations);

#define GCHL_GENERATE(NAME) \
		if (any(flags & StatisticsGuiFlags::NAME)) \
		{ \
			auto &buffer = CAGE_JOIN(engineData->profilingBuffer, NAME); \
			add(buffer); \
		}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE,
			GraphicsPrepare,
			GraphicsDispatch,
			FrameTime,
			DrawCalls,
			DrawPrimitives,
			Entities
		));
#undef GCHL_GENERATE

		return result;
	}
}
