#ifndef guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA
#define guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA

#include "geometry.h"

namespace cage
{
	class CAGE_CORE_API MarchingCubes : private Immovable
	{
	public:
		PointerRange<real> densities();
		PointerRange<const real> densities() const;
		void densities(const PointerRange<const real> &values);
		real density(uint32 x, uint32 y, uint32 z) const;
		void density(uint32 x, uint32 y, uint32 z, real value);

		void updateByCoordinates(const Delegate<real(uint32, uint32, uint32)> &generator);
		void updateByPosition(const Delegate<real(const vec3 &)> &generator);

		void generateSurface();

		Holder<Collider> makeCollider() const;
		Holder<Polyhedron> makePolyhedron() const;
	};

	struct CAGE_CORE_API MarchingCubesCreateConfig
	{
		aabb box = aabb(vec3(-1), vec3(1));
		uint32 resolutionX = 20;
		uint32 resolutionY = 20;
		uint32 resolutionZ = 20;

		vec3 position(uint32 x, uint32 y, uint32 z) const;
	};

	CAGE_CORE_API Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config);
}

#endif // guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA
