#ifndef guard_collision_h_erthg456ter4h56r1th64rth
#define guard_collision_h_erthg456ter4h56r1th64rth

namespace cage
{
	class CAGE_API CollisionQuery : private Immovable
	{
	public:
		uint32 name() const;
		void collider(const CollisionMesh *&c, transform &t) const;
		real fractionBefore() const;
		real fractionContact() const;
		PointerRange<CollisionPair> collisionPairs() const;

		void query(const CollisionMesh *collider, const transform &t);
		void query(const CollisionMesh *collider, const transform &t1, const transform &t2);

		void query(const line &shape);
		void query(const triangle &shape);
		void query(const plane &shape);
		void query(const sphere &shape);
		void query(const aabb &shape);
	};

	class CAGE_API CollisionData : private Immovable
	{
	public:
		void update(uint32 name, const CollisionMesh *collider, const transform &t);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_API CollisionDataCreateConfig
	{
		const SpatialDataCreateConfig *spatialConfig;
		uint32 maxCollisionPairs;
		CollisionDataCreateConfig();
	};

	CAGE_API Holder<CollisionData> newCollisionData(const CollisionDataCreateConfig &config);
	CAGE_API Holder<CollisionQuery> newCollisionQuery(const CollisionData *data);
}

#endif // guard_collision_h_erthg456ter4h56r1th64rth
