#include <cage-core/flatSet.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/profiling.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#include <cage-engine/sound.h>
#include <cage-engine/voices.h>
#include <cage-engine/speaker.h>
#include <cage-engine/scene.h>

#include "engine.h"
#include "interpolationTimingCorrector.h"

#include <vector>
#include <robin_hood.h>

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

		struct ClearOnScopeExit : private Immovable
		{
			template<class T>
			explicit ClearOnScopeExit(T *&ptr) : ptr((void *&)ptr)
			{}

			~ClearOnScopeExit()
			{
				ptr = nullptr;
			}

		private:
			void *&ptr;
		};

		struct EmitSound
		{
			TransformComponent transform, transformHistory;
			SoundComponent sound = {};
			uintPtr id = 0;
		};

		struct EmitListener
		{
			TransformComponent transform, transformHistory;
			ListenerComponent listener = {};
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
			// todo split by sound category (music / voice over / effect)
			Holder<VoicesMixer> mixer;
			Holder<Voice> chaining; // register this mixer in the engine effects mixer

			robin_hood::unordered_map<uintPtr, Holder<Voice>> voicesMapping;
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

			robin_hood::unordered_map<uintPtr, PrepareListener> listenersMapping;

			explicit SoundPrepareImpl(const EngineCreateConfig &config)
			{
				SwapBufferGuardCreateConfig cfg;
				cfg.buffersCount = 3;
				cfg.repeatedReads = true;
				swapController = newSwapBufferGuard(cfg);
			}

			void finalize()
			{
				listenersMapping.clear();
			}

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping sound emit");
					return;
				}

				emitWrite = &emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitWrite(emitWrite);
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
					c.id = (uintPtr)e;
					emitWrite->listeners.push_back(c);
				}

				// emit voices
				for (Entity *e : engineEntities()->component<SoundComponent>()->entities())
				{
					EmitSound c;
					c.transform = e->value<TransformComponent>();
					if (e->has(transformHistoryComponent))
						c.transformHistory = e->value<TransformComponent>(transformHistoryComponent);
					else
						c.transformHistory = c.transform;
					c.sound = e->value<SoundComponent>();
					c.id = (uintPtr)e;
					emitWrite->sounds.push_back(c);
				}
			}

			void prepare(PrepareListener &l, Holder<Voice> &v, const EmitSound &e)
			{
				Holder<Sound> s = engineAssets()->get<AssetSchemeIndexSound, Sound>(e.sound.name);
				if (!s)
				{
					v.clear();
					return;
				}

				if (!v)
					v = l.mixer->newVoice();

				v->sound = s.share();
				const Transform t = interpolate(e.transformHistory, e.transform, interFactor);
				v->position = t.position;
				v->startTime = e.sound.startTime;
				v->gain = e.sound.gain;
			}

			void prepare(PrepareListener &l, const EmitListener &e)
			{
				{ // listener
					if (!l.mixer)
					{
						l.mixer = newVoicesMixer({});
						l.chaining = engineEffectsMixer()->newVoice();
						l.chaining->callback.bind<VoicesMixer, &VoicesMixer::process>(+l.mixer);
					}
					Listener &p = l.mixer->listener();
					const Transform t = interpolate(e.transformHistory, e.transform, interFactor);
					p.position = t.position;
					p.orientation = t.orientation;
					p.rolloffFactor = e.listener.rolloffFactor;
					p.gain = e.listener.gain;
				}

				{ // remove obsolete
					FlatSet<uintPtr> used;
					used.reserve(emitRead->sounds.size());
					for (const EmitSound &s : emitRead->sounds)
						if (e.listener.sceneMask & s.sound.sceneMask)
							used.insert(e.id);
					eraseUnused(l.voicesMapping, used);
				}

				for (const EmitSound &s : emitRead->sounds)
					if (e.listener.sceneMask & s.sound.sceneMask)
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
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping sound tick");
					return;
				}

				{
					ProfilingScope profiling("sound prepare");
					emitRead = &emitBuffers[lock.index()];
					ClearOnScopeExit resetEmitRead(emitRead);
					emitTime = emitRead->time;
					dispatchTime = itc(emitTime, time, soundThread().updatePeriod());
					interFactor = saturate(Real(dispatchTime - emitTime) / controlThread().updatePeriod());
					prepare();
				}

				{
					ProfilingScope profiling("speaker");
					engineSpeaker()->process(dispatchTime);
				}
			}
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
