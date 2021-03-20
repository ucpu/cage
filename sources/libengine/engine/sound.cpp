#include <cage-core/flatSet.h>

#include <cage-engine/sound.h>
#include <cage-engine/voices.h>
#include <cage-engine/speaker.h>

#include "engine.h"

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

		struct Listener
		{
			// todo split by sound category (music / voice over / effect)
			Holder<VoicesMixer> mixer;
			Holder<VoiceProperties> chaining; // register this mixer in the engine effects mixer

			robin_hood::unordered_map<uintPtr, Holder<VoiceProperties>> voicesMapping;
		};

		struct SoundPrepareImpl
		{
			Emit emitBuffers[3];
			Emit *emitRead = nullptr, *emitWrite = nullptr;
			Holder<SwapBufferGuard> swapController;

			InterpolationTimingCorrector itc;
			uint64 emitTime = 0;
			uint64 dispatchTime = 0;
			real interFactor;

			robin_hood::unordered_map<uintPtr, Listener> listenersMapping;

			explicit SoundPrepareImpl(const EngineCreateConfig &config)
			{
				SwapBufferGuardCreateConfig cfg(3);
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
				for (Entity *e : ListenerComponent::component->entities())
				{
					EmitListener c;
					c.transform = e->value<TransformComponent>(TransformComponent::component);
					if (e->has(TransformComponent::componentHistory))
						c.transformHistory = e->value<TransformComponent>(TransformComponent::componentHistory);
					else
						c.transformHistory = c.transform;
					c.listener = e->value<ListenerComponent>(ListenerComponent::component);
					c.id = (uintPtr)e;
					emitWrite->listeners.push_back(c);
				}

				// emit voices
				for (Entity *e : SoundComponent::component->entities())
				{
					EmitSound c;
					c.transform = e->value<TransformComponent>(TransformComponent::component);
					if (e->has(TransformComponent::componentHistory))
						c.transformHistory = e->value<TransformComponent>(TransformComponent::componentHistory);
					else
						c.transformHistory = c.transform;
					c.sound = e->value<SoundComponent>(SoundComponent::component);
					c.id = (uintPtr)e;
					emitWrite->sounds.push_back(c);
				}
			}

			void prepare(Listener &l, Holder<VoiceProperties> &v, const EmitSound &e)
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
				v->callback.bind<Sound, &Sound::process>(+v->sound);
				const transform t = interpolate(e.transformHistory, e.transform, interFactor);
				v->position = t.position;
				v->startTime = e.sound.startTime;
				v->referenceDistance = s->referenceDistance;
				v->rolloffFactor = s->rolloffFactor;
				v->gain = e.sound.gain * s->gain;
			}

			void prepare(Listener &l, const EmitListener &e)
			{
				{ // listener
					if (!l.mixer)
					{
						l.mixer = newVoicesMixer({});
						l.chaining = engineEffectsMixer()->newVoice();
						l.chaining->callback.bind<VoicesMixer, &VoicesMixer::process>(+l.mixer);
					}
					ListenerProperties &p = l.mixer->listener();
					const transform t = interpolate(e.transformHistory, e.transform, interFactor);
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
				OPTICK_EVENT("prepare");

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
					emitRead = &emitBuffers[lock.index()];
					ClearOnScopeExit resetEmitRead(emitRead);
					emitTime = emitRead->time;
					dispatchTime = itc(emitTime, time, soundThread().updatePeriod());
					interFactor = saturate(real(dispatchTime - emitTime) / controlThread().updatePeriod());
					prepare();
				}

				{
					OPTICK_EVENT("speaker");
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
		soundPrepare = detail::systemArena().createObject<SoundPrepareImpl>(config);
	}

	void soundDestroy()
	{
		detail::systemArena().destroy<SoundPrepareImpl>(soundPrepare);
		soundPrepare = nullptr;
	}
}
