#include <cage-core/audioChannelsConverter.h>
#include <cage-core/math.h>

namespace cage
{
	namespace
	{
		constexpr const float DefaultMixingMatrices[8][8][64] = {
			{
				// from 1
				{
					// to 1
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 2
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 3
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 4
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
			},
			{
				// from 2
				{
					// to 1
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 2
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 3
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 4
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
			},
			{
				// from 3
				{
					// to 1
					.333f,
					.333f,
					.333f,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 2
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 3
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 4
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
			},
			{
				// from 4
				{
					// to 1
					.25f,
					.25f,
					.25f,
					.25f,
					0,
					0,
					0,
					0,
				},
				{
					// to 2
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
				},
				{
					// to 3
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
				},
				{
					// to 4
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
			},
			{
				// from 5
				{
					// to 1
					.2f,
					.2f,
					.2f,
					.2f,
					.2f,
					0,
					0,
					0,
				},
				{
					// to 2
					.5f,
					.25f,
					0,
					.25f,
					0,
					0,
					0,
					0,
					0,
					.25f,
					.5f,
					0,
					.25f,
					0,
					0,
					0,
				},
				{
					// to 3
					.5f,
					0,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
				},
				{
					// to 4
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
				},
			},
			{
				// from 6
				{
					// to 1
					.2f,
					.2f,
					.2f,
					.2f,
					.2f,
					0,
					0,
					0,
				},
				{
					// to 2
					.5f,
					.25f,
					0,
					.25f,
					0,
					0,
					0,
					0,
					0,
					.25f,
					.5f,
					0,
					.25f,
					0,
					0,
					0,
				},
				{
					// to 3
					.5f,
					0,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
				},
				{
					// to 4
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
				},
			},
			{
				// from 7
				{
					// to 1
					.166f,
					.166f,
					.166f,
					.166f,
					.166f,
					.166f,
					0,
					0,
				},
				{
					// to 2
					.5f,
					.25f,
					0,
					.25f,
					0,
					0,
					0,
					0,
					0,
					.25f,
					.5f,
					0,
					.25f,
					0,
					0,
					0,
				},
				{
					// to 3
					.5f,
					0,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
				},
				{
					// to 4
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
				},
			},
			{
				// from 8
				{
					// to 1
					.143f,
					.143f,
					.143f,
					.143f,
					.143f,
					.143f,
					.143f,
					0,
				},
				{
					// to 2
					.3f,
					.3f,
					0,
					.2f,
					0,
					.2f,
					0,
					0,
					0,
					.3f,
					.3f,
					0,
					.2f,
					0,
					.2f,
					0,
				},
				{
					// to 3
					.5f,
					0,
					0,
					.25f,
					0,
					.25f,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.25f,
					0,
					.25f,
					0,
				},
				{
					// to 4
					.5f,
					.25f,
					0,
					.25f,
					0,
					0,
					0,
					0,
					0,
					.25f,
					.5f,
					0,
					.25f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
				},
				{
					// to 5
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
				},
				{
					// to 6
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					0,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
				},
				{
					// to 7
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					.5f,
					.5f,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
				},
				{
					// to 8
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					1,
				},
			},
		};
	}

	namespace
	{
		class AudioChannelsConverterImpl : public AudioChannelsConverter
		{
		public:
			const AudioChannelsConverterCreateConfig config;

			AudioChannelsConverterImpl(const AudioChannelsConverterCreateConfig &config) : config(config) {}

			void convert(PointerRange<const float> src, PointerRange<float> dst, uint32 srcChannels, uint32 dstChannels)
			{
				CAGE_ASSERT(src.size() % srcChannels == 0);
				CAGE_ASSERT(dst.size() % dstChannels == 0);
				CAGE_ASSERT(src.size() * dstChannels == dst.size() * srcChannels);

				const uintPtr frames = src.size() / srcChannels;
				const float *m = DefaultMixingMatrices[srcChannels - 1][dstChannels - 1];
				for (uintPtr f = 0; f < frames; f++)
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
		return systemMemory().createImpl<AudioChannelsConverter, AudioChannelsConverterImpl>(config);
	}
}
