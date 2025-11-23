#include <plf_colony.h>
#include <svector.h>
#include <unordered_dense.h>

#include <cage-core/concurrent.h>
#include <cage-core/math.h>

#ifdef CAGE_ENGINE_API
	#include <cage-engine/core.h>
#endif // !CAGE_ENGINE_API

using namespace cage;

namespace
{
	bool closing = false;

	void testColony()
	{
		plf::colony<Vec3> colony;
		colony.reserve(100);
		colony.insert(Vec3(13));
		colony.insert(Vec3(42));
	}

	void testSvector()
	{
		ankerl::svector<Vec3, 4> vec;
		vec.push_back(Vec3(1, 2, 3));
	}

	void testUnorderedDense()
	{
		ankerl::unordered_dense::map<uint32, Vec3> um;
		um[13] = Vec3(42);
	}
}

void testCageInstallConsistentPaths()
{
	testColony();
	testSvector();
	testUnorderedDense();

#ifdef CAGE_ENGINE_API
	static_assert(MaxTexturesCountPerMaterial > 0);
#endif // !CAGE_ENGINE_API
}
