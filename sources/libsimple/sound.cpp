#include <unordered_dense.h>
#include <vector>

#include "engine.h"
#include "interpolationTimingCorrector.h"

#include <cage-core/assetsManager.h>
#include <cage-core/entities.h>
#include <cage-core/flatSet.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-engine/scene.h>
#include <cage-engine/sound.h>
#include <cage-engine/soundsVoices.h>
#include <cage-engine/speaker.h>

namespace cage
{
	namespace
	{
		template<class T>
		void eraseUnused(T &map, const FlatSet<uintPtr> &used)
		{
			auto it = map.begin();
			while (it != map.end())
			{
				if (used.count(it->first))
					it++;
				else
					it = map.erase(it);
			}
		}

		struct EmitSound
		{
			TransformComponent transform, transformHistory;
			SoundComponent sound;
			SpawnTimeComponent time;
			SceneComponent scene;
			uintPtr id = 0;
		};

		struct EmitListener
		{
			TransformComponent transform, transformHistory;
			ListenerComponent listener;
			SceneComponent scene;
			uintPtr id = 0;
		};

		struct Emit
		{
			std::vector<EmitListener> listeners;
			std::vector<EmitSound> sounds;
			uint64 time = 0;
		};

		struct PrepareListener
		{
			Holder<VoicesMixer> mixer;
			Holder<Voice> chaining; // register this mixer in the engine scene mixer

			ankerl::unordered_dense::map<uintPtr, Holder<Voice>> voicesMapping;
		};

		struct SoundPrepareImpl
		{
			Emit emitBuffers[3];
			Emit *emitRead = nullptr, *emitWrite = nullptr;
			Holder<SwapBufferGuard> swapController;

			InterpolationTimingCorrector itc;
			uint64 emitTime = 0;
			uint64 dispatchTime = 0;
			Real interFactor;

			ankerl::unordered_dense::map<uintPtr, PrepareListener> listenersMapping;

			explicit SoundPrepareImpl(const EngineCreateConfig &config)
			{
				SwapBufferGuardCreateConfig cfg;
				cfg.buffersCount = 3;
				cfg.repeatedReads = true;
				swapController = newSwapBufferGuard(cfg);
			}

			void finalize() { listenersMapping.clear(); }

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
					return;

				emitWrite = &emitBuffers[lock.index()];
				ScopeGuard guard([&]() { emitWrite = nullptr; });
				emitWrite->listeners.clear();
				emitWrite->sounds.clear();
				emitWrite->time = time;

				// emit listeners
				for (Entity *e : engineEntities()->component<ListenerComponent>()->entities())
				{
					EmitListener c;
					c.transform = e->value<TransformComponent>();
					if (e->has(transformHistoryComponent))
						c.transformHistory = e->value<TransformComponent>(transformHistoryComponent);
					else
						c.transformHistory = c.transform;
					c.listener = e->value<ListenerComponent>();
					if (e->has<SceneComponent>())
						c.scene = e->value<SceneComponent>();
					c.id = (uintPtr)e;
					emitWrite->listeners.push_back(c);
				}

				// emit sounds
				for (Entity *e : engineEntities()->component<SoundComponent>()->entities())
				{
					EmitSound c;
					c.transform = e->value<TransformComponent>();
					if (e->has(transformHistoryComponent))
						c.transformHistory = e->value<TransformComponent>(transformHistoryComponent);
					else
						c.transformHistory = c.transform;
					c.sound = e->value<SoundComponent>();
					if (e->has<SpawnTimeComponent>())
						c.time = e->value<SpawnTimeComponent>();
					if (e->has<SceneComponent>())
						c.scene = e->value<SceneComponent>();
					c.id = (uintPtr)e;
					emitWrite->sounds.push_back(c);
				}
			}

			void prepare(PrepareListener &l, Holder<Voice> &v, const EmitSound &e)
			{
				Holder<Sound> s = engineAssets()->get<AssetSchemeIndexSound, Sound>(e.sound.sound);
				if (!s)
				{
					v.clear();
					return;
				}

				if (!v)
					v = l.mixer->newVoice();

				v->sound = std::move(s);
				const Transform t = interpolate(e.transformHistory, e.transform, interFactor);
				v->position = t.position;
				v->startTime = e.time.spawnTime;
				v->attenuation = e.sound.attenuation;
				v->minDistance = e.sound.minDistance;
				v->maxDistance = e.sound.maxDistance;
				v->gain = e.sound.gain;
				v->priority = e.sound.priority;
				v->loop = e.sound.loop;
			}

			void prepare(PrepareListener &l, const EmitListener &e)
			{
				{ // listener
					if (!l.mixer)
					{
						l.mixer = newVoicesMixer();
						l.chaining = engineSceneMixer()->newVoice();
						l.chaining->callback.bind<VoicesMixer, &VoicesMixer::process>(+l.mixer);
					}
					const Transform t = interpolate(e.transformHistory, e.transform, interFactor);
					l.mixer->orientation = t.orientation;
					l.mixer->position = t.position;
					l.mixer->maxActiveSounds = e.listener.maxSounds;
					l.mixer->maxGainThreshold = e.listener.maxGainThreshold;
					l.mixer->gain = e.listener.gain;
				}

				{ // remove obsolete
					FlatSet<uintPtr> used;
					used.reserve(emitRead->sounds.size());
					for (const EmitSound &s : emitRead->sounds)
						if (e.scene.sceneMask & s.scene.sceneMask)
							used.insert(e.id);
					eraseUnused(l.voicesMapping, used);
				}

				for (const EmitSound &s : emitRead->sounds)
					if (e.scene.sceneMask & s.scene.sceneMask)
						prepare(l, l.voicesMapping[s.id], s);
			}

			void prepare()
			{
				{ // remove obsolete
					FlatSet<uintPtr> used;
					used.reserve(emitRead->listeners.size());
					for (const EmitListener &e : emitRead->listeners)
						used.insert(e.id);
					eraseUnused(listenersMapping, used);
				}

				for (const EmitListener &e : emitRead->listeners)
					prepare(listenersMapping[e.id], e);
			}

			void tick(uint64 time)
			{
				auto lock = swapController->read();
				if (!lock)
					return;

				ProfilingScope profiling("sound tick");
				emitRead = &emitBuffers[lock.index()];
				ScopeGuard guard([&]() { emitRead = nullptr; });
				emitTime = emitRead->time;
				const uint64 period = controlThread().updatePeriod();
				dispatchTime = itc(emitTime, time, period);
				interFactor = saturate(Real(dispatchTime - emitTime) / period);
				prepare();
			}

			void dispatch() { engineSpeaker()->process(dispatchTime); }
		};

		SoundPrepareImpl *soundPrepare;
	}

	void soundEmit(uint64 time)
	{
		soundPrepare->emit(time);
	}

	void soundTick(uint64 time)
	{
		soundPrepare->tick(time);
	}

	void soundDispatch()
	{
		soundPrepare->dispatch();
	}

	void soundFinalize()
	{
		soundPrepare->finalize();
	}

	void soundCreate(const EngineCreateConfig &config)
	{
		soundPrepare = systemMemory().createObject<SoundPrepareImpl>(config);
	}

	void soundDestroy()
	{
		systemMemory().destroy<SoundPrepareImpl>(soundPrepare);
		soundPrepare = nullptr;
	}
}
