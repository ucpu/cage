#include <vector>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>

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
				CAGE_ASSERT_RUNTIME(voice->voice.input == nullptr || voice->voice.sound == 0, voice->voice.input, voice->voice.sound, "voice may only have one source");
				if (voice->voice.input)
				{ // direct bus input
					voice->voice.input->addOutput(bus.get());
				}
				else
				{ // asset input
					assetManagerClass *ass = assets();
					if (!ass->ready(voice->voice.sound))
						return;
					ass->get<assetSchemeIndexSound, sourceClass>(voice->voice.sound)->addOutput(bus.get());
				}
				if (listener->listener.output)
					bus->addOutput(listener->listener.output);
				else
					bus->addOutput(effectsMixer());
				filter->setBus(bus.get());
			}

			void exe(const filterApiStruct &api);
		};

		struct soundPrepareImpl
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> emitMemory;
			memoryArena emitArena;

			std::vector<emitVoiceStruct*> emitVoices;
			std::vector<emitListenerStruct*> emitListeners;
			std::vector<holder<mixStruct> > mixers;
			std::vector<float> soundMixBuffer;

			uint64 controlTime;
			uint64 prepareTime;
			real interFactor;
			bool wasEmit;

			soundPrepareImpl(const engineCreateConfig &config) : emitMemory(config.soundEmitMemory)
			{
				mixers.reserve(256);
				emitVoices.reserve(256);
				emitListeners.reserve(4);
				soundMixBuffer.resize(10000);
			}

			void initialize()
			{
				emitArena = memoryArena(&emitMemory);
			}

			void finalize()
			{
				emitArena.flush();
				emitArena = memoryArena();
				mixers.clear();
			}

			void emit()
			{
				wasEmit = true;
				emitVoices.clear();
				emitListeners.clear();
				emitArena.flush();

				{ // emit voices
					uint32 cnt = voiceComponent::component->getComponentEntities()->entitiesCount();
					entityClass *const *ents = voiceComponent::component->getComponentEntities()->entitiesArray();
					for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
					{
						entityClass *e = *i;
						emitVoiceStruct *c = emitArena.createObject<emitVoiceStruct>();
						c->transform = e->value<transformComponent>(transformComponent::component);
						if (e->hasComponent(transformComponent::componentHistory))
							c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
						else
							c->transformHistory = c->transform;
						c->voice = e->value<voiceComponent>(voiceComponent::component);
						emitVoices.push_back(c);
					}
				}
				{ // emit listeners
					uint32 cnt = listenerComponent::component->getComponentEntities()->entitiesCount();
					entityClass *const *ents = listenerComponent::component->getComponentEntities()->entitiesArray();
					for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
					{
						entityClass *e = *i;
						emitListenerStruct *c = emitArena.createObject<emitListenerStruct>();
						c->transform = e->value<transformComponent>(transformComponent::component);
						if (e->hasComponent(transformComponent::componentHistory))
							c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
						else
							c->transformHistory = c->transform;
						c->listener = e->value<listenerComponent>(listenerComponent::component);
						emitListeners.push_back(c);
					}
				}
			}

			void postEmit()
			{
				uint32 index = 0;
				for (auto itL = emitListeners.begin(), etL = emitListeners.end(); itL != etL; itL++)
				{
					for (auto itV = emitVoices.begin(), etV = emitVoices.end(); itV != etV; itV++)
					{
						if (((*itL)->listener.renderMask & (*itV)->voice.renderMask) == 0)
							continue;
						if (mixers.size() <= index)
						{
							mixers.resize(index + 1);
							mixers[index] = detail::systemArena().createHolder<mixStruct>(this);
						}
						mixers[index]->prepare(*itV, *itL);
						index++;
					}
				}
				for (uint32 i = index, e = numeric_cast<uint32>(mixers.size()); i < e; i++)
					mixers[i]->prepare(nullptr, nullptr);
			}

			void tick(uint64 controlTime, uint64 prepareTime)
			{
				if (wasEmit)
				{
					postEmit();
					wasEmit = false;
				}

				this->controlTime = controlTime;
				this->prepareTime = prepareTime;
				interFactor = clamp(real(prepareTime - controlTime) / controlThread::timePerTick, 0, 1);

				speaker()->update(prepareTime);
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
			vec3 &a = listener->listener.volumeAttenuationByDistance;
			real vol = min(real(1) / (a[0] + a[1] * dist + a[2] * dist * dist), 1);
			if (vol < 1e-5)
				return;
			soundDataBufferStruct s = api.output;
			if (data->soundMixBuffer.size() < s.frames)
				data->soundMixBuffer.resize(s.frames);
			s.buffer = data->soundMixBuffer.data();
			s.time -= voice->voice.soundStart;
			s.channels = 1;
			if (listener->listener.dopplerEffect)
			{
				real scale = 1000000 / controlThread::timePerTick;
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

	void soundEmit()
	{
		soundPrepare->emit();
	}

	void soundTick(uint64 controlTime, uint64 prepareTime)
	{
		soundPrepare->tick(controlTime, prepareTime);
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

	void soundInitialize()
	{
		soundPrepare->initialize();
	}

	void soundFinalize()
	{
		soundPrepare->finalize();
	}
}
