#include "utilities.h"

#include <cage-core/audio.h>

namespace cage
{
	namespace soundPrivat
	{
		namespace
		{
#if defined (CAGE_SYSTEM_WINDOWS)
#pragma warning (push)
#pragma warning (disable: 4838) // disable warning about converting double to float
#pragma warning (disable: 4305) // disable warning about truncating double to float
#endif
			real mixingMatrices[8][8][64] = {
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

			vec3 speakerDirections[8][8] = {
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
		}

		void channelsMixing(float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 frames)
		{
			const real *m = mixingMatrices[channelsIn - 1][channelsOut - 1];
			for (uint32 f = 0; f < frames; f++)
			{
				const float *in = bufferIn + f * channelsIn;
				float *out = bufferOut + f * channelsOut;
				for (uint32 cout = 0; cout < channelsOut; cout++)
				{
					float res = 0;
					for (uint32 cin = 0; cin < channelsIn; cin++)
						res += m[cout * 8 + cin].value * in[cin];
					out[cout] = res;
				}
			}
		}

		namespace
		{
			const float directionalFunc(float dot)
			{
				return max(dot, 0.f);
			}
		}

		void channelsDirection(float *bufferIn, float *bufferOut, uint32 channelsOut, uint32 frames, const vec3 &direction)
		{
			if (direction == vec3())
				return channelsMixing(bufferIn, bufferOut, 1, channelsOut, frames);
			const vec3 *m = speakerDirections[channelsOut - 1];
			for (uint32 f = 0; f < frames; f++)
			{
				float in = bufferIn[f];
				float *out = bufferOut + f * channelsOut;
				for (uint32 c = 0; c < channelsOut; c++)
					out[c] = directionalFunc(dot(direction, m[c]).value) * in;
			}
		}

		void channelsOrdering(float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 frames, uint32 *channelsOrdering)
		{
#ifdef CAGE_ASSERT_ENABLED
			for (uint32 i = 0; i < channelsOut; i++)
			{
				CAGE_ASSERT(channelsOrdering[i] < channelsIn);
			}
#endif // CAGE_ASSERT_ENABLED
			for (uint32 f = 0; f < frames; f++)
			{
				for (uint32 c = 0; c < channelsOut; c++)
					bufferOut[f * channelsOut + c] = bufferIn[f * channelsIn + channelsOrdering[c]];
			}
		}

		void resample(float *bufferIn, float *bufferOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut, uint32 channels)
		{
			Holder<SampleRateConverter> cnv = newSampleRateConverter(channels);
			PointerRange<const float> src = { bufferIn, bufferIn + framesIn * channels };
			PointerRange<float> dst = { bufferOut, bufferOut + framesOut * channels };
			cnv->convert(src, dst, rateOut / (double)rateIn);
		}

		void transfer(std::vector<float> &temporary, float *bufferIn, float *bufferOut, uint32 channelsIn, uint32 channelsOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut)
		{
			temporary.resize(framesOut * channelsIn);
			resample(bufferIn, temporary.data(), framesIn, framesOut, rateIn, rateOut, channelsIn);
			channelsMixing(temporary.data(), bufferOut, channelsIn, channelsOut, framesOut);
		}
	}

	void soundSetSpeakerDirections(uint32 channels, const vec3 *directions)
	{
		CAGE_ASSERT(channels > 0 && channels < 9);
		detail::memcpy(soundPrivat::speakerDirections[channels - 1], directions, channels * sizeof(vec3));
	}

	void soundGetSpeakerDirections(uint32 channels, vec3 *directions)
	{
		CAGE_ASSERT(channels > 0 && channels < 9);
		detail::memcpy(directions, soundPrivat::speakerDirections[channels - 1], channels * sizeof(vec3));
	}

	void soundSetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, const real *matrix)
	{
		CAGE_ASSERT(channelsIn > 0 && channelsIn < 9);
		CAGE_ASSERT(channelsOut > 0 && channelsOut < 9);
		detail::memcpy(soundPrivat::mixingMatrices[channelsIn - 1][channelsOut - 1], matrix, 64 * sizeof(real));
	}

	void soundGetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, real *matrix)
	{
		CAGE_ASSERT(channelsIn > 0 && channelsIn < 9);
		CAGE_ASSERT(channelsOut > 0 && channelsOut < 9);
		detail::memcpy(matrix, soundPrivat::mixingMatrices[channelsIn - 1][channelsOut - 1], 64 * sizeof(real));
	}
}
