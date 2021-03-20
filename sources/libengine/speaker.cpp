#include <cage-core/string.h>
#include <cage-core/files.h>
#include <cage-core/concurrent.h>

#include <cage-engine/speaker.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <../src/cubeb_ringbuffer.h>

#include <cubeb/cubeb.h>
#ifdef CAGE_SYSTEM_WINDOWS
#include <Objbase.h>
#endif // CAGE_SYSTEM_WINDOWS

//#define GCHL_CUBEB_LOGGING
#ifdef GCHL_CUBEB_LOGGING
#include <cstdarg>
#endif

#include <vector>

namespace cage
{
	void cageCheckCubebError(int code)
	{
		switch (code)
		{
		case CUBEB_OK:
			return;
		case CUBEB_ERROR:
			CAGE_THROW_ERROR(Exception, "generic sound error");
			break;
		case CUBEB_ERROR_INVALID_FORMAT:
			CAGE_THROW_ERROR(Exception, "invalid sound format");
			break;
		case CUBEB_ERROR_INVALID_PARAMETER:
			CAGE_THROW_ERROR(Exception, "invalid sound parameter");
			break;
		case CUBEB_ERROR_NOT_SUPPORTED:
			CAGE_THROW_ERROR(Exception, "sound not supported error");
			break;
		case CUBEB_ERROR_DEVICE_UNAVAILABLE:
			CAGE_THROW_ERROR(Exception, "sound device unavailable");
			break;
		default:
			CAGE_THROW_CRITICAL(SystemError, "unknown sound error", code);
		}
	}

	namespace
	{
		string sanitizeName(const string &name)
		{
			return replace(name, ":", "_");
		}

		string defaultName()
		{
			return sanitizeName(pathExtractFilename(detail::getExecutableFullPathNoExe()));
		}

#ifdef GCHL_CUBEB_LOGGING
		void soundLogCallback(const char *fmt, ...)
		{
			char buffer[512];
			va_list args;
			va_start(args, fmt);
			vsnprintf(buffer, 500, fmt, args);
			va_end(args);
			CAGE_LOG(SeverityEnum::Info, "cubeb", buffer);
		}
#endif

		class CageCubebInitializer
		{
		public:
			cubeb *c = nullptr;

			CageCubebInitializer()
			{
#ifdef GCHL_CUBEB_LOGGING
				cageCheckCubebError(cubeb_set_log_callback(CUBEB_LOG_VERBOSE, &soundLogCallback));
#endif
#ifdef CAGE_SYSTEM_WINDOWS
				CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif // CAGE_SYSTEM_WINDOWS
				const string name = defaultName();
				CAGE_LOG(SeverityEnum::Info, "cubeb", stringizer() + "creating cubeb context, name: '" + name + "'");
				cageCheckCubebError(cubeb_init(&c, name.c_str(), nullptr));
				CAGE_ASSERT(c);
				CAGE_LOG(SeverityEnum::Info, "cubeb", stringizer() + "using cubeb backend: '" + cubeb_get_backend_id(c) + "'");
			}

			~CageCubebInitializer()
			{
				CAGE_LOG(SeverityEnum::Info, "cubeb", stringizer() + "destroying cubeb context");
				cubeb_destroy(c);
#ifdef CAGE_SYSTEM_WINDOWS
				CoUninitialize();
#endif // CAGE_SYSTEM_WINDOWS
			}
		};
	}

	cubeb *cageCubebInitializeFunc()
	{
		static CageCubebInitializer *m = new CageCubebInitializer(); // intentional leak
		if (!m || !m->c)
			CAGE_THROW_ERROR(Exception, "cubeb initialization had failed");
		return m->c;
	}

	namespace
	{
		long dataCallbackFree(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes);
		void stateCallbackFree(cubeb_stream *stream, void *user_ptr, cubeb_state state);

		struct RingBuffer : Immovable
		{
			lock_free_audio_ring_buffer<float> ring;
			Delegate<void(const SoundCallbackData &)> callback;
			std::vector<float> buffer;
			uint64 lastTime = 0;
			const uint32 channels = 0;
			const uint32 sampleRate = 0;

			RingBuffer(uint32 channels, uint32 sampleRate, Delegate<void(const SoundCallbackData &)> callback) : ring(channels, sampleRate / 2), callback(callback), channels(channels), sampleRate(sampleRate)
			{}

			void process(uint64 currentTime)
			{
				if (lastTime == 0 || currentTime <= lastTime)
				{
					lastTime = currentTime;
					return;
				}

				const uint64 request = sampleRate * (currentTime - lastTime) / 1000000;
				lastTime += request * 1000000 / sampleRate;
				const uint32 frames = numeric_cast<uint32>(min(request, numeric_cast<uint64>(ring.available_write())));
				if (request > frames)
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "sound", "sound buffer overflow");
				if (frames == 0)
					return;

				buffer.resize(frames * channels);
				SoundCallbackData data;
				data.buffer = buffer;
				data.time = lastTime;
				data.channels = channels;
				data.frames = frames;
				data.sampleRate = sampleRate;
				callback(data);
				ring.enqueue(buffer.data(), frames);
			}

			void speaker(const SoundCallbackData &data)
			{
				if (data.frames == 0)
					return;
				CAGE_ASSERT(data.channels == channels);
				CAGE_ASSERT(data.sampleRate == sampleRate);
				const uint32 n = min(numeric_cast<uint32>(ring.available_read()), data.frames);
				uint32 r = ring.dequeue(data.buffer.data(), n);
				CAGE_ASSERT(r == n);
				if (r == data.frames)
					return;
				CAGE_LOG_DEBUG(SeverityEnum::Warning, "sound", "sound buffer underflow");
				float *buff = data.buffer.data() + r * channels;
				while (r < data.frames)
				{
					for (uint32 i = 0; i < channels; i++)
						*buff++ = 0;
					r++;
				}
				CAGE_ASSERT(buff == data.buffer.end());
			}
		};

		struct DevicesCollection : Immovable, public cubeb_device_collection
		{
			DevicesCollection()
			{
				cageCheckCubebError(cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_OUTPUT, this));
			}

			~DevicesCollection()
			{
				cubeb_device_collection_destroy(context, this);
			}

		private:
			cubeb *context = cageCubebInitializeFunc();
		};

		class SpeakerImpl : public Speaker
		{
		public:
			Holder<RingBuffer> ringBuffer;
			Delegate<void(const SoundCallbackData &)> callback;
			cubeb_stream *stream = nullptr;
			uint32 channels = 0;
			uint32 sampleRate = 0;
			uint32 latency = 0;
			bool started = false;

			SpeakerImpl(const SpeakerCreateConfig &config, Delegate<void(const SoundCallbackData &)> callback) : channels(config.channels), sampleRate(config.sampleRate)
			{
				const string name = config.name.empty() ? defaultName() : sanitizeName(config.name);
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "creating speaker, name: '" + name + "'");
				cubeb *context = cageCubebInitializeFunc();

				cubeb_devid devid = nullptr;
				if (config.deviceId.empty())
				{
					CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "requesting default device");
					if (!channels)
						cageCheckCubebError(cubeb_get_max_channel_count(context, &channels));
					if (!sampleRate)
						cageCheckCubebError(cubeb_get_preferred_sample_rate(context, &sampleRate));
					{
						cubeb_stream_params params = {};
						params.format = CUBEB_SAMPLE_FLOAT32NE;
						params.channels = channels;
						params.rate = sampleRate;
						cageCheckCubebError(cubeb_get_min_latency(context, &params, &latency));
					}
				}
				else
				{
					CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "requesting device id: '" + config.deviceId + "'");
					DevicesCollection collection;
					const cubeb_device_info *info = nullptr;
					for (uint32 index = 0; index < collection.count; index++)
					{
						const cubeb_device_info &d = collection.device[index];
						if (d.device_id == config.deviceId)
							info = &d;
					}
					if (!info)
						CAGE_THROW_ERROR(Exception, "invalid sound device id");
					if (info->state != CUBEB_DEVICE_STATE_ENABLED)
						CAGE_THROW_ERROR(Exception, "sound device is disabled or unplugged");
					devid = info->devid;
					if (!channels)
						channels = info->max_channels;
					if (!sampleRate)
						sampleRate = info->default_rate;
					latency = info->latency_lo;
				}

				if (config.ringBuffer)
				{
					ringBuffer = detail::systemArena().createHolder<RingBuffer>(channels, sampleRate, callback);
					this->callback = Delegate<void(const SoundCallbackData &)>().bind<RingBuffer, &RingBuffer::speaker>(+ringBuffer);
				}
				else
					this->callback = callback;

				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "initializing sound stream with " + channels + " channels, " + sampleRate + " Hz sample rate and " + latency + " frames latency");

				{
					cubeb_stream_params params = {};
					params.format = CUBEB_SAMPLE_FLOAT32NE;
					params.channels = channels;
					params.rate = sampleRate;
					cageCheckCubebError(cubeb_stream_init(context, &stream, name.c_str(), nullptr, nullptr, devid, &params, latency, &dataCallbackFree, &stateCallbackFree, this));
				}
			}

			~SpeakerImpl()
			{
				if (started)
					cubeb_stream_stop(stream); // no check for error
				cubeb_stream_destroy(stream);
				threadSleep(100); // give enough time for the audio thread to finish
			}

			void start()
			{
				if (started)
					return;
				started = true;
				cageCheckCubebError(cubeb_stream_start(stream));
			}

			void stop()
			{
				if (!started)
					return;
				started = false;
				cageCheckCubebError(cubeb_stream_stop(stream));
			}

			void process(uint64 currentTime)
			{
				if (ringBuffer)
					ringBuffer->process(currentTime);
			}

			void dataCallback(float *output_buffer, uint32 nframes)
			{
				SoundCallbackData data;
				data.buffer = { output_buffer, output_buffer + channels * nframes };
				data.channels = channels;
				data.frames = nframes;
				data.sampleRate = sampleRate;
				callback(data);
			}

			void stateCallback(cubeb_state state)
			{
				SoundCallbackData data;
				if (state == CUBEB_STATE_STARTED)
				{
					data.channels = channels;
					data.sampleRate = sampleRate;
				}
				callback(data);
			}
		};

		long dataCallbackFree(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes)
		{
			((SpeakerImpl *)user_ptr)->dataCallback((float *)output_buffer, numeric_cast<uint32>(nframes));
			return nframes;
		}

		void stateCallbackFree(cubeb_stream *stream, void *user_ptr, cubeb_state state)
		{
			((SpeakerImpl *)user_ptr)->stateCallback(state);
		}
	}

	uint32 Speaker::channels() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->channels;
	}

	uint32 Speaker::sampleRate() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->sampleRate;
	}

	uint32 Speaker::latency() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->latency;
	}

	void Speaker::start()
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
		impl->start();
	}

	void Speaker::stop()
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
		impl->stop();
	}

	bool Speaker::running() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->started;
	}

	void Speaker::process(uint64 timeStamp)
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
		impl->process(timeStamp);
	}

	Holder<Speaker> newSpeaker(const SpeakerCreateConfig &config, Delegate<void(const SoundCallbackData &)> callback)
	{
		return detail::systemArena().createImpl<Speaker, SpeakerImpl>(config, callback);
	}
}
