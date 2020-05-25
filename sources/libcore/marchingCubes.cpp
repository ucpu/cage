#include <cage-core/marchingCubes.h>

#include <dualmc.h>

namespace cage
{
	namespace
	{
		struct MarchingCubesImpl : public MarchingCubes
		{
		};
	}

	Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config)
	{
		return detail::systemArena().createImpl<MarchingCubes, MarchingCubesImpl>();
	}
}
