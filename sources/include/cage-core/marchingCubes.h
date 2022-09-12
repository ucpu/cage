#ifndef guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA
#define guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA

#include "geometry.h"

namespace cage
{
	class Collider;
	class Mesh;

	class CAGE_CORE_API MarchingCubes : private Immovable
	{
	public:
		PointerRange<Real> densities();
		PointerRange<const Real> densities() const;
		void densities(const PointerRange<const Real> &values);
		Real density(uint32 x, uint32 y, uint32 z) const;
		void density(uint32 x, uint32 y, uint32 z, Real value);

		void updateByCoordinates(const Delegate<Real(uint32, uint32, uint32)> &generator);
		void updateByPosition(const Delegate<Real(const Vec3 &)> &generator);

		Holder<Collider> makeCollider() const;
		Holder<Mesh> makeMesh() const;
	};

	struct CAGE_CORE_API MarchingCubesCreateConfig
	{
		Aabb box = Aabb(Vec3(-1), Vec3(1));
		Vec3i resolution = Vec3i(20);
		bool clip = true;

		Vec3 position(uint32 x, uint32 y, uint32 z) const;
		uint32 index(uint32 x, uint32 y, uint32 z) const;
	};

	CAGE_CORE_API Holder<MarchingCubes> newMarchingCubes(const MarchingCubesCreateConfig &config);
}

#endif // guard_marchingCubes_h_5C676C29C593444FAD41FE48E265E5DA
