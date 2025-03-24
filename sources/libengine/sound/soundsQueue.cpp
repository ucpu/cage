#include <algorithm>
#include <atomic>
#include <variant>
#include <vector>

#include <cage-core/assetsOnDemand.h>
#include <cage-core/audioChannelsConverter.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-engine/sound.h>
#include <cage-engine/soundsQueue.h>

namespace cage
{
	namespace
	{
		struct Stop
		{};

		struct Event
		{
			std::variant<std::monostate, Holder<Sound>, uint32, Stop> data;
			Real gain;
		};

		struct Emit
		{
			std::vector<Event> data;
		};

		struct Active
		{
			Holder<Sound> sound;
			sint64 startTime = m;
			sint64 endTime = m;
			uint32 name = 0;
			Real gain = 1;
		};

		class SoundsQueueImpl : public SoundsQueue
		{
		public:
			AssetsManager *assets = nullptr;
			Holder<AssetsOnDemand> onDemand;

			Emit emits[3];
			Holder<SwapBufferGuard> swapController;

			std::vector<Active> active;
			std::atomic<bool> playing = false;
			std::atomic<bool> purging = false;

			Holder<SampleRateConverter> rateConv;
			Holder<AudioChannelsConverter> chansConv;
			std::vector<float> tmp1, tmp2;

			SoundsQueueImpl(AssetsManager *assets) : assets(assets), onDemand(newAssetsOnDemand(assets)), swapController(newSwapBufferGuard(SwapBufferGuardCreateConfig{ .buffersCount = 3, .repeatedWrites = true })) {}

			void updateActive(sint64 currentTime)
			{
				// remove finished
				std::erase_if(active,
					[&](const Active &a) -> bool
					{
						CAGE_ASSERT(a.endTime != m);
						return a.endTime < currentTime;
					});

				// add new
				if (auto lock = swapController->read())
				{
					const sint64 endTime = currentTime + 5'000'000; // implicitly remove sounds that have failed to load in less than 5 second
					for (auto &it : emits[lock.index()].data)
					{
						std::visit(
							[&](auto &d)
							{
								using T = std::decay_t<decltype(d)>;
								if constexpr (std::is_same_v<T, Holder<Sound>>)
									active.push_back(Active{ .sound = std::move(d), .endTime = endTime, .gain = it.gain });
								if constexpr (std::is_same_v<T, uint32>)
									active.push_back(Active{ .endTime = endTime, .name = d, .gain = it.gain });
								if constexpr (std::is_same_v<T, Stop>)
									active.clear();
							},
							it.data);
					}
					emits[lock.index()].data.clear();
				}

				// update times
				for (Active &a : active)
				{
					if (a.startTime != m)
						continue;
					if (!a.sound)
						a.sound = onDemand->get<AssetSchemeIndexSound, Sound>(a.name);
					if (a.sound)
					{
						a.startTime = currentTime;
						a.endTime = a.startTime + a.sound->duration();
					}
				}

				// sort by priority
				// todo
			}

			void processActive(Active &v, const SoundCallbackData &data)
			{
				if (v.startTime == m)
					return;

				CAGE_ASSERT(v.sound);
				const Real gain = v.gain * this->gain;
				if (gain < 1e-6)
					return;

				// decode source
				const uint32 channels = v.sound->channels();
				const uint32 sampleRate = v.sound->sampleRate();
				const sintPtr startFrame = numeric_cast<sintPtr>((data.time - v.startTime) * sampleRate / 1'000'000);
				const uintPtr frames = numeric_cast<uintPtr>(uint64(data.frames) * sampleRate / data.sampleRate);
				tmp1.resize(frames * channels);
				v.sound->decode(startFrame, tmp1, false);

				// convert channels
				if (channels != data.channels)
				{
					tmp2.resize(frames * data.channels);
					chansConv->convert(tmp1, tmp2, channels, data.channels);
					std::swap(tmp1, tmp2);
				}
				CAGE_ASSERT((tmp1.size() % data.channels) == 0);

				// convert sample rate
				if (sampleRate != data.sampleRate)
				{
					tmp2.resize(data.frames * data.channels);
					rateConv->convert(tmp1, tmp2, data.sampleRate / (double)sampleRate);
					std::swap(tmp1, tmp2);
				}

				// add the result to accumulation buffer
				CAGE_ASSERT(tmp1.size() == data.buffer.size());
				auto src = tmp1.begin();
				for (float &dst : data.buffer)
					dst += *src++ * gain.value;
			}

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(data.buffer.size() == data.frames * data.channels);

				if (purging)
					return;

				updateActive(data.time);
				playing = !active.empty();
				onDemand->process();
				if (active.empty())
					return;

				if (!rateConv || rateConv->channels() != data.channels)
					rateConv = newSampleRateConverter(data.channels);
				if (!chansConv)
					chansConv = newAudioChannelsConverter();

				detail::memset(data.buffer.data(), 0, data.buffer.size() * sizeof(float));
				for (Active &a : active)
					processActive(a, data);
			}
		};
	}

	void SoundsQueue::play(Holder<Sound> sound, Real gain)
	{
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		if (auto lock = impl->swapController->write())
			impl->emits[lock.index()].data.push_back(Event{ std::move(sound), gain });
	}

	void SoundsQueue::play(uint32 soundId, Real gain)
	{
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		impl->onDemand->preload(soundId);
		if (auto lock = impl->swapController->write())
			impl->emits[lock.index()].data.push_back(Event{ soundId, gain });
	}

	void SoundsQueue::stop()
	{
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		if (auto lock = impl->swapController->write())
			impl->emits[lock.index()].data.push_back(Event{ Stop(), 0 });
	}

	bool SoundsQueue::playing() const
	{
		const SoundsQueueImpl *impl = (const SoundsQueueImpl *)this;
		return impl->playing;
	}

	void SoundsQueue::process(const SoundCallbackData &data)
	{
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		impl->process(data);
	}

	void SoundsQueue::purge()
	{
		stop();
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		impl->purging = true;
		impl->onDemand->clear();
		impl->active.clear();
		for (auto &it : impl->emits)
			it.data.clear();
	}

	Holder<SoundsQueue> newSoundsQueue(AssetsManager *assets)
	{
		return systemMemory().createImpl<SoundsQueue, SoundsQueueImpl>(assets);
	}
}
