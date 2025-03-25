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
		class SoundsQueueImpl;

		struct Event : private Noncopyable, public SoundEventConfig
		{
			SoundsQueueImpl *impl = nullptr;
			Holder<Sound> sound;
			sint64 startTime = m;
			sint64 endTime = m;
			uint32 name = 0;
			Real effectiveGain = 0;

			Event(SoundsQueueImpl *impl);
			Event(Event &&other) noexcept;
			~Event();
			Event &operator=(Event &&ee) noexcept = default;
		};

		struct Emit
		{
			std::vector<Event> data;
		};

		class SoundsQueueImpl : public SoundsQueue
		{
		public:
			AssetsManager *assets = nullptr;
			Holder<AssetsOnDemand> onDemand;

			Emit emits[3];
			Holder<SwapBufferGuard> swapController;

			std::vector<Event> active;

			std::atomic<sint32> counter = 0;
			std::atomic<uint64> elapsed = 0;
			std::atomic<uint64> remaining = 0;
			std::atomic<bool> purging = false;

			Holder<SampleRateConverter> rateConv;
			Holder<AudioChannelsConverter> chansConv;
			std::vector<float> tmp1, tmp2;

			SoundsQueueImpl(AssetsManager *assets) : assets(assets), onDemand(newAssetsOnDemand(assets)), swapController(newSwapBufferGuard(SwapBufferGuardCreateConfig{ .buffersCount = 3, .repeatedWrites = true })) {}

			void updateEvents(sint64 currentTime)
			{
				// remove finished
				std::erase_if(active,
					[&](const Event &a) -> bool
					{
						CAGE_ASSERT(a.endTime != m);
						return a.endTime < currentTime;
					});

				// add new
				if (auto lock = swapController->read())
				{
					for (Event &it : emits[lock.index()].data)
					{
						it.endTime = currentTime + it.maxDelay;
						active.push_back(std::move(it));
					}
					emits[lock.index()].data.clear();
				}

				// update times
				{
					sint64 minStart = 0;
					sint64 maxEnd = 0;
					for (Event &a : active)
					{
						if (a.startTime == m)
						{
							if (!a.sound)
								a.sound = onDemand->get<AssetSchemeIndexSound, Sound>(a.name);
							if (a.sound)
							{
								a.startTime = currentTime;
								a.endTime = a.startTime + a.sound->duration();
							}
						}
						if (a.startTime != m)
						{
							minStart = min(minStart, a.startTime);
							maxEnd = max(maxEnd, a.endTime);
						}
					}
					const uint64 elp = max(currentTime - minStart, sint64(0));
					elapsed.store(elp, std::memory_order_relaxed);
					const uint64 rem = max(maxEnd - currentTime, sint64(0));
					remaining.store(rem, std::memory_order_relaxed);
				}

				// update effective gains
				for (Event &a : active)
					a.effectiveGain = a.gain * this->gain;

				// sort by priority
				std::sort(active.begin(), active.end(), [](const Event &a, const Event &b) -> bool { return std::pair(a.priority, a.effectiveGain) > std::pair(b.priority, b.effectiveGain); });
				for (uint32 i = maxActiveSounds; i < active.size(); i++)
					active[i].effectiveGain = 0;
			}

			void processEvent(Event &v, const SoundCallbackData &data)
			{
				if (v.startTime == m || v.effectiveGain < 1e-5)
					return;

				CAGE_ASSERT(v.sound);

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
				const float g = v.effectiveGain.value;
				for (float &dst : data.buffer)
					dst += *src++ * g;
			}

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(data.buffer.size() == data.frames * data.channels);
				CAGE_ASSERT(gain.valid() && gain >= 0 && gain.finite());
				detail::memset(data.buffer.data(), 0, data.buffer.size() * sizeof(float));

				if (purging)
					return;

				updateEvents(data.time);
				onDemand->process();
				if (active.empty() || gain < 1e-5)
					return;

				if (!rateConv || rateConv->channels() != data.channels)
					rateConv = newSampleRateConverter(data.channels);
				if (!chansConv)
					chansConv = newAudioChannelsConverter();

				for (Event &a : active)
					processEvent(a, data);
			}
		};

		Event::Event(SoundsQueueImpl *impl) : impl(impl)
		{
			CAGE_ASSERT(impl);
			impl->counter.fetch_add(1, std::memory_order_relaxed);
		}

		Event::~Event()
		{
			impl->counter.fetch_add(-1, std::memory_order_relaxed);
		}

		Event::Event(Event &&other) noexcept
		{
			*this = std::move(other);
			impl->counter.fetch_add(1, std::memory_order_relaxed);
		}
	}

	void SoundsQueue::play(Holder<Sound> sound, const SoundEventConfig &cfg)
	{
		CAGE_ASSERT(cfg.gain.valid() && cfg.gain >= 0);
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		if (auto lock = impl->swapController->write())
		{
			Event e(impl);
			e.sound = std::move(sound);
			(SoundEventConfig &)e = cfg;
			impl->emits[lock.index()].data.push_back(std::move(e));
		}
	}

	void SoundsQueue::play(uint32 soundId, const SoundEventConfig &cfg)
	{
		CAGE_ASSERT(cfg.gain.valid() && cfg.gain >= 0);
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		impl->onDemand->preload(soundId);
		if (auto lock = impl->swapController->write())
		{
			Event e(impl);
			e.name = soundId;
			(SoundEventConfig &)e = cfg;
			impl->emits[lock.index()].data.push_back(std::move(e));
		}
	}

	bool SoundsQueue::playing() const
	{
		const SoundsQueueImpl *impl = (const SoundsQueueImpl *)this;
		return impl->counter.load(std::memory_order_relaxed) > 0;
	}

	uint64 SoundsQueue::elapsedTime() const
	{
		const SoundsQueueImpl *impl = (const SoundsQueueImpl *)this;
		return impl->elapsed.load(std::memory_order_relaxed);
	}

	uint64 SoundsQueue::remainingTime() const
	{
		const SoundsQueueImpl *impl = (const SoundsQueueImpl *)this;
		return impl->remaining.load(std::memory_order_relaxed);
	}

	void SoundsQueue::process(const SoundCallbackData &data)
	{
		SoundsQueueImpl *impl = (SoundsQueueImpl *)this;
		impl->process(data);
	}

	void SoundsQueue::purge()
	{
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
