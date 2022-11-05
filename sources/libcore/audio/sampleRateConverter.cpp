#include <cage-core/sampleRateConverter.h>

#include "samplerate.h"

namespace cage
{
	namespace
	{
		class SampleRateConverterImpl : public SampleRateConverter
		{
		public:
			const SampleRateConverterCreateConfig config;
			SRC_STATE *state = nullptr;

			static int convType(uint32 quality)
			{
				switch (quality)
				{
				case 0: return SRC_ZERO_ORDER_HOLD;
				case 1: return SRC_LINEAR;
				case 2: return SRC_SINC_FASTEST;
				case 3: return SRC_SINC_MEDIUM_QUALITY;
				case 4: return SRC_SINC_BEST_QUALITY;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid sample rate conversion quality option");
				}
			}

			SampleRateConverterImpl(const SampleRateConverterCreateConfig &config) : config(config)
			{
				int err = 0;
				state = src_new(convType(config.quality), config.channels, &err);
				if (!state)
					handleError(err);
			}

			~SampleRateConverterImpl()
			{
				src_delete(state);
			}

			void handleError(int err)
			{
				if (err == 0)
					return;
				CAGE_LOG_THROW(src_strerror(err));
				CAGE_THROW_ERROR(Exception, "audio sample rate conversion error");
			}

			void convert(PointerRange<const float> src, PointerRange<float> dst, double ratio)
			{
				CAGE_ASSERT((src.size() % config.channels) == 0);
				CAGE_ASSERT((dst.size() % config.channels) == 0);
				handleError(src_reset(state));
				SRC_DATA data = {};
				data.data_in = src.data();
				data.data_out = dst.data();
				data.input_frames = numeric_cast<long>(src.size() / config.channels); // todo split large buffers into multiple smaller passes
				data.output_frames = numeric_cast<long>(dst.size() / config.channels);
				data.src_ratio = ratio;
				data.end_of_input = 1;
				handleError(src_process(state, &data));
				//CAGE_ASSERT(data.output_frames_gen == data.output_frames);
			}

			void convert(PointerRange<const float> src, PointerRange<float> dst, double startRatio, double endRatio)
			{
				CAGE_ASSERT((src.size() % config.channels) == 0);
				CAGE_ASSERT((dst.size() % config.channels) == 0);
				handleError(src_reset(state));
				SRC_DATA data = {};
				data.data_in = src.data();
				data.data_out = dst.data();
				data.input_frames = numeric_cast<long>(src.size() / config.channels); // todo split large buffers into multiple smaller passes
				data.output_frames = numeric_cast<long>(dst.size() / config.channels);
				data.src_ratio = startRatio;
				data.end_of_input = 1;
				handleError(src_set_ratio(state, startRatio));
				handleError(src_set_ratio(state, endRatio));
				handleError(src_process(state, &data));
				CAGE_ASSERT(data.output_frames_gen == data.output_frames);
			}
		};
	}

	uint32 SampleRateConverter::channels() const
	{
		const SampleRateConverterImpl *impl = (const SampleRateConverterImpl *)this;
		return impl->config.channels;
	}

	void SampleRateConverter::convert(PointerRange<const float> src, PointerRange<float> dst, double ratio)
	{
		SampleRateConverterImpl *impl = (SampleRateConverterImpl *)this;
		impl->convert(src, dst, ratio);
	}

	void SampleRateConverter::convert(PointerRange<const float> src, PointerRange<float> dst, double startRatio, double endRatio)
	{
		SampleRateConverterImpl *impl = (SampleRateConverterImpl *)this;
		impl->convert(src, dst, startRatio, endRatio);
	}

	Holder<SampleRateConverter> newSampleRateConverter(const SampleRateConverterCreateConfig &config)
	{
		return systemMemory().createImpl<SampleRateConverter, SampleRateConverterImpl>(config);
	}
}
