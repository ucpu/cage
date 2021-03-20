#include <cage-core/ambisonicsConverter.h>

namespace cage
{
	namespace
	{
		const vec3 defaultSpeakerDirections[8][8] = {
			{ // mono
				normalize(vec3(+0, 0, -1)), // C
			},
			{ // stereo
				normalize(vec3(-1, 0, -1)), // L
				normalize(vec3(+1, 0, -1)), // R
			},
			{ // 3
				normalize(vec3(-1, 0, -1)), // L
				normalize(vec3(+0, 0, -1)), // C
				normalize(vec3(+1, 0, -1)), // R
			},
			{ // 4
				normalize(vec3(-1, 0, -1)), // FL
				normalize(vec3(+1, 0, -1)), // FR
				normalize(vec3(-1, 0, +1)), // RL
				normalize(vec3(+1, 0, +1)), // RR
			},
			{ // 5
				normalize(vec3(-1, 0, -1)), // FL
				normalize(vec3(+0, 0, -1)), // C
				normalize(vec3(+1, 0, -1)), // FR
				normalize(vec3(-1, 0, +1)), // RL
				normalize(vec3(+1, 0, +1)), // RR
			},
			{ // 5.1
				normalize(vec3(-1, 0, -1)), // FL
				normalize(vec3(+0, 0, -1)), // C
				normalize(vec3(+1, 0, -1)), // FR
				normalize(vec3(-1, 0, +1)), // RL
				normalize(vec3(+1, 0, +1)), // RR
				vec3(0, 0, 0), // LFE
			},
			{ // 6.1
				normalize(vec3(-1, 0, -1)), // FL
				normalize(vec3(+0, 0, -1)), // C
				normalize(vec3(+1, 0, -1)), // FR
				normalize(vec3(-1, 0, +0)), // SL
				normalize(vec3(+1, 0, +0)), // SR
				normalize(vec3(+0, 0, +1)), // RC
				vec3(0, 0, 0), // LFE
			},
			{ // 7.1
				normalize(vec3(-1, 0, -1)), // FL
				normalize(vec3(+0, 0, -1)), // C
				normalize(vec3(+1, 0, -1)), // FR
				normalize(vec3(-1, 0, +0)), // SL
				normalize(vec3(+1, 0, +0)), // SR
				normalize(vec3(-1, 0, +1)), // RL
				normalize(vec3(+1, 0, +1)), // RR
				vec3(0, 0, 0), // LFE
			},
		};

		class AmbisonicsConverterImpl : public AmbisonicsConverter
		{
		public:
			const AmbisonicsConverterCreateConfig config;

			AmbisonicsConverterImpl(const AmbisonicsConverterCreateConfig &config) : config(config)
			{
				CAGE_ASSERT(config.channels > 0 && config.channels <= 8);
			}

			void process(PointerRange<const float> srcMono, PointerRange<float> dstPoly, const AmbisonicsData &data)
			{
				CAGE_ASSERT((dstPoly.size() % config.channels) == 0);
				CAGE_ASSERT(srcMono.size() * config.channels == dstPoly.size());

				real factors[8];
				{
					const real mono = 0.7;
					for (uint32 ch = 0; ch < config.channels; ch++)
					{
						factors[ch] = dot(data.direction, defaultSpeakerDirections[ch][config.channels]) * 0.5 + 0.5;
						factors[ch] = mono + factors[ch] * (1 - mono);
					}
				}

				float *dst = dstPoly.begin();
				for (float src : srcMono)
				{
					for (uint32 ch = 0; ch < config.channels; ch++)
						*dst++ = src * factors[ch].value;
				}
				CAGE_ASSERT(dst == dstPoly.end());
			}
		};
	}

	void AmbisonicsConverter::process(PointerRange<const float> srcMono, PointerRange<float> dstPoly, const AmbisonicsData &data)
	{
		AmbisonicsConverterImpl *impl = (AmbisonicsConverterImpl *)this;
		impl->process(srcMono, dstPoly, data);
	}

	Holder<AmbisonicsConverter> newAmbisonicsConverter(const AmbisonicsConverterCreateConfig &config)
	{
		return detail::systemArena().createImpl<AmbisonicsConverter, AmbisonicsConverterImpl>(config);
	}
}
