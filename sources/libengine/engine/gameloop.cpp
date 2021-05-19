#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h> // getExecutableName
#include <cage-core/skeletalAnimation.h> // for sizeof in defineScheme
#include <cage-core/collider.h> // for sizeof in defineScheme
#include <cage-core/textPack.h> // for sizeof in defineScheme
#include <cage-core/memoryBuffer.h> // for sizeof in defineScheme
#include <cage-core/threadPool.h>
#include <cage-core/scheduler.h>
#include <cage-core/debug.h>
#include <cage-core/macros.h>
#include <cage-core/logger.h>
#include <cage-core/assetContext.h>

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
#include <cage-engine/gui.h>
#include <cage-engine/engineProfiling.h>
#include <cage-engine/renderQueue.h>

#include "engine.h"

#include <atomic>
#include <exception>

//#define CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS

namespace cage
{
	namespace
	{
		ConfigBool confAutoAssetListen("cage/assets/listen", false);

		struct EngineGraphicsUploadThread
		{
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			static constexpr uint32 threadIndex = 2;
#else
			static constexpr uint32 threadIndex = EngineGraphicsDispatchThread::threadIndex;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
		};

		struct ScopedSemaphores : private Immovable
		{
			explicit ScopedSemaphores(Holder<Semaphore> &lock, Holder<Semaphore> &unlock) : sem(unlock.get())
			{
				OPTICK_EVENT("waiting");
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
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> &vsb;
			uint64 st;

			explicit ScopedTimer(VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> &vsb) : vsb(vsb), st(applicationTime())
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
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferGraphicsPrepare;
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferGraphicsDispatch;
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferFrameTime;
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferDrawCalls;
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferDrawPrimitives;
			VariableSmoothingBuffer<uint64, Schedule::StatisticsWindowSize> profilingBufferEntities;

			Holder<AssetManager> assets;
			Holder<Window> window;
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			Holder<Window> windowUpload;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			Holder<Speaker> speaker;
			Holder<VoicesMixer> masterBus;
			Holder<VoicesMixer> effectsBus;
			Holder<VoicesMixer> guiBus;
			Holder<Voice> effectsVoice;
			Holder<Voice> guiVoice;
			Holder<Gui> gui;
			ExclusiveHolder<RenderQueue> guiRenderQueue;
			Holder<EntityManager> entities;

			Holder<Semaphore> graphicsSemaphore1;
			Holder<Semaphore> graphicsSemaphore2;
			Holder<Barrier> threadsStateBarier;
			Holder<Thread> graphicsDispatchThreadHolder;
			Holder<Thread> graphicsPrepareThreadHolder;
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			Holder<Thread> graphicsUploadThreadHolder;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			Holder<Thread> soundThreadHolder;

			std::atomic<uint32> engineStarted = 0;
			std::atomic<bool> stopping = false;
			uint64 controlTime = 0;

			Holder<Scheduler> controlScheduler;
			Schedule *controlUpdateSchedule = nullptr;
			Schedule *controlInputSchedule = nullptr;
			Holder<Scheduler> soundScheduler;
			Schedule *soundUpdateSchedule = nullptr;

			EngineData(const EngineCreateConfig &config);

			~EngineData();

			//////////////////////////////////////
			// graphics PREPARE
			//////////////////////////////////////

			void graphicsPrepareInitializeStage()
			{}

			void graphicsPrepareStep()
			{
				OPTICK_EVENT("prepare");
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore1, graphicsSemaphore2);
					ScopedTimer timing(profilingBufferGraphicsPrepare);
					{
						OPTICK_EVENT("prepare callback");
						graphicsPrepareThread().prepare.dispatch();
					}
					{
						OPTICK_EVENT("tick");
						graphicsPrepareTick(applicationTime());
					}
				}
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(graphicsPrepareThread().threadIndex);
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
			{
				graphicsPrepareFinalize();
				assets->unloadCustomThread(graphicsPrepareThread().threadIndex);
			}

			//////////////////////////////////////
			// graphics DISPATCH
			//////////////////////////////////////

			void graphicsDispatchInitializeStage()
			{
				window->makeCurrent();
				graphicsDispatchInitialize();
			}

			void graphicsDispatchStep()
			{
				OPTICK_FRAME("engine graphics dispatch");
				ScopedTimer timing(profilingBufferFrameTime);
				{
					ScopedSemaphores lockGraphics(graphicsSemaphore2, graphicsSemaphore1);
					ScopedTimer timing(profilingBufferGraphicsDispatch);
					{
						OPTICK_EVENT("dispatch callback");
						graphicsDispatchThread().dispatch.dispatch();
					}
					{
						OPTICK_EVENT("tick");
						uint32 drawCalls = 0, drawPrimitives = 0;
						graphicsDispatchTick(drawCalls, drawPrimitives);
						profilingBufferDrawCalls.add(drawCalls);
						profilingBufferDrawPrimitives.add(drawPrimitives);
						OPTICK_TAG("draw calls", drawCalls);
						OPTICK_TAG("draw primitives", drawPrimitives);
					}
				}
				if (graphicsPrepareThread().stereoMode == StereoModeEnum::Mono)
				{
					OPTICK_EVENT("gui render");
					Holder<RenderQueue> grq = guiRenderQueue.get();
					if (grq)
						grq->dispatch();
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(EngineGraphicsDispatchThread::threadIndex);
				}
				{
					OPTICK_EVENT("swap callback");
					graphicsDispatchThread().swap.dispatch();
				}
				{
					OPTICK_EVENT("swap");
					graphicsDispatchSwap();
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
				graphicsDispatchFinalize();
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
				OPTICK_EVENT("update");
				if (stopping)
				{
					soundScheduler->stop();
					return;
				}
				{
					OPTICK_EVENT("sound callback");
					soundThread().sound.dispatch();
				}
				{
					OPTICK_EVENT("tick");
					soundTick(soundUpdateSchedule->time());
				}
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(EngineSoundThread::threadIndex);
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
				assets->unloadCustomThread(soundThread().threadIndex);
			}

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void updateHistoryComponents()
			{
				for (Entity *e : TransformComponent::component->entities())
				{
					CAGE_COMPONENT_ENGINE(Transform, ts, e);
					TransformComponent &hs = e->value<TransformComponent>(TransformComponent::componentHistory);
					hs = ts;
				}
			}

			void controlInputs()
			{
				OPTICK_EVENT("inputs");
				{
					OPTICK_EVENT("gui prepare");
					gui->outputResolution(window->resolution());
					gui->outputRetina(window->contentScaling());
					gui->prepare();
				}
				{
					OPTICK_EVENT("window events");
					window->processEvents();
				}
				{
					OPTICK_EVENT("gui finish");
					guiRenderQueue.assign(gui->finish());
				}
			}

			void controlUpdate()
			{
				OPTICK_EVENT("update");
				if (stopping)
				{
					controlScheduler->stop();
					return;
				}
				controlTime = controlUpdateSchedule->time();
				{
					OPTICK_EVENT("update history components");
					updateHistoryComponents();
				}
				{
					OPTICK_EVENT("update callback");
					controlThread().update.dispatch();
				}
				{
					OPTICK_EVENT("sound emit");
					soundEmit(controlTime);
				}
				{
					OPTICK_EVENT("graphics emit");
					graphicsPrepareEmit(controlTime);
				}
				OPTICK_TAG("entities count", entities->group()->count());
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

#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			void graphicsUploadEntry()
			{
				windowUpload->makeCurrent();
				while (!stopping)
				{
					while (assets->processCustomThread(EngineGraphicsUploadThread::threadIndex));
					threadSleep(10000);
				}
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(EngineGraphicsUploadThread::threadIndex));
					threadSleep(2000);
				}
				windowUpload->makeNotCurrent();
			}
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS

			void initialize(const EngineCreateConfig &config)
			{
				CAGE_ASSERT(engineStarted == 0);
				engineStarted = 1;

				CAGE_LOG(SeverityEnum::Info, "engine", "initializing engine");

				{ // create assets manager
					AssetManagerCreateConfig c;
					if (config.assets)
						c = *config.assets;
					c.schemesMaxCount = max(c.schemesMaxCount, 30u);
					c.threadsMaxCount = max(c.threadsMaxCount, 5u);
					assets = newAssetManager(c);
				}

				{ // create graphics
					window = newWindow();
					window->makeNotCurrent();
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					windowUpload = newWindow(window.get());
					windowUpload->makeNotCurrent();
					windowUpload->setHidden();
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
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
					GuiCreateConfig c;
					if (config.gui)
						c = *config.gui;
					c.assetMgr = +assets;
					gui = newGui(c);
					gui->handleWindowEvents(+window);
				}

				{ // create entities
					entities = newEntityManager();
				}

				{ // create sync objects
					threadsStateBarier = newBarrier(4);
					graphicsSemaphore1 = newSemaphore(1, 1);
					graphicsSemaphore2 = newSemaphore(0, 1);
				}

				{ // create threads
					graphicsDispatchThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsDispatchEntry>(this), "engine graphics dispatch");
					graphicsPrepareThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsPrepareEntry>(this), "engine graphics prepare");
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					graphicsUploadThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::graphicsUploadEntry>(this), "engine graphics upload");
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					soundThreadHolder = newThread(Delegate<void()>().bind<EngineData, &EngineData::soundEntry>(this), "engine sound");
				}

				{ // initialize asset schemes
					// core assets
					assets->defineScheme<AssetSchemeIndexPack, AssetPack>(genAssetSchemePack());
					assets->defineScheme<AssetSchemeIndexRaw, MemoryBuffer>(genAssetSchemeRaw());
					assets->defineScheme<AssetSchemeIndexTextPack, TextPack>(genAssetSchemeTextPack());
					assets->defineScheme<AssetSchemeIndexCollider, Collider>(genAssetSchemeCollider());
					assets->defineScheme<AssetSchemeIndexSkeletonRig, SkeletonRig>(genAssetSchemeSkeletonRig());
					assets->defineScheme<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(genAssetSchemeSkeletalAnimation());
					// engine assets
					assets->defineScheme<AssetSchemeIndexShaderProgram, ShaderProgram>(genAssetSchemeShaderProgram(EngineGraphicsUploadThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexTexture, Texture>(genAssetSchemeTexture(EngineGraphicsUploadThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexModel, Model>(genAssetSchemeModel(EngineGraphicsDispatchThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexRenderObject, RenderObject>(genAssetSchemeRenderObject());
					assets->defineScheme<AssetSchemeIndexFont, Font>(genAssetSchemeFont(EngineGraphicsUploadThread::threadIndex));
					assets->defineScheme<AssetSchemeIndexSound, Sound>(genAssetSchemeSound(EngineSoundThread::threadIndex));
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
					TransformComponent::component = entityMgr->defineComponent(TransformComponent());
					TransformComponent::componentHistory = entityMgr->defineComponent(TransformComponent());
					RenderComponent::component = entityMgr->defineComponent(RenderComponent());
					TextureAnimationComponent::component = entityMgr->defineComponent(TextureAnimationComponent());
					SkeletalAnimationComponent::component = entityMgr->defineComponent(SkeletalAnimationComponent());
					LightComponent::component = entityMgr->defineComponent(LightComponent());
					ShadowmapComponent::component = entityMgr->defineComponent(ShadowmapComponent());
					TextComponent::component = entityMgr->defineComponent(TextComponent());
					CameraComponent::component = entityMgr->defineComponent(CameraComponent());
					SoundComponent::component = entityMgr->defineComponent(SoundComponent());
					ListenerComponent::component = entityMgr->defineComponent(ListenerComponent());
				}

				{ ScopeLock l(threadsStateBarier); }
				CAGE_LOG(SeverityEnum::Info, "engine", "engine initialized");

				CAGE_ASSERT(engineStarted == 1);
				engineStarted = 2;
			}

			void start()
			{
				OPTICK_THREAD("engine control");
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

				{ // release resources hold by gui
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
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					graphicsUploadThreadHolder->wait();
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
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
					window.clear();
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					if (windowUpload)
						windowUpload->makeCurrent();
					windowUpload.clear();
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
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

			graphicsDispatchCreate(config);
			graphicsPrepareCreate(config);
			soundCreate(config);

			controlScheduler = newScheduler({});
			{
				ScheduleCreateConfig c;
				c.name = "engine control update";
				c.action = Delegate<void()>().bind<EngineData, &EngineData::controlUpdate>(this);
				c.period = 1000000 / 20;
				c.type = ScheduleTypeEnum::SteadyPeriodic;
				controlUpdateSchedule = controlScheduler->newSchedule(c);
			}
			{
				ScheduleCreateConfig c;
				c.name = "engine control inputs";
				c.action = Delegate<void()>().bind<EngineData, &EngineData::controlInputs>(this);
				c.period = 1000000 / 60;
				c.type = ScheduleTypeEnum::FreePeriodic;
				controlInputSchedule = controlScheduler->newSchedule(c);
			}

			soundScheduler = newScheduler({});
			{
				ScheduleCreateConfig c;
				c.name = "engine sound update";
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
			graphicsPrepareDestroy();
			graphicsDispatchDestroy();
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
		engineData = systemArena().createHolder<EngineData>(config);
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
		return engineData->assets.get();
	}

	EntityManager *engineEntities()
	{
		return engineData->entities.get();
	}

	Window *engineWindow()
	{
		return engineData->window.get();
	}

	Gui *engineGui()
	{
		return engineData->gui.get();
	}

	Speaker *engineSpeaker()
	{
		return engineData->speaker.get();
	}

	VoicesMixer *engineMasterMixer()
	{
		return engineData->masterBus.get();
	}

	VoicesMixer *engineEffectsMixer()
	{
		return engineData->effectsBus.get();
	}

	VoicesMixer *engineGuiMixer()
	{
		return engineData->guiBus.get();
	}

	uint64 engineControlTime()
	{
		return engineData->controlTime;
	}

	uint64 engineProfilingValues(EngineProfilingStatsFlags flags, EngineProfilingModeEnum mode)
	{
		uint64 result = 0;

		const auto &add = [&](const auto &buffer)
		{
			switch (mode)
			{
			case EngineProfilingModeEnum::Average: result += buffer.smooth(); break;
			case EngineProfilingModeEnum::Maximum: result += buffer.max(); break;
			case EngineProfilingModeEnum::Last: result += buffer.current(); break;
			default: CAGE_THROW_CRITICAL(Exception, "invalid profiling mode enum");
			}
		};

		if (any(flags & EngineProfilingStatsFlags::Control))
			add(engineData->controlUpdateSchedule->statsDuration());
		if (any(flags & EngineProfilingStatsFlags::Sound))
			add(engineData->soundUpdateSchedule->statsDuration());

#define GCHL_GENERATE(NAME) \
		if (any(flags & EngineProfilingStatsFlags::NAME)) \
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
