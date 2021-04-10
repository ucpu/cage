#include <cage-core/audioChannelsConverter.h>
#include <cage-core/math.h>

namespace cage
{
	namespace
	{
#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4838) // disable warning about converting double to float
#pragma warning (disable: 4305) // disable warning about truncating double to float
#endif
		constexpr const float DefaultMixingMatrices[8][8][64] = {
			{ // from 1
				{ // to 1
					1, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 2
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 3
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 4
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
			},
			{ // from 2
				{ // to 1
					.5, .5, 0, 0, 0, 0, 0, 0,
				},
				{ // to 2
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
				},
				{ // to 3
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
				},
				{ // to 4
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
			},
			{ // from 3
				{ // to 1
					.333, .333, .333, 0, 0, 0, 0, 0,
				},
				{ // to 2
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, .5, 0, 0, 0, 0, 0,
				},
				{ // to 3
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
				},
				{ // to 4
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, .5, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
			},
			{ // from 4
				{ // to 1
					.25, .25, .25, .25, 0, 0, 0, 0,
				},
				{ // to 2
					.5, 0, .5, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
				},
				{ // to 3
					.5, 0, .5, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
				},
				{ // to 4
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					.5, 0, .5, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					.5, 0, .5, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
			},
			{ // from 5
				{ // to 1
					.2, .2, .2, .2, .2, 0, 0, 0,
				},
				{ // to 2
					.5, .25, 0, .25, 0, 0, 0, 0,
					0, .25, .5, 0, .25, 0, 0, 0,
				},
				{ // to 3
					.5, 0, 0, .5, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
				},
				{ // to 4
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, .5, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
					0, 0, 0, .5, .5, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0,
				},
			},
			{ // from 6
				{ // to 1
					.2, .2, .2, .2, .2, 0, 0, 0,
				},
				{ // to 2
					.5, .25, 0, .25, 0, 0, 0, 0,
					0, .25, .5, 0, .25, 0, 0, 0,
				},
				{ // to 3
					.5, 0, 0, .5, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
				},
				{ // to 4
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, .5, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
					0, 0, 0, .5, .5, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, .5, 0, .5, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
				},
			},
			{ // from 7
				{ // to 1
					.166, .166, .166, .166, .166, .166, 0, 0,
				},
				{ // to 2
					.5, .25, 0, .25, 0, 0, 0, 0,
					0, .25, .5, 0, .25, 0, 0, 0,
				},
				{ // to 3
					.5, 0, 0, .5, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, .5, 0, .5, 0, 0, 0,
				},
				{ // to 4
					.5, .5, 0, 0, 0, 0, 0, 0,
					0, .5, .5, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 1, 0,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 0, 0, 1, 0,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 0, 0, 1, 0,
				},
			},
			{ // from 8
				{ // to 1
					.143, .143, .143, .143, .143, .143, .143, 0,
				},
				{ // to 2
					.3, .3, 0, .2, 0, .2, 0, 0,
					0, .3, .3, 0, .2, 0, .2, 0,
				},
				{ // to 3
					.5, 0, 0, .25, 0, .25, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, .5, 0, .25, 0, .25, 0,
				},
				{ // to 4
					.5, .25, 0, .25, 0, 0, 0, 0,
					0, .25, .5, 0, .25, 0, 0, 0,
					0, 0, 0, .5, 0, .5, 0, 0,
					0, 0, 0, 0, .5, 0, .5, 0,
				},
				{ // to 5
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, .5, 0, .5, 0, 0,
					0, 0, 0, 0, .5, 0, .5, 0,
				},
				{ // to 6
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, .5, 0, .5, 0, 0,
					0, 0, 0, 0, .5, 0, .5, 0,
					0, 0, 0, 0, 0, 0, 0, 1,
				},
				{ // to 7
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, .5, .5, 0,
					0, 0, 0, 0, 0, 0, 0, 1,
				},
				{ // to 8
					1, 0, 0, 0, 0, 0, 0, 0,
					0, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 0, 0, 1, 0,
					0, 0, 0, 0, 0, 0, 0, 1,
				},
			},
		};
#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (pop)
#endif
	}

	namespace
	{
		class AudioChannelsConverterImpl : public AudioChannelsConverter
		{
		public:
			const AudioChannelsConverterCreateConfig config;

			AudioChannelsConverterImpl(const AudioChannelsConverterCreateConfig &config) : config(config)
			{}

			void convert(PointerRange<const float> src, PointerRange<float> dst, uint32 srcChannels, uint32 dstChannels)
			{
				CAGE_ASSERT(src.size() % srcChannels == 0);
				CAGE_ASSERT(dst.size() % dstChannels == 0);
				CAGE_ASSERT(src.size() * dstChannels == dst.size() * srcChannels);

				const uint64 frames = src.size() / srcChannels;
				const float *m = DefaultMixingMatrices[srcChannels - 1][dstChannels - 1];
				for (uint64 f = 0; f < frames; f++)
				{
					const float *in = src.data() + f * srcChannels;
					float *out = dst.data() + f * dstChannels;
					for (uint32 cout = 0; cout < dstChannels; cout++)
					{
						float res = 0;
						for (uint32 cin = 0; cin < srcChannels; cin++)
							res += m[cout * 8 + cin] * in[cin];
						out[cout] = res;
					}
				}
			}
		};
	}

	void AudioChannelsConverter::convert(PointerRange<const float> src, PointerRange<float> dst, uint32 srcChannels, uint32 dstChannels)
	{
		AudioChannelsConverterImpl *impl = (AudioChannelsConverterImpl *)this;
		impl->convert(src, dst, srcChannels, dstChannels);
	}

	Holder<AudioChannelsConverter> newAudioChannelsConverter(const AudioChannelsConverterCreateConfig &config)
	{
		return systemArena().createImpl<AudioChannelsConverter, AudioChannelsConverterImpl>(config);
	}
}
