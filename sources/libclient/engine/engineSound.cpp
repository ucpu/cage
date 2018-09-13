#include <vector>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/swapBufferController.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>
#include <cage-client/engine.h>

#include "engine.h"
#include "../sound/utilities.h"

namespace cage
{
	namespace
	{
		struct soundPrepareImpl;

		struct emitVoiceStruct
		{
			transformComponent transform, transformHistory;
			voiceComponent voice;

			emitVoiceStruct()
			{
				detail::memset(this, 0, sizeof(emitVoiceStruct));
			}
		};

		struct emitListenerStruct
		{
			transformComponent transform, transformHistory;
			listenerComponent listener;

			emitListenerStruct()
			{
				detail::memset(this, 0, sizeof(emitListenerStruct));
			}
		};

		struct mixStruct
		{
			holder<busClass> bus;
			holder<filterClass> filter;
			soundPrepareImpl *data;
			emitVoiceStruct *voice;
			emitListenerStruct *listener;

			mixStruct(soundPrepareImpl *data) : data(data), voice(nullptr), listener(nullptr) {};

			void prepare(emitVoiceStruct *voice, emitListenerStruct *listener)
			{
				CAGE_ASSERT_RUNTIME(!!voice == !!listener);
				if (bus)
					bus->clear();
				else
				{
					bus = newBus(sound());
					filter = newFilter(sound());
					filter->execute.bind<mixStruct, &mixStruct::exe>(this);
				}
				this->voice = voice;
				this->listener = listener;
				if (!voice)
					return;
				CAGE_ASSERT_RUNTIME(voice->voice.input == nullptr || voice->voice.name == 0, voice->voice.input, voice->voice.name, "voice may only have one source");
				if (voice->voice.input)
				{ // direct bus input
					voice->voice.input->addOutput(bus.get());
				}
				else
				{ // asset input
					assetManagerClass *ass = assets();
					if (!ass->ready(voice->voice.name))
						return;
					ass->get<assetSchemeIndexSound, sourceClass>(voice->voice.name)->addOutput(bus.get());
				}
				if (listener->listener.output)
					bus->addOutput(listener->listener.output);
				else
					bus->addOutput(effectsMixer());
				filter->setBus(bus.get());
			}

			void exe(const filterApiStruct &api);
		};

		struct emitStruct
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> emitMemory;
			memoryArena emitArena;

			std::vector<emitVoiceStruct*> voices;
			std::vector<emitListenerStruct*> listeners;

			uint64 time;
			bool fresh;

			emitStruct(const engineCreateConfig &config) : emitMemory(config.soundEmitMemory), emitArena(&emitMemory), time(0), fresh(false)
			{
				voices.reserve(256);
				listeners.reserve(4);
			}

			~emitStruct()
			{
				emitArena.flush();
			}
		};

		struct soundPrepareImpl
		{
			emitStruct emitBuffers[3];
			emitStruct *emitRead, *emitWrite;
			holder<swapBufferControllerClass> swapController;

			std::vector<holder<mixStruct>> mixers;
			std::vector<float> soundMixBuffer;

			interpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			real interFactor;

			soundPrepareImpl(const engineCreateConfig &config) : emitBuffers{ config, config, config }, emitRead(nullptr), emitWrite(nullptr), emitTime(0), dispatchTime(0)
			{
				mixers.reserve(256);
				soundMixBuffer.resize(10000);
				{
					swapBufferControllerCreateConfig cfg(3);
					cfg.repeatedReads = true;
					swapController = newSwapBufferController(cfg);
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
					CAGE_LOG_DEBUG(severityEnum::Warning, "engine", "skipping sound emit (write)");
					return;
				}

				emitWrite = &emitBuffers[lock.index()];
				emitWrite->fresh = true;
				emitWrite->voices.clear();
				emitWrite->listeners.clear();
				emitWrite->emitArena.flush();
				emitWrite->time = time;

				// emit voices
				for (entityClass *e : voiceComponent::component->getComponentEntities()->entities())
				{
					emitVoiceStruct *c = emitWrite->emitArena.createObject<emitVoiceStruct>();
					c->transform = e->value<transformComponent>(transformComponent::component);
					if (e->hasComponent(transformComponent::componentHistory))
						c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->voice = e->value<voiceComponent>(voiceComponent::component);
					emitWrite->voices.push_back(c);
				}

				// emit listeners
				for (entityClass *e : listenerComponent::component->getComponentEntities()->entities())
				{
					emitListenerStruct *c = emitWrite->emitArena.createObject<emitListenerStruct>();
					c->transform = e->value<transformComponent>(transformComponent::component);
					if (e->hasComponent(transformComponent::componentHistory))
						c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->listener = e->value<listenerComponent>(listenerComponent::component);
					emitWrite->listeners.push_back(c);
				}

				emitWrite = nullptr;
			}

			void postEmit()
			{
				uint32 used = 0;
				for (auto itL : emitRead->listeners)
				{
					for (auto itV : emitRead->voices)
					{
						if ((itL->listener.renderMask & itV->voice.renderMask) == 0)
							continue;
						if (mixers.size() <= used)
						{
							mixers.resize(used + 1);
							mixers[used] = detail::systemArena().createHolder<mixStruct>(this);
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
					CAGE_LOG_DEBUG(severityEnum::Warning, "engine", "skipping sound emit (read)");
					return;
				}

				emitRead = &emitBuffers[lock.index()];
				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, soundThread().timePerTick);
				interFactor = clamp(real(dispatchTime - emitTime) / soundThread().timePerTick, 0, 1);

				if (emitRead->fresh)
				{
					postEmit();
					emitRead->fresh = false;
				}
				speaker()->update(dispatchTime);

				emitRead = nullptr;
			}
		};

		void mixStruct::exe(const filterApiStruct &api)
		{
			CAGE_ASSERT_RUNTIME(voice && listener);
			vec3 posVoice = interpolate(voice->transformHistory.position, voice->transform.position, data->interFactor);
			vec3 posListener = interpolate(listener->transformHistory.position, listener->transform.position, data->interFactor);
			vec3 diff = posVoice - posListener;
			real dist = diff.length();
			vec3 dir = dist > 1e-7 ? diff.normalize() : vec3();
			vec3 &a = listener->listener.attenuation;
			real vol = min(real(1) / (a[0] + a[1] * dist + a[2] * dist * dist), 1);
			if (vol < 1e-5)
				return;
			soundDataBufferStruct s = api.output;
			if (data->soundMixBuffer.size() < s.frames)
				data->soundMixBuffer.resize(s.frames);
			s.buffer = data->soundMixBuffer.data();
			s.time -= voice->voice.startTime;
			s.channels = 1;
			if (listener->listener.dopplerEffect)
			{
				real scale = 1000000 / controlThread().timePerTick;
				vec3 velL = scale * (listener->transformHistory.position - listener->transform.position);
				vec3 velV = scale * (voice->transformHistory.position - voice->transform.position);
				real vl = velL.dot(dir);
				real vv = velV.dot(dir);
				real doppler = max((listener->listener.speedOfSound + vl) / (listener->listener.speedOfSound + vv), 0);
				s.sampleRate = numeric_cast<uint32>(real(s.sampleRate) * doppler);
				sint64 ts = numeric_cast<sint64>(1000000 * dist / listener->listener.speedOfSound);
				s.time -= ts;
			}
			detail::memset(s.buffer, 0, s.frames * sizeof(float));
			api.input(s);
			for (uint32 i = 0; i < s.frames; i++)
				s.buffer[i] *= vol.value;
			soundPrivat::channelsDirection(s.buffer, api.output.buffer, api.output.channels, s.frames, dir * listener->transform.orientation.conjugate());
		}

		soundPrepareImpl *soundPrepare;
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

	void soundCreate(const engineCreateConfig &config)
	{
		soundPrepare = detail::systemArena().createObject<soundPrepareImpl>(config);
	}

	void soundDestroy()
	{
		detail::systemArena().destroy<soundPrepareImpl>(soundPrepare);
		soundPrepare = nullptr;
	}
}
