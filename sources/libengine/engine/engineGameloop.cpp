#include <atomic>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h> // getExecutableName
#include <cage-core/assetStructs.h>
#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/collisionMesh.h> // for sizeof in defineScheme
#include <cage-core/textPack.h> // for sizeof in defineScheme
#include <cage-core/memoryBuffer.h> // for sizeof in defineScheme
#include <cage-core/threadPool.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-core/swapBufferGuard.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include <cage-engine/sound.h>
#include <cage-engine/gui.h>
#include <cage-engine/engine.h>
#include <cage-engine/engineProfiling.h>

#include "engine.h"

//#define CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS

namespace cage
{
	namespace
	{
		configBool confAutoAssetListen("cage-engine.engine.assetsListen", false);
		configUint32 confOptickFrameMode("cage-engine.optick.frameMode", 1);

		struct graphicsUploadThreadClass
		{
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			static const uint32 threadIndex = 2;
#else
			static const uint32 threadIndex = graphicsDispatchThreadClass::threadIndex;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
		};

		struct scopedSemaphores
		{
			scopedSemaphores(holder<syncSemaphore> &lock, holder<syncSemaphore> &unlock) : sem(unlock.get())
			{
				lock->lock();
			}

			~scopedSemaphores()
			{
				sem->unlock();
			}

		private:
			syncSemaphore *sem;
		};

		struct scopedTimer
		{
			variableSmoothingBuffer<uint64, 60> &vsb;
			uint64 st;

			scopedTimer(variableSmoothingBuffer<uint64, 60> &vsb) : vsb(vsb), st(getApplicationTime())
			{}

			~scopedTimer()
			{
				uint64 et = getApplicationTime();
				vsb.add(et - st);
			}
		};

		struct engineDataStruct
		{
			variableSmoothingBuffer<uint64, 60> profilingBufferControl;
			variableSmoothingBuffer<uint64, 60> profilingBufferSound;
			variableSmoothingBuffer<uint64, 60> profilingBufferGraphicsPrepare;
			variableSmoothingBuffer<uint64, 60> profilingBufferGraphicsDispatch;
			variableSmoothingBuffer<uint64, 60> profilingBufferFrameTime;
			variableSmoothingBuffer<uint64, 60> profilingBufferDrawCalls;
			variableSmoothingBuffer<uint64, 60> profilingBufferDrawPrimitives;
			variableSmoothingBuffer<uint64, 60> profilingBufferEntities;

			holder<assetManager> assets;
			holder<windowHandle> window;
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			holder<windowHandle> windowUpload;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			holder<soundContext> sound;
			holder<speakerOutput> speaker;
			holder<mixingBus> masterBus;
			holder<mixingBus> musicBus;
			holder<mixingBus> effectsBus;
			holder<mixingBus> guiBus;
			holder<guiManager> gui;
			holder<entityManager> entities;

			holder<syncMutex> assetsSoundMutex;
			holder<syncMutex> assetsGraphicsMutex;
			holder<syncSemaphore> graphicsSemaphore1;
			holder<syncSemaphore> graphicsSemaphore2;
			holder<syncBarrier> threadsStateBarier;
			holder<threadHandle> graphicsDispatchThreadHolder;
			holder<threadHandle> graphicsPrepareThreadHolder;
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			holder<threadHandle> graphicsUploadThreadHolder;
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			holder<threadHandle> soundThreadHolder;
			holder<threadPool> emitThreadsHolder;

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

			void graphicsPrepareStep()
			{
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(graphicsPrepareThread().threadIndex);
				}
				scopedSemaphores lockGraphics(graphicsSemaphore1, graphicsSemaphore2);
				scopeLock<syncMutex> lockAssets(assetsGraphicsMutex);
				scopedTimer timing(profilingBufferGraphicsPrepare);
				{
					OPTICK_EVENT("callback");
					graphicsPrepareThread().prepare.dispatch();
				}
				{
					OPTICK_EVENT("tick");
					graphicsPrepareTick(getApplicationTime());
				}
			}

			void graphicsPrepareGameloopStage()
			{
				while (!stopping)
				{
					if (confOptickFrameMode == graphicsPrepareThreadClass::threadIndex)
					{
						OPTICK_FRAME("engine graphics prepare");
						graphicsPrepareStep();
					}
					else
					{
						OPTICK_EVENT("engine graphics prepare");
						graphicsPrepareStep();
					}
				}
			}

			void graphicsPrepareStopStage()
			{
				graphicsSemaphore2->unlock();
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
				gui->graphicsInitialize();
				graphicsDispatchInitialize();
			}

			void graphicsDispatchStep()
			{
				scopedTimer timing(profilingBufferFrameTime);
				{
					scopedSemaphores lockGraphics(graphicsSemaphore2, graphicsSemaphore1);
					scopedTimer timing(profilingBufferGraphicsDispatch);
					{
						OPTICK_EVENT("render callback");
						graphicsDispatchThread().render.dispatch();
					}
					{
						OPTICK_EVENT("tick");
						uint32 drawCalls = 0, drawPrimitives = 0;
						graphicsDispatchTick(drawCalls, drawPrimitives);
						profilingBufferDrawCalls.add(drawCalls);
						profilingBufferDrawPrimitives.add(drawPrimitives);
						OPTICK_TAG("drawCalls", drawCalls);
						OPTICK_TAG("drawPrimitives", drawPrimitives);
					}
				}
				if (graphicsPrepareThread().stereoMode == stereoModeEnum::Mono)
				{
					OPTICK_EVENT("gui render");
					gui->graphicsRender();
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(graphicsDispatchThreadClass::threadIndex);
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
				{
					if (confOptickFrameMode == graphicsDispatchThreadClass::threadIndex)
					{
						OPTICK_FRAME("engine graphics dispatch");
						graphicsDispatchStep();
					}
					else
					{
						OPTICK_EVENT("engine graphics dispatch");
						graphicsDispatchStep();
					}
				}
			}

			void graphicsDispatchStopStage()
			{
				graphicsSemaphore1->unlock();
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
					{
						OPTICK_EVENT("sleep");
						threadSleep(soundThread().timePerTick - timeDelay);
					}
					currentSoundTime += soundThread().timePerTick;
				}
			}

			void soundStep()
			{
				{
					scopeLock<syncMutex> lockAssets(assetsSoundMutex);
					scopedTimer timing(profilingBufferSound);
					{
						OPTICK_EVENT("sound callback");
						soundThread().sound.dispatch();
					}
					{
						OPTICK_EVENT("tick");
						soundTick(currentSoundTime);
					}
				}
				{
					OPTICK_EVENT("assets");
					assets->processCustomThread(soundThreadClass::threadIndex);
				}
				uint64 newTime = getApplicationTime();
				soundTiming(newTime > currentSoundTime ? newTime - currentSoundTime : 0);
			}

			void soundGameloopStage()
			{
				while (!stopping)
				{
					if (confOptickFrameMode == soundThreadClass::threadIndex)
					{
						OPTICK_FRAME("engine sound");
						soundStep();
					}
					else
					{
						OPTICK_EVENT("engine sound");
						soundStep();
					}
				}
			}

			void soundStopStage()
			{
				// nothing
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

			void controlAssets()
			{
				OPTICK_EVENT("assets");
				{
					assetSyncAttempts++;
					OPTICK_TAG("assetSyncAttempts", assetSyncAttempts);
					scopeLock<syncMutex> lockGraphics(assetsGraphicsMutex, assetSyncAttempts < 20);
					if (lockGraphics)
					{
						scopeLock<syncMutex> lockSound(assetsSoundMutex, assetSyncAttempts < 10);
						if (lockSound)
						{
							{
								OPTICK_EVENT("callback");
								controlThread().assets.dispatch();
							}
							{
								OPTICK_EVENT("control");
								while (assets->processControlThread());
							}
							assetSyncAttempts = 0;
						}
					}
				}
				{
					OPTICK_EVENT("assets");
					while (assets->processCustomThread(controlThreadClass::threadIndex));
				}
			}

			void updateHistoryComponents()
			{
				OPTICK_EVENT("update history");
				for (entity *e : transformComponent::component->entities())
				{
					CAGE_COMPONENT_ENGINE(transform, ts, e);
					transformComponent &hs = e->value<transformComponent>(transformComponent::componentHistory);
					hs = ts;
				}
			}

			void emitThreadsEntry(uint32 index, uint32)
			{
				switch (index)
				{
				case 0:
				{
					OPTICK_EVENT("sound emit");
					soundEmit(currentSoundTime);
				} break;
				case 1:
				{
					OPTICK_EVENT("gui emit");
					gui->controlUpdateDone();
				} break;
				case 2:
				{
					OPTICK_EVENT("graphics emit");
					graphicsPrepareEmit(currentControlTime);
				} break;
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
					{
						OPTICK_EVENT("sleep");
						threadSleep(controlThread().timePerTick - timeDelay);
					}
					currentControlTime += controlThread().timePerTick;
				}
			}

			void controlStep()
			{
				updateHistoryComponents();
				{
					OPTICK_EVENT("gui update");
					gui->setOutputResolution(window->resolution());
					gui->controlUpdateStart();
				}
				{
					OPTICK_EVENT("window events");
					window->processEvents();
				}
				{
					scopedTimer timing(profilingBufferControl);
					OPTICK_EVENT("application update");
					controlThread().update.dispatch();
				}
				{
					OPTICK_EVENT("emit");
					OPTICK_TAG("entitiesCount", entities->group()->count());
					profilingBufferEntities.add(entities->group()->count());
					emitThreadsHolder->run();
				}
				controlAssets();
				uint64 newTime = getApplicationTime();
				controlTiming(newTime > currentControlTime ? newTime - currentControlTime : 0);
			}

			void controlGameloopStage()
			{
				while (!stopping)
				{
					if (confOptickFrameMode == controlThreadClass::threadIndex)
					{
						OPTICK_FRAME("engine control");
						controlStep();
					}
					else
					{
						OPTICK_EVENT("engine control");
						controlStep();
					}
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
				{ scopeLock<syncBarrier> l(threadsStateBarier); } \
				{ scopeLock<syncBarrier> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().initialize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				{ scopeLock<syncBarrier> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, GameloopStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in gameloop in " CAGE_STRINGIZE(NAME)); engineStop(); } \
				CAGE_JOIN(NAME, StopStage)(); \
				{ scopeLock<syncBarrier> l(threadsStateBarier); } \
				try { CAGE_JOIN(NAME, Thread)().finalize.dispatch(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (application) in " CAGE_STRINGIZE(NAME)); } \
				try { CAGE_JOIN(NAME, FinalizeStage)(); } \
				catch (...) { CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (engine) in " CAGE_STRINGIZE(NAME)); } \
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE_ENTRY, graphicsPrepare, graphicsDispatch, sound));
#undef GCHL_GENERATE_ENTRY

#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
			void graphicsUploadEntry()
			{
				windowUpload->makeCurrent();
				while (!stopping)
				{
					while (assets->processCustomThread(graphicsUploadThreadClass::threadIndex));
					threadSleep(5000);
				}
				while (assets->countTotal() > 0)
				{
					while (assets->processCustomThread(graphicsUploadThreadClass::threadIndex));
					threadSleep(5000);
				}
				windowUpload->makeNotCurrent();
			}
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS

			void initialize(const engineCreateConfig &config)
			{
				CAGE_ASSERT(engineStarted == 0);
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
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					windowUpload = newWindow(window.get());
					windowUpload->makeNotCurrent();
					windowUpload->modeSetHidden();
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
				}

				{ // create sound
					string name = pathExtractFilename(detail::getExecutableFullPathNoExe());
					sound = newSoundContext(config.soundContext ? *config.soundContext : soundContextCreateConfig(), name);
					speaker = newSpeakerOutput(sound.get(), config.speaker ? *config.speaker : speakerOutputCreateConfig(), name);
					masterBus = newMixingBus(sound.get());
					musicBus = newMixingBus(sound.get());
					effectsBus = newMixingBus(sound.get());
					guiBus = newMixingBus(sound.get());
					speaker->setInput(masterBus.get());
					masterBus->addInput(musicBus.get());
					masterBus->addInput(effectsBus.get());
					masterBus->addInput(guiBus.get());
				}

				{ // create gui
					guiManagerCreateConfig c;
					if (config.gui)
						c = *config.gui;
					c.assetMgr = assets.get();
					gui = newGuiManager(c);
					gui->handleWindowEvents(window.get());
					gui->setOutputSoundBus(guiBus.get());
				}

				{ // create entities
					entities = newEntityManager(config.entities ? *config.entities : entityManagerCreateConfig());
				}

				{ // create sync objects
					threadsStateBarier = newSyncBarrier(4);
					assetsSoundMutex = newSyncMutex();
					assetsGraphicsMutex = newSyncMutex();
					graphicsSemaphore1 = newSyncSemaphore(1, 1);
					graphicsSemaphore2 = newSyncSemaphore(0, 1);
				}

				{ // create threads
					graphicsDispatchThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicsDispatchEntry>(this), "engine graphics dispatch");
					graphicsPrepareThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicsPrepareEntry>(this), "engine graphics prepare");
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					graphicsUploadThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::graphicsUploadEntry>(this), "engine graphics upload");
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					soundThreadHolder = newThread(delegate<void()>().bind<engineDataStruct, &engineDataStruct::soundEntry>(this), "engine sound");
					emitThreadsHolder = newThreadPool("engine emit ", 3);
					emitThreadsHolder->function.bind<engineDataStruct, &engineDataStruct::emitThreadsEntry>(this);
				}

				{ // initialize asset schemes
					// core assets
					assets->defineScheme<void>(assetSchemeIndexPack, genAssetSchemePack(controlThreadClass::threadIndex));
					assets->defineScheme<memoryBuffer>(assetSchemeIndexRaw, genAssetSchemeRaw(controlThreadClass::threadIndex));
					assets->defineScheme<textPack>(assetSchemeIndexTextPackage, genAssetSchemeTextPackage(controlThreadClass::threadIndex));
					assets->defineScheme<collisionMesh>(assetSchemeIndexCollisionMesh, genAssetSchemeCollisionMesh(controlThreadClass::threadIndex));
					// client assets
					assets->defineScheme<shaderProgram>(assetSchemeIndexShaderProgram, genAssetSchemeShaderProgram(graphicsUploadThreadClass::threadIndex, window.get()));
					assets->defineScheme<renderTexture>(assetSchemeIndexRenderTexture, genAssetSchemeRenderTexture(graphicsUploadThreadClass::threadIndex, window.get()));
					assets->defineScheme<renderMesh>(assetSchemeIndexMesh, genAssetSchemeRenderMesh(graphicsDispatchThreadClass::threadIndex, window.get()));
					assets->defineScheme<skeletonRig>(assetSchemeIndexSkeletonRig, genAssetSchemeSkeletonRig(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<skeletalAnimation>(assetSchemeIndexSkeletalAnimation, genAssetSchemeSkeletalAnimation(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<renderObject>(assetSchemeIndexRenderObject, genAssetSchemeRenderObject(graphicsPrepareThreadClass::threadIndex));
					assets->defineScheme<fontFace>(assetSchemeIndexFontFace, genAssetSchemeFontFace(graphicsUploadThreadClass::threadIndex, window.get()));
					assets->defineScheme<soundSource>(assetSchemeIndexSoundSource, genAssetSchemeSoundSource(soundThreadClass::threadIndex, sound.get()));
					// cage pack
					assets->add(hashString("cage/cage.pack"));
				}

				{ // initialize assets change listening
					if (confAutoAssetListen)
					{
						CAGE_LOG(severityEnum::Info, "assets", "enabling assets updates listening");
						detail::overrideBreakpoint overrideBreakpoint;
						detail::overrideException overrideException;
						try
						{
							assets->listen("localhost", 65042);
						}
						catch (const exception &)
						{
							CAGE_LOG(severityEnum::Warning, "assets", "assets updates failed to connect to the database");
						}
					}
				}

				{ // initialize entity components
					entityManager *entityMgr = entities.get();
					transformComponent::component = entityMgr->defineComponent(transformComponent(), true);
					transformComponent::componentHistory = entityMgr->defineComponent(transformComponent(), false);
					renderComponent::component = entityMgr->defineComponent(renderComponent(), true);
					textureAnimationComponent::component = entityMgr->defineComponent(textureAnimationComponent(), false);
					skeletalAnimationComponent::component = entityMgr->defineComponent(skeletalAnimationComponent(), false);
					lightComponent::component = entityMgr->defineComponent(lightComponent(), true);
					shadowmapComponent::component = entityMgr->defineComponent(shadowmapComponent(), false);
					renderTextComponent::component = entityMgr->defineComponent(renderTextComponent(), true);
					cameraComponent::component = entityMgr->defineComponent(cameraComponent(), true);
					voiceComponent::component = entityMgr->defineComponent(voiceComponent(), true);
					listenerComponent::component = entityMgr->defineComponent(listenerComponent(), true);
				}

				{ scopeLock<syncBarrier> l(threadsStateBarier); }
				CAGE_LOG(severityEnum::Info, "engine", "engine initialized");

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
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "engine", "exception caught in initialization (application) in control");
					engineStop();
				}

				CAGE_LOG(severityEnum::Info, "engine", "starting engine");

				currentControlTime = currentSoundTime = getApplicationTime();

				{ scopeLock<syncBarrier> l(threadsStateBarier); }
				{ scopeLock<syncBarrier> l(threadsStateBarier); }

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

				CAGE_ASSERT(engineStarted == 3);
				engineStarted = 4;
			}

			void finalize()
			{
				CAGE_ASSERT(engineStarted == 4);
				engineStarted = 5;

				CAGE_LOG(severityEnum::Info, "engine", "finalizing engine");
				{ scopeLock<syncBarrier> l(threadsStateBarier); }

				{ // unload assets
					assets->remove(hashString("cage/cage.pack"));
					while (assets->countTotal() > 0)
					{
						try
						{
							controlThread().assets.dispatch();
						}
						catch (...)
						{
							CAGE_LOG(severityEnum::Error, "engine", "exception caught in finalization (unloading assets) in control");
						}
						while (assets->processCustomThread(controlThread().threadIndex) || assets->processControlThread());
						threadSleep(5000);
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
#ifdef CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
					if (windowUpload)
						windowUpload->makeCurrent();
					windowUpload.clear();
#endif // CAGE_USE_SEPARATE_THREAD_FOR_GPU_UPLOADS
				}

				{ // destroy assets
					assets.clear();
				}

				CAGE_LOG(severityEnum::Info, "engine", "engine finalized");

				CAGE_ASSERT(engineStarted == 5);
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
		CAGE_ASSERT(!engineData);
		engineData = detail::systemArena().createHolder<engineDataStruct>(config);
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
			CAGE_LOG(severityEnum::Info, "engine", "stopping engine");
		}
	}

	void engineFinalize()
	{
		CAGE_ASSERT(engineData);
		engineData->finalize();
		engineData.clear();
	}

	soundContext *sound()
	{
		return engineData->sound.get();
	}

	assetManager *assets()
	{
		return engineData->assets.get();
	}

	entityManager *entities()
	{
		return engineData->entities.get();
	}

	windowHandle *window()
	{
		return engineData->window.get();
	}

	guiManager *gui()
	{
		return engineData->gui.get();
	}

	speakerOutput *speaker()
	{
		return engineData->speaker.get();
	}

	mixingBus *masterMixer()
	{
		return engineData->masterBus.get();
	}

	mixingBus *musicMixer()
	{
		return engineData->musicBus.get();
	}

	mixingBus *effectsMixer()
	{
		return engineData->effectsBus.get();
	}

	mixingBus *guiMixer()
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
			case engineProfilingModeEnum::Last: result += buffer.current(); break; \
			default: CAGE_THROW_CRITICAL(exception, "invalid profiling mode enum"); \
			} \
		}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE,
			Control,
			Sound,
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
