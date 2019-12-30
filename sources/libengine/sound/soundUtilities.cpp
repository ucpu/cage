#include <cage-core/core.h>
#include <cage-core/math.h>
#include "utilities.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>

#include <vector>

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
			float mixingMatrices[8][8][64] = {
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
			const float *m = mixingMatrices[channelsIn - 1][channelsOut - 1];
			for (uint32 f = 0; f < frames; f++)
			{
				const float *in = bufferIn + f * channelsIn;
				float *out = bufferOut + f * channelsOut;
				for (uint32 cout = 0; cout < channelsOut; cout++)
				{
					float res = 0;
					for (uint32 cin = 0; cin < channelsIn; cin++)
						res += m[cout * 8 + cin] * in[cin];
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
				CAGE_ASSERT(channelsOrdering[i] < channelsIn, channelsOrdering[i], i, channelsIn, channelsOut);
			}
#endif // CAGE_ASSERT_ENABLED
			for (uint32 f = 0; f < frames; f++)
			{
				for (uint32 c = 0; c < channelsOut; c++)
					bufferOut[f * channelsOut + c] = bufferIn[f * channelsIn + channelsOrdering[c]];
			}
		}

		namespace
		{
			void resampleNearestNeighbor(float *bufferIn, float *bufferOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut, uint32 channels)
			{
				uint32 m = framesIn * rateOut / rateIn;
				m = min(m, framesOut);
				for (uint32 fo = 0; fo < m; fo++)
				{
					uint32 fi = fo * rateIn / rateOut;
					CAGE_ASSERT(fi < framesIn, fi, framesIn, fo, framesOut);
					for (uint32 c = 0; c < channels; c++)
						bufferOut[fo * channels + c] = bufferIn[fi * channels + c];
				}
				for (uint32 fo = m; fo < framesOut; fo++)
					for (uint32 c = 0; c < channels; c++)
						bufferOut[fo * channels + c] = bufferIn[(framesIn - 1) * channels + c];
			}

			namespace
			{
				static const uint32 lanczosResolution = 512;
				static const uint32 lanczosA = 2;
				real lanczosTable[lanczosResolution];

				struct lanczosTableStruct
				{
					lanczosTableStruct()
					{
						lanczosTable[0] = 1;
						for (uint32 i = 1; i < lanczosResolution; i++)
						{
							real x = real(i * lanczosA) / real(lanczosResolution);
							real px = x * real::Pi();
							lanczosTable[i] = real(lanczosA) * sin(rads(px)) * sin(rads(px / lanczosA)) / (real::Pi() * real::Pi() * x * x);
						}
					}
				} lanczosTableInitializer;

				const real lanczos(real x)
				{
					x = abs(x) * lanczosResolution / lanczosA;
					sint32 i = clamp(numeric_cast<sint32>(x), 0, (sint32)lanczosResolution - 1);
					return lanczosTable[i];
				}
			}

			void resampleLanczos(float *bufferIn, float *bufferOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut, uint32 channels)
			{
				CAGE_ASSERT(framesIn > lanczosA * 2 + 1, framesIn);
				uint32 border = (lanczosA + 1) * rateOut / rateIn;
				CAGE_ASSERT(border < framesOut, border, framesOut, framesIn, rateOut, rateIn);
				for (uint32 o = border, e = framesOut - border; o < e; o++)
				{
					real x = real(o * rateIn) / real(rateOut);
					uint32 xf = numeric_cast<uint32>(floor(x));
					float *output = bufferOut + o * channels;
					for (uint32 ch = 0; ch < channels; ch++)
						output[ch] = 0;
					CAGE_ASSERT((sint32)xf + 1 - (sint32)lanczosA >= 0, xf);
					CAGE_ASSERT(xf + lanczosA < framesIn, xf, framesIn);
					for (uint32 i = xf + 1 - lanczosA, e = xf + lanczosA + 1; i < e; i++)
					{
						real l = lanczos(x - i);
						for (uint32 ch = 0; ch < channels; ch++)
							output[ch] += (bufferIn[i * channels + ch] * l).value;
					}
				}
				for (uint32 o = 0; o < border; o++)
					for (uint32 ch = 0; ch < channels; ch++)
						bufferOut[o * channels + ch] = bufferOut[border * channels + ch];
				for (uint32 o = framesOut - border; o < framesOut; o++)
					for (uint32 ch = 0; ch < channels; ch++)
						bufferOut[o * channels + ch] = bufferOut[(framesOut - border - 1) * channels + ch];
			}
		}

		void resample(float *bufferIn, float *bufferOut, uint32 framesIn, uint32 framesOut, uint32 rateIn, uint32 rateOut, uint32 channels)
		{
			if (rateOut > rateIn && framesOut > (lanczosA + 1) * rateOut / rateIn && framesIn > lanczosA * 2 + 1)
				return resampleLanczos(bufferIn, bufferOut, framesIn, framesOut, rateIn, rateOut, channels);
			else
				return resampleNearestNeighbor(bufferIn, bufferOut, framesIn, framesOut, rateIn, rateOut, channels);
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
		CAGE_ASSERT(channels > 0 && channels < 9, channels);
		detail::memcpy(soundPrivat::speakerDirections[channels - 1], directions, channels * sizeof(vec3));
	}

	void soundGetSpeakerDirections(uint32 channels, vec3 *directions)
	{
		CAGE_ASSERT(channels > 0 && channels < 9, channels);
		detail::memcpy(directions, soundPrivat::speakerDirections[channels - 1], channels * sizeof(vec3));
	}

	void soundSetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, const vec3 *matrix)
	{
		CAGE_ASSERT(channelsIn > 0 && channelsIn < 9, channelsIn);
		CAGE_ASSERT(channelsOut > 0 && channelsOut < 9, channelsOut);
		detail::memcpy(soundPrivat::mixingMatrices[channelsIn - 1][channelsOut - 1], matrix, 64 * sizeof(vec3));
	}

	void soundGetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, vec3 *matrix)
	{
		CAGE_ASSERT(channelsIn > 0 && channelsIn < 9, channelsIn);
		CAGE_ASSERT(channelsOut > 0 && channelsOut < 9, channelsOut);
		detail::memcpy(matrix, soundPrivat::mixingMatrices[channelsIn - 1][channelsOut - 1], 64 * sizeof(vec3));
	}
}
