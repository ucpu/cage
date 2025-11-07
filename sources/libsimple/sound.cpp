#include <unordered_dense.h>
#include <vector>

#include "engine.h"

#include <cage-core/assetsManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/flatSet.h>
#include <cage-core/profiling.h>
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

		struct PrepareListener
		{
			Holder<VoicesMixer> mixer;
			Holder<Voice> chaining; // register this mixer in the engine scene mixer
			ankerl::unordered_dense::map<uintPtr, Holder<Voice>> voicesMapping; // must be destroyed before the mixer
			sint64 lastTime = 0;
			sint64 dispatchTime = 0;

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(mixer);
				// correction for time drift
				const sint64 period = controlThread().updatePeriod();
				const sint64 duration = sint64(data.frames) * 1'000'000 / data.sampleRate;
				if (abs(lastTime - dispatchTime) > period)
					lastTime = dispatchTime; // reset time
				SoundCallbackData clb = data;
				clb.time = lastTime;
				lastTime += duration;
				dispatchTime += duration;
				mixer->process(clb);
			}
		};

		class EnginePrivateSoundImpl : public EnginePrivateSound
		{
		public:
			Holder<Mutex> mut = newMutex();
			std::vector<EmitListener> listeners;
			std::vector<EmitSound> sounds;
			ankerl::unordered_dense::map<uintPtr, Holder<PrepareListener>> listenersMapping; // requires stable pointers

			// control thread ---------------------------------------------------------------------

			explicit EnginePrivateSoundImpl(const EngineCreateConfig &config) {}

			void emit()
			{
				auto lock = ScopeLock(mut);

				listeners.clear();
				sounds.clear();

				EntityComponent *sceneComponent = engineEntities()->component<SceneComponent>();
				EntityComponent *spawnTimeComponent = engineEntities()->component<SpawnTimeComponent>();
				EntityComponent *prevComponent = engineEntities()->componentsByType(detail::typeIndex<TransformComponent>())[1];

				// emit listeners
				entitiesVisitor(
					[&](Entity *e, const TransformComponent &tr, const ListenerComponent &ls)
					{
						EmitListener c;
						c.transform = tr;
						if (e->has(prevComponent))
							c.transformHistory = e->value<TransformComponent>(prevComponent);
						else
							c.transformHistory = c.transform;
						c.listener = ls;
						if (e->has(sceneComponent))
							c.scene = e->value<SceneComponent>(sceneComponent);
						c.id = (uintPtr)e;
						listeners.push_back(c);
					},
					engineEntities(), false);

				// emit sounds
				entitiesVisitor(
					[&](Entity *e, const TransformComponent &tr, const SoundComponent &snd)
					{
						EmitSound c;
						c.transform = tr;
						if (e->has(prevComponent))
							c.transformHistory = e->value<TransformComponent>(prevComponent);
						else
							c.transformHistory = c.transform;
						c.sound = snd;
						if (e->has(spawnTimeComponent))
							c.time = e->value<SpawnTimeComponent>(spawnTimeComponent);
						if (e->has(sceneComponent))
							c.scene = e->value<SceneComponent>(sceneComponent);
						c.id = (uintPtr)e;
						sounds.push_back(c);
					},
					engineEntities(), false);
			}

			// sound thread ---------------------------------------------------------------------

			void initialize() {}

			void finalize() { listenersMapping.clear(); }

			void prepare(PrepareListener &l, Holder<Voice> &v, const EmitSound &e)
			{
				Holder<Sound> s = engineAssets()->get<Sound>(e.sound.sound);
				if (!s)
				{
					v.clear();
					return;
				}

				if (!v)
					v = l.mixer->newVoice();

				v->sound = std::move(s);
				v->position = e.transform.position;
				v->startTime = e.time.spawnTime;
				v->attenuation = e.sound.attenuation;
				v->minDistance = e.sound.minDistance;
				v->maxDistance = e.sound.maxDistance;
				v->gain = e.sound.gain;
				v->priority = e.sound.priority;
				v->loop = e.sound.loop;
			}

			void prepare(PrepareListener &l, const EmitListener &e, uint64 dispatchTime)
			{
				{ // listener
					if (!l.mixer)
					{
						l.mixer = newVoicesMixer();
						l.chaining = engineSceneMixer()->newVoice();
						l.chaining->callback.bind<PrepareListener, &PrepareListener::process>(&l);
					}
					l.mixer->orientation = e.transform.orientation;
					l.mixer->position = e.transform.position;
					l.mixer->maxActiveSounds = e.listener.maxSounds;
					l.mixer->maxGainThreshold = e.listener.maxGainThreshold;
					l.mixer->gain = e.listener.gain;
					l.dispatchTime = dispatchTime;
				}

				{ // remove obsolete
					FlatSet<uintPtr> used;
					used.reserve(sounds.size());
					for (const EmitSound &s : sounds)
						if (e.scene.sceneMask & s.scene.sceneMask)
							used.insert(e.id);
					eraseUnused(l.voicesMapping, used);
				}

				for (const EmitSound &s : sounds)
					if (e.scene.sceneMask & s.scene.sceneMask)
						prepare(l, l.voicesMapping[s.id], s);
			}

			void dispatch(uint64 dispatchTime)
			{
				auto lock = ScopeLock(mut);

				{ // remove obsolete
					FlatSet<uintPtr> used;
					used.reserve(listeners.size());
					for (const EmitListener &e : listeners)
						used.insert(e.id);
					eraseUnused(listenersMapping, used);
				}

				for (const EmitListener &e : listeners)
				{
					Holder<PrepareListener> &h = listenersMapping[e.id];
					if (!h)
						h = systemMemory().createHolder<PrepareListener>();
					prepare(*h, e, dispatchTime);
				}

				{
					ProfilingScope profiling("speaker process");
					engineSpeaker()->process(controlThread().updatePeriod());
				}
			}
		};

	}

	void EnginePrivateSound::emit()
	{
		EnginePrivateSoundImpl *impl = (EnginePrivateSoundImpl *)this;
		impl->emit();
	}

	void EnginePrivateSound::initialize()
	{
		EnginePrivateSoundImpl *impl = (EnginePrivateSoundImpl *)this;
		impl->initialize();
	}

	void EnginePrivateSound::finalize()
	{
		EnginePrivateSoundImpl *impl = (EnginePrivateSoundImpl *)this;
		impl->finalize();
	}

	void EnginePrivateSound::dispatch(uint64 time)
	{
		EnginePrivateSoundImpl *impl = (EnginePrivateSoundImpl *)this;
		impl->dispatch(time);
	}

	Holder<EnginePrivateSound> newEnginePrivateSound(const EngineCreateConfig &config)
	{
		return systemMemory().createImpl<EnginePrivateSound, EnginePrivateSoundImpl>(config);
	}
}
