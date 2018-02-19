#include <atomic>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h> // getExecutableName
#include <cage-core/assets.h>
#include <cage-core/utility/collider.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/variableSmoothingBuffer.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/textPack.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/sound.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>

#include "engine.h"

namespace cage
{
	namespace
	{
		struct engineDataStruct
		{
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
			holder<barrierClass> threadsStateBarier;
			holder<semaphoreClass> graphicPrepareSemaphore;
			holder<semaphoreClass> graphicDispatchSemaphore;
			holder<semaphoreClass> emitGraphicStartSemaphore;
			holder<semaphoreClass> emitGraphicAssetsSemaphore;
			holder<semaphoreClass> emitGraphicEndSemaphore;
			holder<semaphoreClass> emitSoundStartSemaphore;
			holder<semaphoreClass> emitSoundAssetsSemaphore;
			holder<semaphoreClass> emitSoundEndSemaphore;
			holder<threadClass> graphicDispatchThread;
			holder<threadClass> graphicPrepareThread;
			holder<threadClass> soundThread;
			variableSmoothingBufferStruct<uint64, 64> timeBufferControlTick;
			variableSmoothingBufferStruct<uint64, 64> timeBufferControlWait;
			variableSmoothingBufferStruct<uint64, 64> timeBufferControlEmit;
			variableSmoothingBufferStruct<uint64, 64> timeBufferControlSleep;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicPrepareWait;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicPrepareEmit;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicPrepareTick;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicDispatchWait;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicDispatchTick;
			variableSmoothingBufferStruct<uint64, 64> timeBufferGraphicDispatchSwap;
			variableSmoothingBufferStruct<uint64, 64> timeBufferSoundEmit;
			variableSmoothingBufferStruct<uint64, 64> timeBufferSoundTick;
			variableSmoothingBufferStruct<uint64, 64> timeBufferSoundSleep;
			std::atomic<uint32> engineStarted;
			std::atomic<bool> emitIsReady;
			std::atomic<bool> stopping;
			uint64 currentControlTime;

			engineDataStruct(const engineCreateConfig &config);

			~engineDataStruct();

			//////////////////////////////////////
			// graphic PREPARE
			//////////////////////////////////////

			void graphicPrepareInitializeStage()
			{
				graphicPrepareInitialize();
			}

			void graphicPrepareGameloopStage()
			{
				try
				{
					uint64 lastEmit = 0;
					while (!stopping)
					{
						uint64 time1 = getApplicationTime();
						graphicPrepareSemaphore->lock();
						uint64 time2 = getApplicationTime();
						graphicPrepareThread::prepare.dispatch();
						graphicPrepareTick(lastEmit, time2);
						graphicDispatchSemaphore->unlock();
						while(assets->processCustomThread(graphicPrepareThread::threadIndex));
						uint64 time3 = getApplicationTime();
						if (emitIsReady)
						{
							lastEmit = time2;
							emitGraphicStartSemaphore->unlock();
							graphicPrepareEmit();
							emitGraphicEndSemaphore->unlock();
							emitGraphicAssetsSemaphore->lock();
						}
						uint64 time4 = getApplicationTime();
						timeBufferGraphicPrepareWait.add(time2 - time1);
						timeBufferGraphicPrepareTick.add(time3 - time2);
						timeBufferGraphicPrepareEmit.add(time4 - time3);
					}
				}
				catch (...)
				{
					engineStop();
					emitGraphicStartSemaphore->unlock();
					emitGraphicEndSemaphore->unlock();
					graphicDispatchSemaphore->unlock();
					throw;
				}
				emitGraphicStartSemaphore->unlock();
				emitGraphicEndSemaphore->unlock();
				graphicDispatchSemaphore->unlock();
			}

			void graphicPrepareFinalizeStage()
			{
				graphicPrepareFinalize();
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(graphicPrepareThread::threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// graphic DISPATCH
			//////////////////////////////////////

			void graphicDispatchInitializeStage()
			{
				window->makeCurrent();
				gui->graphicInitialize(window.get());
				graphicDispatchInitialize();
			}

			void graphicDispatchGameloopStage()
			{
				try
				{
					while (!stopping)
					{
						uint64 time1 = getApplicationTime();
						graphicDispatchSemaphore->lock();
						uint64 time2 = getApplicationTime();
						graphicDispatchThread::render.dispatch();
						graphicDispatchTick();
						if (graphicPrepareThread::stereoMode == stereoModeEnum::Mono)
						{
							gui->graphicRender();
							CAGE_CHECK_GL_ERROR_DEBUG();
						}
						graphicPrepareSemaphore->unlock();
						while(assets->processCustomThread(graphicDispatchThread::threadIndex));
						uint64 time3 = getApplicationTime();
						graphicDispatchThread::swap.dispatch();
						graphicDispatchSwap();
						uint64 time4 = getApplicationTime();
						timeBufferGraphicDispatchWait.add(time2 - time1);
						timeBufferGraphicDispatchTick.add(time3 - time2);
						timeBufferGraphicDispatchSwap.add(time4 - time3);
					}
				}
				catch (...)
				{
					engineStop();
					graphicPrepareSemaphore->unlock();
					throw;
				}
				graphicPrepareSemaphore->unlock();
			}

			void graphicDispatchFinalizeStage()
			{
				gui->graphicFinalize();
				graphicDispatchFinalize();
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(graphicDispatchThread::threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// SOUND
			//////////////////////////////////////

			void soundInitializeStage()
			{
				soundInitialize();
				gui->soundInitialize(sound.get());
			}

			void soundGameloopStage()
			{
				try
				{
					uint64 soundTickTime = getApplicationTime();
					uint64 lastEmit = 0;
					while (!stopping)
					{
						uint64 time1 = getApplicationTime();
						if (emitIsReady)
						{
							lastEmit = soundTickTime;
							emitSoundStartSemaphore->unlock();
							soundEmit();
							emitSoundEndSemaphore->unlock();
							emitSoundAssetsSemaphore->lock();
						}
						uint64 time2 = getApplicationTime();
						while(assets->processCustomThread(soundThread::threadIndex));
						soundThread::sound.dispatch();
						soundTick(lastEmit, soundTickTime);
						gui->soundRender();
						uint64 time3 = getApplicationTime();
						uint64 timeDelay = time3 > soundTickTime ? time3 - soundTickTime : 0;
						if (timeDelay > soundThread::timePerTick * 2)
						{
							uint64 skip = timeDelay / soundThread::timePerTick + 1;
							CAGE_LOG(severityEnum::Warning, "engine", string() + "skipping " + skip + " sound ticks");
							soundTickTime += skip * soundThread::timePerTick;
						}
						else
						{
							if (timeDelay < soundThread::timePerTick)
								threadSleep(soundThread::timePerTick - timeDelay);
							soundTickTime += soundThread::timePerTick;
						}
						uint64 time4 = getApplicationTime();
						timeBufferSoundEmit.add(time2 - time1);
						timeBufferSoundTick.add(time3 - time2);
						timeBufferSoundSleep.add(time4 - time3);
					}
				}
				catch (...)
				{
					engineStop();
					emitSoundStartSemaphore->unlock();
					emitSoundEndSemaphore->unlock();
					throw;
				}
				emitSoundStartSemaphore->unlock();
				emitSoundEndSemaphore->unlock();
			}

			void soundFinalizeStage()
			{
				gui->soundFinalize();
				soundFinalize();
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(soundThread::threadIndex));
					threadSleep(5000);
				}
			}

			//////////////////////////////////////
			// CONTROL
			//////////////////////////////////////

			void controlGameloopStage()
			{
				try
				{
					currentControlTime = getApplicationTime();
					while (!stopping)
					{
						uint64 time1 = getApplicationTime();
						{ // update history components
							uint32 cnt = transformComponent::component->getComponentEntities()->entitiesCount();
							entityClass *const *ents = transformComponent::component->getComponentEntities()->entitiesArray();
							for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
							{
								entityClass *e = *i;
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
						controlThread::update.dispatch();
						gui->setOutputResolution(window->resolution());
						gui->controlUpdate();
						window->processEvents();
						uint64 time2 = getApplicationTime();
						// emit
						emitIsReady = true;
						emitGraphicStartSemaphore->lock(); // wait for both other threads to enter the emit state
						emitSoundStartSemaphore->lock();
						emitIsReady = false;
						uint64 time3 = getApplicationTime();
						gui->controlEmit();
						controlThread::assets.dispatch();
						while (assets->processControlThread() || assets->processCustomThread(controlThread::threadIndex));
						emitGraphicAssetsSemaphore->unlock(); // let both other threads know, that assets are updated
						emitSoundAssetsSemaphore->unlock();
						emitGraphicEndSemaphore->lock(); // wait for both other threads to leave the emit state
						emitSoundEndSemaphore->lock();
						uint64 time4 = getApplicationTime();
						{ // timing
							uint64 timeDelay = time3 > currentControlTime ? time3 - currentControlTime : 0;
							if (timeDelay > controlThread::timePerTick * 2)
							{
								uint64 skip = timeDelay / controlThread::timePerTick + 1;
								CAGE_LOG(severityEnum::Warning, "engine", string() + "skipping " + skip + " control update ticks");
								currentControlTime += skip * controlThread::timePerTick;
							}
							else
							{
								if (timeDelay < controlThread::timePerTick)
									threadSleep(controlThread::timePerTick - timeDelay);
								currentControlTime += controlThread::timePerTick;
							}
						}
						uint64 time5 = getApplicationTime();
						timeBufferControlTick.add(time2 - time1);
						timeBufferControlWait.add(time3 - time2);
						timeBufferControlEmit.add(time4 - time3);
						timeBufferControlSleep.add(time5 - time4);
					}
				}
				catch (...)
				{
					engineStop();
					emitGraphicAssetsSemaphore->unlock();
					emitSoundAssetsSemaphore->unlock();
					throw;
				}
				emitGraphicAssetsSemaphore->unlock();
				emitSoundAssetsSemaphore->unlock();
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
				try { CAGE_JOIN(NAME, Thread)::initialize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, GameloopStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in gameloop in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<barrierClass> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)::finalize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (application) in " CAGE_STRINGIZE(NAME)); } \
				try { CAGE_JOIN(NAME, FinalizeStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (engine) in " CAGE_STRINGIZE(NAME)); } \
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphicPrepare, graphicDispatch, sound));
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

				{ // create graphic
					window = newWindow();
					window->makeNotCurrent();
				}

				{ // create sound
					string name = pathExtractFilenameNoExtension(detail::getExecutableName());
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
					graphicPrepareSemaphore = newSemaphore(1, 1);
					graphicDispatchSemaphore = newSemaphore(0, 1);
					emitGraphicStartSemaphore = newSemaphore(0, 1);
					emitGraphicAssetsSemaphore = newSemaphore(0, 1);
					emitGraphicEndSemaphore = newSemaphore(0, 1);
					emitSoundStartSemaphore = newSemaphore(0, 1);
					emitSoundAssetsSemaphore = newSemaphore(0, 1);
					emitSoundEndSemaphore = newSemaphore(0, 1);
				}

				{ // create threads
					graphicDispatchThread = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicDispatchEntry>(this), "engine graphic dispatch");
					graphicPrepareThread = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicPrepareEntry>(this), "engine graphic prepare");
					soundThread = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::soundEntry>(this), "engine sound");
				}

				{ // initialize asset schemes
					// core assets
					assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(controlThread::threadIndex));
					assets->defineScheme<memoryBuffer>(assetSchemeIndexRaw, genAssetSchemeRaw(controlThread::threadIndex));
					assets->defineScheme<textPackClass>(assetSchemeIndexTextPackage, genAssetSchemeTextPackage(controlThread::threadIndex));
					assets->defineScheme<colliderClass>(assetSchemeIndexCollider, genAssetSchemeCollider(controlThread::threadIndex));
					// client assets
					assets->defineScheme<shaderClass>(assetSchemeIndexShader, genAssetSchemeShader(graphicDispatchThread::threadIndex, window.get()));
					assets->defineScheme<textureClass>(assetSchemeIndexTexture, genAssetSchemeTexture(graphicDispatchThread::threadIndex, window.get()));
					assets->defineScheme<meshClass>(assetSchemeIndexMesh, genAssetSchemeMesh(graphicDispatchThread::threadIndex, window.get()));
					assets->defineScheme<skeletonClass>(assetSchemeIndexSkeleton, genAssetSchemeSkeleton(graphicPrepareThread::threadIndex));
					assets->defineScheme<animationClass>(assetSchemeIndexAnimation, genAssetSchemeAnimation(graphicPrepareThread::threadIndex));
					assets->defineScheme<objectClass>(assetSchemeIndexObject, genAssetSchemeObject(graphicPrepareThread::threadIndex));
					assets->defineScheme<fontClass>(assetSchemeIndexFont, genAssetSchemeFont(graphicDispatchThread::threadIndex, window.get()));
					assets->defineScheme<sourceClass>(assetSchemeIndexSound, genAssetSchemeSound(soundThread::threadIndex, sound.get()));
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
			}

			void start()
			{
				CAGE_ASSERT_RUNTIME(engineStarted == 1);
				engineStarted = 2;

				try
				{
					controlThread::initialize.dispatch();
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in control");
					engineStop();
				}

				CAGE_LOG(severityEnum::Info, "engine", "starting engine");

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
					controlThread::finalize.dispatch();
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (application) in control");
				}
			}

			void finalize()
			{
				CAGE_ASSERT_RUNTIME(engineStarted == 2);
				engineStarted = 3;

				CAGE_LOG(severityEnum::Info, "engine", "finalizing engine");
				{ scopeLock<barrierClass> l(threadsStateBarier); }

				{ // unload assets
					assets->remove(hashString("cage/cage.pack"));
					while (assets->countTotal() > 0)
					{
						controlThread::assets.dispatch();
						while (assets->processCustomThread(controlThread::threadIndex) || assets->processControlThread());
						threadSleep(5000);
					}
				}

				{ // wait for threads to finish
					graphicPrepareThread->wait();
					graphicDispatchThread->wait();
					soundThread->wait();
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

				{ // destroy graphic
					if (window)
						window->makeCurrent();
					window.clear();
				}

				{ // destroy assets
					assets.clear();
				}

				CAGE_LOG(severityEnum::Info, "engine", "engine finalized");
			}
		};

		holder<engineDataStruct> engineData;

		engineDataStruct::engineDataStruct(const engineCreateConfig &config) : engineStarted(0), emitIsReady(false), stopping(false), currentControlTime(0)
		{
			CAGE_LOG(severityEnum::Info, "engine", "creating engine");
			graphicDispatchCreate(config);
			graphicPrepareCreate(config);
			soundCreate(config);
			CAGE_LOG(severityEnum::Info, "engine", "engine created");
		}

		engineDataStruct::~engineDataStruct()
		{
			CAGE_LOG(severityEnum::Info, "engine", "destroying engine");
			soundDestroy();
			graphicPrepareDestroy();
			graphicDispatchDestroy();
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

	namespace engineProfiling
	{
		uint64 getTime(profilingTimeFlags flags, bool smooth)
		{
			uint64 result = 0;
#define GCHL_GENERATE(NAME) \
			if ((flags & profilingTimeFlags::NAME) == profilingTimeFlags::NAME) \
			{ \
				auto &buffer = CAGE_JOIN(engineData->timeBuffer, NAME); \
				if (smooth) \
					result += buffer.smooth(); \
				else \
					result += buffer.last(); \
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE,
				ControlTick,
				ControlWait,
				ControlEmit,
				ControlSleep,
				GraphicPrepareWait,
				GraphicPrepareEmit,
				GraphicPrepareTick,
				GraphicDispatchWait,
				GraphicDispatchTick,
				GraphicDispatchSwap,
				SoundEmit,
				SoundTick,
				SoundSleep
			));
#undef GCHL_GENERATE
			return result;
		}
	}
}
