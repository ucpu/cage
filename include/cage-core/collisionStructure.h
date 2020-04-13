#ifndef guard_collision_h_erthg456ter4h56r1th64rth
#define guard_collision_h_erthg456ter4h56r1th64rth

#include "core.h"

namespace cage
{
	class CAGE_CORE_API CollisionQuery : private Immovable
	{
	public:
		uint32 name() const;
		void collider(const CollisionMesh *&c, transform &t) const;
		real fractionBefore() const; // the ratio between initial and final transformations just before the meshes start to collide
		real fractionContact() const; // the ratio between initial and final transformations just when the meshes are colliding
		PointerRange<CollisionPair> collisionPairs() const;

		void query(const CollisionMesh *collider, const transform &t);
		void query(const CollisionMesh *collider, const transform &t1, const transform &t2); // t1 and t2 are initial and final transformations

		void query(const line &shape);
		void query(const triangle &shape);
		void query(const plane &shape);
		void query(const sphere &shape);
		void query(const aabb &shape);
	};

	class CAGE_CORE_API CollisionStructure : private Immovable
	{
	public:
		void update(uint32 name, const CollisionMesh *collider, const transform &t);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_CORE_API CollisionStructureCreateConfig
	{
		const SpatialStructureCreateConfig *spatialConfig = nullptr;
	};

	CAGE_CORE_API Holder<CollisionStructure> newCollisionStructure(const CollisionStructureCreateConfig &config);
	CAGE_CORE_API Holder<CollisionQuery> newCollisionQuery(const CollisionStructure *structure);
}

#endif // guard_collision_h_erthg456ter4h56r1th64rth
