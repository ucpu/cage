#ifndef guard_collisionStructure_h_erthg456ter4h56r1th64rth
#define guard_collisionStructure_h_erthg456ter4h56r1th64rth

#include <cage-core/core.h>

namespace cage
{
	class Collider;
	struct CollisionPair;
	struct SpatialStructureCreateConfig;

	class CAGE_CORE_API CollisionQuery : private Immovable
	{
	public:
		uint32 name() const;
		void collider(Holder<const Collider> &c, Transform &t) const;
		Real fractionBefore() const; // the ratio between initial and final transformations just before the meshes start to collide
		Real fractionContact() const; // the ratio between initial and final transformations just when the meshes are colliding
		PointerRange<CollisionPair> collisionPairs() const;

		// finds arbitrary collision, if any
		bool query(const Collider *collider, Transform t);

		// finds a collision that is first in time, if any
		bool query(const Collider *collider, Transform t1, Transform t2); // t1 and t2 are initial and final transformations

		// finds a collision that is first along the line, if any
		bool query(Line shape);

		// finds arbitrary collision, if any
		bool query(Triangle shape);
		bool query(Plane shape);
		bool query(Sphere shape);
		bool query(Aabb shape);
		bool query(Cone shape);
		bool query(const Frustum &shape);
	};

	class CAGE_CORE_API CollisionStructure : private Immovable
	{
	public:
		void update(uint32 name, Holder<const Collider> collider, Transform t);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_CORE_API CollisionStructureCreateConfig
	{
		const SpatialStructureCreateConfig *spatialConfig = nullptr;
	};

	CAGE_CORE_API Holder<CollisionStructure> newCollisionStructure(const CollisionStructureCreateConfig &config);
	CAGE_CORE_API Holder<CollisionQuery> newCollisionQuery(Holder<const CollisionStructure> structure);
}

#endif // guard_collisionStructure_h_erthg456ter4h56r1th64rth
