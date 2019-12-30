#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferGuard.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>
#include <cage-engine/engine.h>

#include "engine.h"
#include "../sound/utilities.h"

#include <vector>

namespace cage
{
	namespace
	{
		struct SoundPrepareImpl;

		struct EmitSound
		{
			TransformComponent transform, transformHistory;
			SoundComponent sound;

			EmitSound()
			{
				detail::memset(this, 0, sizeof(EmitSound));
			}
		};

		struct EmitListener
		{
			TransformComponent transform, transformHistory;
			ListenerComponent listener;

			EmitListener()
			{
				detail::memset(this, 0, sizeof(EmitListener));
			}
		};

		struct Mix
		{
			Holder<MixingBus> bus;
			Holder<MixingFilter> filter;
			SoundPrepareImpl *data;
			EmitSound *sound;
			EmitListener *listener;

			Mix(SoundPrepareImpl *data) : data(data), sound(nullptr), listener(nullptr) {};

			void prepare(EmitSound *sound, EmitListener *listener)
			{
				CAGE_ASSERT(!!sound == !!listener);
				if (bus)
					bus->clear();
				else
				{
					bus = newMixingBus(cage::sound());
					filter = newMixingFilter(cage::sound());
					filter->execute.bind<Mix, &Mix::exe>(this);
				}
				this->sound = sound;
				this->listener = listener;
				if (!sound)
					return;
				CAGE_ASSERT(sound->sound.input == nullptr || sound->sound.name == 0, sound->sound.input, sound->sound.name, "sound may only have one source");
				if (sound->sound.input)
				{ // direct bus input
					sound->sound.input->addOutput(bus.get());
				}
				else
				{ // asset input
					AssetManager *ass = assets();
					if (!ass->ready(sound->sound.name))
						return;
					ass->get<assetSchemeIndexSoundSource, SoundSource>(sound->sound.name)->addOutput(bus.get());
				}
				if (listener->listener.output)
					bus->addOutput(listener->listener.output);
				else
					bus->addOutput(effectsMixer());
				filter->setBus(bus.get());
			}

			void exe(const MixingFilterApi &api);
		};

		struct Emit
		{
			MemoryArenaGrowing<MemoryAllocatorPolicyLinear<>, MemoryConcurrentPolicyNone> emitMemory;
			MemoryArena emitArena;

			std::vector<EmitSound*> voices;
			std::vector<EmitListener*> listeners;

			uint64 time;
			bool fresh;

			explicit Emit(const EngineCreateConfig &config) : emitMemory(config.soundEmitMemory), emitArena(&emitMemory), time(0), fresh(false)
			{
				voices.reserve(256);
				listeners.reserve(4);
			}

			~Emit()
			{
				emitArena.flush();
			}
		};

		struct SoundPrepareImpl
		{
			Emit emitBufferA, emitBufferB, emitBufferC; // this is awfully stupid, damn you c++
			Emit *emitBuffers[3];
			Emit *emitRead, *emitWrite;
			Holder<SwapBufferGuard> swapController;

			std::vector<Holder<Mix>> mixers;
			std::vector<float> soundMixBuffer;

			InterpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			real interFactor;

			explicit SoundPrepareImpl(const EngineCreateConfig &config) : emitBufferA(config), emitBufferB(config), emitBufferC(config), emitBuffers{ &emitBufferA, &emitBufferB, &emitBufferC }, emitRead(nullptr), emitWrite(nullptr), emitTime(0), dispatchTime(0)
			{
				mixers.reserve(256);
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
				emitWrite->voices.clear();
				emitWrite->listeners.clear();
				emitWrite->emitArena.flush();
				emitWrite->time = time;

				// emit voices
				for (Entity *e : SoundComponent::component->entities())
				{
					EmitSound *c = emitWrite->emitArena.createObject<EmitSound>();
					c->transform = e->value<TransformComponent>(TransformComponent::component);
					if (e->has(TransformComponent::componentHistory))
						c->transformHistory = e->value<TransformComponent>(TransformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->sound = e->value<SoundComponent>(SoundComponent::component);
					emitWrite->voices.push_back(c);
				}

				// emit listeners
				for (Entity *e : ListenerComponent::component->entities())
				{
					EmitListener *c = emitWrite->emitArena.createObject<EmitListener>();
					c->transform = e->value<TransformComponent>(TransformComponent::component);
					if (e->has(TransformComponent::componentHistory))
						c->transformHistory = e->value<TransformComponent>(TransformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->listener = e->value<ListenerComponent>(ListenerComponent::component);
					emitWrite->listeners.push_back(c);
				}
			}

			void postEmit()
			{
				OPTICK_EVENT("post emit");
				uint32 used = 0;
				for (auto itL : emitRead->listeners)
				{
					for (auto itV : emitRead->voices)
					{
						if ((itL->listener.sceneMask & itV->sound.sceneMask) == 0)
							continue;
						if (mixers.size() <= used)
						{
							mixers.resize(used + 1);
							mixers[used] = detail::systemArena().createHolder<Mix>(this);
						}
						mixers[used]->prepare(itV, itL);
						used++;
					}
				}
				for (uint32 i = used, e = numeric_cast<uint32>(mixers.size()); i < e; i++)
					mixers[i]->prepare(nullptr, nullptr);
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
				dispatchTime = itc(emitTime, time, soundThread().timePerTick);
				interFactor = clamp(real(dispatchTime - emitTime) / soundThread().timePerTick, 0, 1);

				if (emitRead->fresh)
				{
					postEmit();
					emitRead->fresh = false;
				}

				{
					OPTICK_EVENT("speaker");
					speaker()->update(dispatchTime);
				}
			}
		};

		void Mix::exe(const MixingFilterApi &api)
		{
			CAGE_ASSERT(sound && listener);
			vec3 posVoice = interpolate(sound->transformHistory.position, sound->transform.position, data->interFactor);
			vec3 posListener = interpolate(listener->transformHistory.position, listener->transform.position, data->interFactor);
			vec3 diff = posVoice - posListener;
			real dist = length(diff);
			vec3 dir = dist > 1e-7 ? normalize(diff) : vec3();
			vec3 &a = listener->listener.attenuation;
			real vol = min(real(1) / (a[0] + a[1] * dist + a[2] * dist * dist), 1);
			if (vol < 1e-5)
				return;
			SoundDataBuffer s = api.output;
			if (data->soundMixBuffer.size() < s.frames)
				data->soundMixBuffer.resize(s.frames);
			s.buffer = data->soundMixBuffer.data();
			s.time -= sound->sound.startTime;
			s.channels = 1;
			if (listener->listener.dopplerEffect)
			{
				real scale = 1000000 / controlThread().timePerTick;
				vec3 velL = scale * (listener->transformHistory.position - listener->transform.position);
				vec3 velV = scale * (sound->transformHistory.position - sound->transform.position);
				real vl = dot(velL, dir);
				real vv = dot(velV, dir);
				real doppler = max((listener->listener.speedOfSound + vl) / (listener->listener.speedOfSound + vv), 0);
				s.sampleRate = numeric_cast<uint32>(real(s.sampleRate) * doppler);
				sint64 ts = numeric_cast<sint64>(1000000 * dist / listener->listener.speedOfSound);
				s.time -= ts;
			}
			detail::memset(s.buffer, 0, s.frames * sizeof(float));
			api.input(s);
			for (uint32 i = 0; i < s.frames; i++)
				s.buffer[i] *= vol.value;
			soundPrivat::channelsDirection(s.buffer, api.output.buffer, api.output.channels, s.frames, dir * conjugate(listener->transform.orientation));
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
