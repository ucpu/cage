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

			ankerl::unordered_dense::map<uintPtr, Holder<Voice>> voicesMapping;
		};

		struct SoundPrepareImpl
		{
			Holder<Mutex> mut = newMutex();
			std::vector<EmitListener> listeners;
			std::vector<EmitSound> sounds;
			ankerl::unordered_dense::map<uintPtr, PrepareListener> listenersMapping;

			explicit SoundPrepareImpl(const EngineCreateConfig &config) {}

			void finalize() { listenersMapping.clear(); }

			void emit()
			{
				auto lock = ScopeLock(mut);

				listeners.clear();
				sounds.clear();

				EntityComponent *sceneComponent = engineEntities()->component<SceneComponent>();
				EntityComponent *spawnTimeComponent = engineEntities()->component<SpawnTimeComponent>();

				// emit listeners
				entitiesVisitor(
					[&](Entity *e, const TransformComponent &tr, const ListenerComponent &ls)
					{
						EmitListener c;
						c.transform = tr;
						if (e->has(transformHistoryComponent))
							c.transformHistory = e->value<TransformComponent>(transformHistoryComponent);
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
						if (e->has(transformHistoryComponent))
							c.transformHistory = e->value<TransformComponent>(transformHistoryComponent);
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
				v->position = e.transform.position;
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
					l.mixer->orientation = e.transform.orientation;
					l.mixer->position = e.transform.position;
					l.mixer->maxActiveSounds = e.listener.maxSounds;
					l.mixer->maxGainThreshold = e.listener.maxGainThreshold;
					l.mixer->gain = e.listener.gain;
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

			void dispatch(uint64 time)
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
					prepare(listenersMapping[e.id], e);

				{
					ProfilingScope profiling("speaker process");
					engineSpeaker()->process(time, controlThread().updatePeriod());
				}
			}
		};

		SoundPrepareImpl *soundPrepare;
	}

	void soundEmit()
	{
		soundPrepare->emit();
	}

	void soundDispatch(uint64 time)
	{
		soundPrepare->dispatch(time);
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
