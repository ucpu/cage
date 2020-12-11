#include <cage-engine/sound.h>

#include "engine.h"
#include "../sound/utilities.h"

#include <vector>
#include <map>

namespace cage
{
	namespace
	{
		struct SoundPrepareImpl;

		struct EmitSound
		{
			TransformComponent transform, transformHistory;
			SoundComponent sound;
			uintPtr id;

			EmitSound()
			{
				detail::memset(this, 0, sizeof(EmitSound));
			}
		};

		struct EmitListener
		{
			TransformComponent transform, transformHistory;
			ListenerComponent listener;
			uintPtr id;

			EmitListener()
			{
				detail::memset(this, 0, sizeof(EmitListener));
			}
		};

		struct Mix
		{
			Holder<MixingBus> bus;
			Holder<MixingFilter> filter;
			Holder<SoundSource> source;
			SoundPrepareImpl *const data;
			EmitListener listener;
			EmitSound sound;

			explicit Mix(SoundPrepareImpl *data) : data(data)
			{};

			void prepare(const EmitListener &listener, const EmitSound &sound)
			{
				this->sound = sound;
				this->listener = listener;
				CAGE_ASSERT(sound.sound.input == nullptr || sound.sound.name == 0);
				source.clear();
				bus = newMixingBus(); // recreating new bus will ensure that any potential previous connections are removed
				filter = newMixingFilter();
				filter->execute.bind<Mix, &Mix::exe>(this);
				if (sound.sound.input)
				{ // direct bus input
					sound.sound.input->addOutput(bus.get());
				}
				else
				{ // asset input
					AssetManager *ass = engineAssets();
					source = ass->get<AssetSchemeIndexSoundSource, SoundSource>(sound.sound.name);
					if (!source)
						return;
					source->addOutput(bus.get());
				}
				if (listener.listener.output)
					bus->addOutput(listener.listener.output);
				else
					bus->addOutput(engineEffectsMixer());
				filter->setBus(bus.get());
			}

			void exe(const MixingFilterApi &api);
		};

		struct Emit
		{
			std::vector<EmitListener> listeners;
			std::vector<EmitSound> voices;

			uint64 time;
			bool fresh;

			explicit Emit(const EngineCreateConfig &config) : time(0), fresh(false)
			{
				listeners.reserve(4);
				voices.reserve(256);
			}

			~Emit()
			{}
		};

		struct SoundPrepareImpl
		{
			Emit emitBufferA, emitBufferB, emitBufferC; // this is awfully stupid, damn you c++
			Emit *emitBuffers[3];
			Emit *emitRead, *emitWrite;
			Holder<SwapBufferGuard> swapController;

			// listener -> sound -> mix
			typedef std::map<uintPtr, std::map<uintPtr, Holder<Mix>>> Mixers; // todo use combined key and unordered_map that can be reserved
			Mixers mixers;

			std::vector<float> soundMixBuffer;
			InterpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			real interFactor;

			explicit SoundPrepareImpl(const EngineCreateConfig &config) : emitBufferA(config), emitBufferB(config), emitBufferC(config), emitBuffers{ &emitBufferA, &emitBufferB, &emitBufferC }, emitRead(nullptr), emitWrite(nullptr), emitTime(0), dispatchTime(0)
			{
				soundMixBuffer.resize(10000);
				{
					SwapBufferGuardCreateConfig cfg(3);
					cfg.repeatedReads = true;
					swapController = newSwapBufferGuard(cfg);
				}
			}

			void finalize()
			{
				mixers.clear();
			}

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping sound emit");
					return;
				}

				emitWrite = emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitWrite(emitWrite);
				emitWrite->fresh = true;
				emitWrite->listeners.clear();
				emitWrite->voices.clear();
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
					emitWrite->voices.push_back(c);
				}
			}

			void postEmit()
			{
				OPTICK_EVENT("post emit");
				Mixers ms;
				for (const auto &itL : emitRead->listeners)
				{
					for (const auto &itV : emitRead->voices)
					{
						if ((itL.listener.sceneMask & itV.sound.sceneMask) == 0)
							continue;
						auto &old = mixers[itL.id][itV.id];
						if (old)
						{
							old->prepare(itL, itV);
							ms[itL.id][itV.id] = templates::move(old);
						}
						else
						{
							Holder<Mix> n = detail::systemArena().createHolder<Mix>(this);
							n->prepare(itL, itV);
							ms[itL.id][itV.id] = templates::move(n);
						}
					}
				}
				std::swap(ms, mixers);
			}

			void tick(uint64 time)
			{
				auto lock = swapController->read();
				if (!lock)
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping sound tick");
					return;
				}

				emitRead = emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitRead(emitRead);
				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, soundThread().updatePeriod());
				interFactor = clamp(real(dispatchTime - emitTime) / controlThread().updatePeriod(), 0, 1);

				if (emitRead->fresh)
				{
					postEmit();
					emitRead->fresh = false;
				}

				{
					OPTICK_EVENT("speaker");
					engineSpeaker()->update(dispatchTime);
				}
			}
		};

		void Mix::exe(const MixingFilterApi &api)
		{
			vec3 posVoice = interpolate(sound.transformHistory.position, sound.transform.position, data->interFactor);
			vec3 posListener = interpolate(listener.transformHistory.position, listener.transform.position, data->interFactor);
			vec3 diff = posVoice - posListener;
			real dist = length(diff);
			vec3 dir = dist > 1e-7 ? normalize(diff) : vec3();
			const vec3 &a = listener.listener.attenuation;
			real vol = min(real(1) / (a[0] + a[1] * dist + a[2] * dist * dist), 1);
			if (vol < 1e-5)
				return;
			SoundDataBuffer s = api.output;
			if (data->soundMixBuffer.size() < s.frames)
				data->soundMixBuffer.resize(s.frames);
			s.buffer = data->soundMixBuffer.data();
			s.time -= sound.sound.startTime;
			s.channels = 1;
			if (listener.listener.dopplerEffect)
			{
				// the Mix filter is now persistent across frames and can therefore have persistent state
				// this can probably be used to improve the doppler effect

				real scale = 1000000 / controlThread().updatePeriod();
				vec3 velL = scale * (listener.transformHistory.position - listener.transform.position);
				vec3 velV = scale * (sound.transformHistory.position - sound.transform.position);
				real vl = dot(velL, dir);
				real vv = dot(velV, dir);
				real doppler = max((listener.listener.speedOfSound + vl) / (listener.listener.speedOfSound + vv), 0);
				s.sampleRate = numeric_cast<uint32>(real(s.sampleRate) * doppler);
				sint64 ts = numeric_cast<sint64>(1000000 * dist / listener.listener.speedOfSound);
				s.time -= ts;
			}
			detail::memset(s.buffer, 0, s.frames * sizeof(float));
			api.input(s);
			for (uint32 i = 0; i < s.frames; i++)
				s.buffer[i] *= vol.value;
			soundPrivat::channelsDirection(s.buffer, api.output.buffer, api.output.channels, s.frames, dir * conjugate(listener.transform.orientation));
		}

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
