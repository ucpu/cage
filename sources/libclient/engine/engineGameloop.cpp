#include <atomic>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h> // getExecutableName
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/collider.h> // for sizeof in defineScheme
#include <cage-core/utility/textPack.h> // for sizeof in defineScheme
#include <cage-core/utility/memoryBuffer.h> // for sizeof in defineScheme
#include <cage-core/utility/threadPool.h>
#include <cage-core/utility/variableSmoothingBuffer.h>
#include <cage-core/utility/swapBufferController.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include <cage-client/sound.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/utility/engineProfiling.h>

#include "engine.h"

namespace cage
{
	namespace
	{
		struct scopedSemaphores
		{
			scopedSemaphores(holder<semaphoreClass> &lock, holder<semaphoreClass> &unlock) : sem(unlock.get())
			{
				lock->lock();
			}

			scopedSemaphores(holder<semaphoreClass> &unlock) : sem(unlock.get())
			{}

			~scopedSemaphores()
			{
				sem->unlock();
			}

		private:
			semaphoreClass *sem;
		};

		struct engineDataStruct
		{
			variableSmoothingBufferStruct<uint64, 60> profilingBufferControlTick;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferControlEmit;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferControlSleep;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsPrepareWait;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsPrepareTick;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsDispatchWait;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsDispatchTick;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsDispatchSwap;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsDrawCalls;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferGraphicsDrawPrimitives;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferSoundWait;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferSoundTick;
			variableSmoothingBufferStruct<uint64, 60> profilingBufferSoundSleep;

			holder<assetManagerClass> assets;
			holder<windowClass> window;
			holder<soundContextClass> sound;
			holder<speakerClass> speaker;
			holder<busClass> masterBus;
			holder<busClass> musicBus;
			holder<busClass> effectsBus;
			holder<busClass> guiBus;
			holder<guiClass> gui;
			holder<entityManagerClass> entities;

			holder<mutexClass> assetsSoundMutex;
			holder<mutexClass> assetsGraphicsMutex;
			holder<semaphoreClass> graphicsSemaphore1;
			holder<semaphoreClass> graphicsSemaphore2;
			holder<barrierClass> threadsStateBarier;
			holder<threadClass> graphicsDispatchThreadHolder;
			holder<threadClass> graphicsPrepareThreadHolder;
			holder<threadClass> soundThreadHolder;
			holder<threadPoolClass> emitThreadsHolder;

			std::atomic<uint32> engineStarted;
			std::atomic<bool> stopping;
			uint64 currentControlTime;
			uint64 currentSoundTime;
			uint32 assetSyncAttempts;

			engineDataStruct(const engineCreateConfig &config);

			~engineDataStruct();

			//////////////////////////////////////
			// graphics PREPARE
			//////////////////////////////////////

			void graphicsPrepareInitializeStage()
			{}

			void graphicsPrepareGameloopStage()
			{
				scopedSemaphores leaveLock(graphicsSemaphore2);
				uint64 time1, time2, time3;
				while (!stopping)
				{
					time1 = getApplicationTime();
					{
						scopedSemaphores lockDispatch(graphicsSemaphore1, graphicsSemaphore2);
						scopeLock<mutexClass> lockAssets(assetsGraphicsMutex);
						time2 = getApplicationTime();
						assets->processCustomThread(graphicsPrepareThread().threadIndex);
						graphicsPrepareThread().prepare.dispatch();
						graphicsPrepareTick(time2);
					}
					time3 = getApplicationTime();
					profilingBufferGraphicsPrepareWait.add(time2 - time1);
					profilingBufferGraphicsPrepareTick.add(time3 - time2);
				}
			}

			void graphicsPrepareFinalizeStage()
			{
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(graphicsPrepareThread().threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// graphics DISPATCH
			//////////////////////////////////////

			void graphicsDispatchInitializeStage()
			{
				window->makeCurrent();
				gui->graphicsInitialize(window.get());
				graphicsDispatchInitialize();
			}

			void graphicsDispatchGameloopStage()
			{
				scopedSemaphores leaveLock(graphicsSemaphore1);
				uint64 time1, time2, time3, time4;
				while (!stopping)
				{
					time1 = getApplicationTime();
					{
						scopedSemaphores lockDispatch(graphicsSemaphore2, graphicsSemaphore1);
						time2 = getApplicationTime();
						assets->processCustomThread(graphicsDispatchThreadClass::threadIndex);
						graphicsDispatchThread().render.dispatch();
						uint32 drawCalls = 0;
						uint32 drawPrimitives = 0;
						graphicsDispatchTick(drawCalls, drawPrimitives);
						profilingBufferGraphicsDrawCalls.add(drawCalls);
						profilingBufferGraphicsDrawPrimitives.add(drawPrimitives);
						if (graphicsPrepareThread().stereoMode == stereoModeEnum::Mono)
						{
							gui->graphicsRender();
							CAGE_CHECK_GL_ERROR_DEBUG();
						}
					}
					time3 = getApplicationTime();
					graphicsDispatchThread().swap.dispatch();
					graphicsDispatchSwap();
					time4 = getApplicationTime();
					profilingBufferGraphicsDispatchWait.add(time2 - time1);
					profilingBufferGraphicsDispatchTick.add(time3 - time2);
					profilingBufferGraphicsDispatchSwap.add(time4 - time3);
				}
			}

			void graphicsDispatchFinalizeStage()
			{
				gui->graphicsFinalize();
				graphicsDispatchFinalize();
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(graphicsDispatchThread().threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// SOUND
			//////////////////////////////////////

			void soundInitializeStage()
			{
				//gui->soundInitialize(sound.get());
			}

			void soundTiming(uint64 timeDelay)
			{
				if (timeDelay > soundThread().timePerTick * 2)
				{
					uint64 skip = timeDelay / soundThread().timePerTick + 1;
					CAGE_LOG(severityEnum::Warning, "engine", string() + "skipping " + skip + " sound ticks");
					currentSoundTime += skip * soundThread().timePerTick;
				}
				else
				{
					if (timeDelay < soundThread().timePerTick)
						threadSleep(soundThread().timePerTick - timeDelay);
					currentSoundTime += soundThread().timePerTick;
				}
			}

			void soundGameloopStage()
			{
				uint64 time1, time2, time3, time4;
				while (!stopping)
				{
					time1 = getApplicationTime();
					{
						scopeLock<mutexClass> lockAssets(assetsSoundMutex);
						time2 = getApplicationTime();
						assets->processCustomThread(soundThreadClass::threadIndex);
						soundThread().sound.dispatch();
						soundTick(currentSoundTime);
					}
					time3 = getApplicationTime();
					soundTiming(time3 > currentSoundTime ? time3 - currentSoundTime : 0);
					time4 = getApplicationTime();
					profilingBufferSoundWait.add(time2 - time1);
					profilingBufferSoundTick.add(time3 - time2);
					profilingBufferSoundSleep.add(time4 - time3);
				}
			}

			void soundFinalizeStage()
			{
				soundFinalize();
				//gui->soundFinalize();
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(soundThread().threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void controlTryAssets()
			{
				assetSyncAttempts++;
				scopeLock<mutexClass> lockGraphics(assetsGraphicsMutex, assetSyncAttempts < 30);
				if (lockGraphics)
				{
					scopeLock<mutexClass> lockSound(assetsSoundMutex, assetSyncAttempts < 15);
					if (lockSound)
					{
						controlThread().assets.dispatch();
						while (assets->processControlThread() || assets->processCustomThread(controlThreadClass::threadIndex));
						assetSyncAttempts = 0;
					}
				}
			}

			void updateHistoryComponents()
			{
				for (entityClass *e : transformComponent::component->getComponentEntities()->entities())
				{
					ENGINE_GET_COMPONENT(transform, ts, e);
					transformComponent &hs = e->value<transformComponent>(transformComponent::componentHistory);
					hs = ts;
					if (e->hasComponent(configuredSkeletonComponent::component))
					{
						ENGINE_GET_COMPONENT(configuredSkeleton, cs, e);
						configuredSkeletonComponent &hs = e->value<configuredSkeletonComponent>(configuredSkeletonComponent::componentHistory);
						hs = cs;
					}
				}
			}

			void emitThreadsEntry(uint32 index, uint32)
			{
				switch (index)
				{
				case 0:
					soundEmit(currentSoundTime);
					break;
				case 1:
					gui->controlUpdateDone(); // guiEmit
					break;
				case 2:
					graphicsPrepareEmit(currentControlTime);
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid engine emit thread index");
				}
			}

			void controlTiming(uint64 timeDelay)
			{
				if (timeDelay > controlThread().timePerTick * 2)
				{
					uint64 skip = timeDelay / controlThread().timePerTick + 1;
					CAGE_LOG(severityEnum::Warning, "engine", string() + "skipping " + skip + " control update ticks");
					currentControlTime += skip * controlThread().timePerTick;
				}
				else
				{
					if (timeDelay < controlThread().timePerTick)
						threadSleep(controlThread().timePerTick - timeDelay);
					currentControlTime += controlThread().timePerTick;
				}
			}

			void controlGameloopStage()
			{
				while (!stopping)
				{
					uint64 time1 = getApplicationTime();
					controlTryAssets();
					updateHistoryComponents();
					gui->setOutputResolution(window->resolution());
					gui->controlUpdateStart();
					window->processEvents();
					controlThread().update.dispatch();
					uint64 time2 = getApplicationTime();
					emitThreadsHolder->run();
					uint64 time3 = getApplicationTime();
					controlTiming(time3 > currentControlTime ? time3 - currentControlTime : 0);
					uint64 time4 = getApplicationTime();
					profilingBufferControlTick.add(time2 - time1);
					profilingBufferControlEmit.add(time3 - time2);
					profilingBufferControlSleep.add(time4 - time3);
				}
			}

			//////////////////////////////////////
			// ENTRY methods
			//////////////////////////////////////

#define GCHL_GENERATE_ENTRY(NAME) \
			void CAGE_JOIN(NAME, Entry)() \
			{ \
				try { CAGE_JOIN(NAME, InitializeStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (engine) in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().initialize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, GameloopStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in gameloop in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().finalize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (application) in " CAGE_STRINGIZE(NAME)); } \
				try { CAGE_JOIN(NAME, FinalizeStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (engine) in " CAGE_STRINGIZE(NAME)); } \
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphicsPrepare, graphicsDispatch, sound));
#undef GCHL_GENERATE_ENTRY

			void initialize(const engineCreateConfig &config)
			{
				CAGE_ASSERT_RUNTIME(engineStarted == 0);
				engineStarted = 1;

				CAGE_LOG(severityEnum::Info, "engine", "initializing engine");

				{ // create assets manager
					assetManagerCreateConfig c;
					if (config.assets)
						c = *config.assets;
					c.schemeMaxCount = max(c.schemeMaxCount, 30u);
					c.threadMaxCount = max(c.threadMaxCount, 5u);
					assets = newAssetManager(c);
				}

				{ // create graphics
					window = newWindow();
					window->makeNotCurrent();
				}

				{ // create sound
					string name = pathExtractFilename(detail::getExecutableFullPathNoExe());
					sound = newSoundContext(config.soundContext ? *config.soundContext : soundContextCreateConfig(), name);
					speaker = newSpeaker(sound.get(), config.speaker ? *config.speaker : speakerCreateConfig(), name);
					masterBus = newBus(sound.get());
					musicBus = newBus(sound.get());
					effectsBus = newBus(sound.get());
					guiBus = newBus(sound.get());
					speaker->setInput(masterBus.get());
					masterBus->addInput(musicBus.get());
					masterBus->addInput(effectsBus.get());
					masterBus->addInput(guiBus.get());
				}

				{ // create gui
					guiCreateConfig c;
					if (config.gui)
						c = *config.gui;
					c.assetManager = assets.get();
					gui = newGui(c);
					gui->handleWindowEvents(window.get());
					gui->setOutputSoundBus(guiBus.get());
				}

				{ // create entities
					entities = newEntityManager(config.entities ? *config.entities : entityManagerCreateConfig());
				}

				{ // create sync objects
					threadsStateBarier = newBarrier(4);
					assetsSoundMutex = newMutex();
					assetsGraphicsMutex = newMutex();
					graphicsSemaphore1 = newSemaphore(1, 1);
					graphicsSemaphore2 = newSemaphore(0, 1);
				}

				{ // create threads
					graphicsDispatchThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicsDispatchEntry>(this), "engine graphics dispatch");
					graphicsPrepareThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicsPrepareEntry>(this), "engine graphics prepare");
					soundThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::soundEntry>(this), "engine sound");
					emitThreadsHolder = newThreadPool("engine emit ", 3);
					emitThreadsHolder->function.bind<engineDataStruct, &engineDataStruct::emitThreadsEntry>(this);
				}

				{ // initialize asset schemes
					// core assets
					assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(controlThread().threadIndex));
					assets->defineScheme<memoryBuffer>(assetSchemeIndexRaw, genAssetSchemeRaw(controlThread().threadIndex));
					assets->defineScheme<textPackClass>(assetSchemeIndexTextPackage, genAssetSchemeTextPackage(controlThread().threadIndex));
					assets->defineScheme<colliderClass>(assetSchemeIndexCollider, genAssetSchemeCollider(controlThread().threadIndex));
					// client assets
					assets->defineScheme<shaderClass>(assetSchemeIndexShader, genAssetSchemeShader(graphicsDispatchThread().threadIndex, window.get()));
					assets->defineScheme<textureClass>(assetSchemeIndexTexture, genAssetSchemeTexture(graphicsDispatchThread().threadIndex, window.get()));
					assets->defineScheme<meshClass>(assetSchemeIndexMesh, genAssetSchemeMesh(graphicsDispatchThread().threadIndex, window.get()));
					assets->defineScheme<skeletonClass>(assetSchemeIndexSkeleton, genAssetSchemeSkeleton(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<animationClass>(assetSchemeIndexAnimation, genAssetSchemeAnimation(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<objectClass>(assetSchemeIndexObject, genAssetSchemeObject(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<fontClass>(assetSchemeIndexFont, genAssetSchemeFont(graphicsDispatchThreadClass::threadIndex, window.get()));
					assets->defineScheme<sourceClass>(assetSchemeIndexSound, genAssetSchemeSound(soundThreadClass::threadIndex, sound.get()));
					// cage pack
					assets->add(hashString("cage/cage.pack"));
				}

				{ // initialize assets change listening
					static configBool confAutoAssetListen("cage-client.engine.autoAssetListen", false);
					if (confAutoAssetListen)
					{
						CAGE_LOG(severityEnum::Info, "assets", "assets updates listening enabled");
						detail::overrideBreakpoint overrideBreakpoint;
						detail::overrideException overrideException;
						try
						{
							assets->listen("localhost", 65042);
						}
						catch (const exception &)
						{
							CAGE_LOG(severityEnum::Warning, "assets", "assets updates listening failed");
						}
					}
				}

				{ // initialize entity components
					entityManagerClass *entityManager = entities.get();
					transformComponent::component = entityManager->defineComponent(transformComponent(), true);
					transformComponent::componentHistory = entityManager->defineComponent(transformComponent(), false);
					renderComponent::component = entityManager->defineComponent(renderComponent(), true);
					animatedTextureComponent::component = entityManager->defineComponent(animatedTextureComponent(), false);
					animatedSkeletonComponent::component = entityManager->defineComponent(animatedSkeletonComponent(), false);
					configuredSkeletonComponent::component = entityManager->defineComponent(configuredSkeletonComponent(), false);
					configuredSkeletonComponent::componentHistory = entityManager->defineComponent(configuredSkeletonComponent(), false);
					lightComponent::component = entityManager->defineComponent(lightComponent(), true);
					shadowmapComponent::component = entityManager->defineComponent(shadowmapComponent(), false);
					cameraComponent::component = entityManager->defineComponent(cameraComponent(), true);
					voiceComponent::component = entityManager->defineComponent(voiceComponent(), true);
					listenerComponent::component = entityManager->defineComponent(listenerComponent(), true);
				}

				{ scopeLock<barrierClass> l(threadsStateBarier); }
				CAGE_LOG(severityEnum::Info, "engine", "engine initialized");

				CAGE_ASSERT_RUNTIME(engineStarted == 1);
				engineStarted = 2;
			}

			void start()
			{
				CAGE_ASSERT_RUNTIME(engineStarted == 2);
				engineStarted = 3;

				try
				{
					controlThread().initialize.dispatch();
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in control");
					engineStop();
				}

				CAGE_LOG(severityEnum::Info, "engine", "starting engine");

				currentControlTime = currentSoundTime = getApplicationTime();

				{ scopeLock<barrierClass> l(threadsStateBarier); }
				{ scopeLock<barrierClass> l(threadsStateBarier); }

				try
				{
					controlGameloopStage();
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in gameloop in control");
					engineStop();
				}

				CAGE_LOG(severityEnum::Info, "engine", "engine stopped");

				try
				{
					controlThread().finalize.dispatch();
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (application) in control");
				}

				CAGE_ASSERT_RUNTIME(engineStarted == 3);
				engineStarted = 4;
			}

			void finalize()
			{
				CAGE_ASSERT_RUNTIME(engineStarted == 4);
				engineStarted = 5;

				CAGE_LOG(severityEnum::Info, "engine", "finalizing engine");
				{ scopeLock<barrierClass> l(threadsStateBarier); }

				{ // unload assets
					assets->remove(hashString("cage/cage.pack"));
					while (assets->countTotal() > 0)
					{
						controlThread().assets.dispatch();
						while (assets->processCustomThread(controlThread().threadIndex) || assets->processControlThread());
						threadSleep(5000);
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
					effectsBus.clear();
					musicBus.clear();
					guiBus.clear();
					masterBus.clear();
					speaker.clear();
					sound.clear();
				}

				{ // destroy graphics
					if (window)
						window->makeCurrent();
					window.clear();
				}

				{ // destroy assets
					assets.clear();
				}

				CAGE_LOG(severityEnum::Info, "engine", "engine finalized");

				CAGE_ASSERT_RUNTIME(engineStarted == 5);
				engineStarted = 6;
			}
		};

		holder<engineDataStruct> engineData;

		engineDataStruct::engineDataStruct(const engineCreateConfig &config) : engineStarted(0), stopping(false), currentControlTime(0), currentSoundTime(0), assetSyncAttempts(0)
		{
			CAGE_LOG(severityEnum::Info, "engine", "creating engine");
			graphicsDispatchCreate(config);
			graphicsPrepareCreate(config);
			soundCreate(config);
			CAGE_LOG(severityEnum::Info, "engine", "engine created");
		}

		engineDataStruct::~engineDataStruct()
		{
			CAGE_LOG(severityEnum::Info, "engine", "destroying engine");
			soundDestroy();
			graphicsPrepareDestroy();
			graphicsDispatchDestroy();
			CAGE_LOG(severityEnum::Info, "engine", "engine destroyed");
		}
	}

	void engineInitialize(const engineCreateConfig &config)
	{
		CAGE_ASSERT_RUNTIME(!engineData);
		engineData = detail::systemArena().createHolder<engineDataStruct>(config);
		engineData->initialize(config);
	}

	void engineStart()
	{
		CAGE_ASSERT_RUNTIME(engineData);
		engineData->start();
	}

	void engineStop()
	{
		CAGE_ASSERT_RUNTIME(engineData);
		if (engineData->stopping)
			return;
		CAGE_LOG(severityEnum::Info, "engine", "stopping engine");
		engineData->stopping = true;
	}

	void engineFinalize()
	{
		CAGE_ASSERT_RUNTIME(engineData);
		engineData->finalize();
		engineData.clear();
	}

	soundContextClass *sound()
	{
		return engineData->sound.get();
	}

	assetManagerClass *assets()
	{
		return engineData->assets.get();
	}

	entityManagerClass *entities()
	{
		return engineData->entities.get();
	}

	windowClass *window()
	{
		return engineData->window.get();
	}

	guiClass *gui()
	{
		return engineData->gui.get();
	}

	speakerClass *speaker()
	{
		return engineData->speaker.get();
	}

	busClass *masterMixer()
	{
		return engineData->masterBus.get();
	}

	busClass *musicMixer()
	{
		return engineData->musicBus.get();
	}

	busClass *effectsMixer()
	{
		return engineData->effectsBus.get();
	}

	busClass *guiMixer()
	{
		return engineData->guiBus.get();
	}

	uint64 currentControlTime()
	{
		return engineData->currentControlTime;
	}

	uint64 engineProfilingValues(engineProfilingStatsFlags flags, engineProfilingModeEnum mode)
	{
		uint64 result = 0;
#define GCHL_GENERATE(NAME) \
		if ((flags & engineProfilingStatsFlags::NAME) == engineProfilingStatsFlags::NAME) \
		{ \
			auto &buffer = CAGE_JOIN(engineData->profilingBuffer, NAME); \
			switch (mode) \
			{ \
			case engineProfilingModeEnum::Average: result += buffer.smooth(); break; \
			case engineProfilingModeEnum::Maximum: result += buffer.max(); break; \
			case engineProfilingModeEnum::Last: result += buffer.last(); break; \
			default: CAGE_THROW_CRITICAL(exception, "invalid profiling mode enum"); \
			} \
		}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE,
			ControlTick,
			ControlEmit,
			ControlSleep,
			GraphicsPrepareWait,
			GraphicsPrepareTick,
			GraphicsDispatchWait,
			GraphicsDispatchTick,
			GraphicsDispatchSwap,
			GraphicsDrawCalls,
			GraphicsDrawPrimitives,
			SoundWait,
			SoundTick,
			SoundSleep
		));
#undef GCHL_GENERATE
		return result;
	}
}
