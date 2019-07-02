#ifndef guard_collision_h_erthg456ter4h56r1th64rth
#define guard_collision_h_erthg456ter4h56r1th64rth

namespace cage
{
	class CAGE_API collisionQuery : private immovable
	{
	public:
		uint32 name() const;
		real fractionBefore() const;
		real fractionContact() const;
		uint32 collisionPairsCount() const;
		const collisionPair *collisionPairsData() const;
		pointerRange<const collisionPair> collisionPairs() const;
		void collider(const collisionMesh *&c, transform &t) const;

		void query(const collisionMesh *collider, const transform &t);
		void query(const collisionMesh *collider, const transform &t1, const transform &t2);

		void query(const line &shape);
		void query(const triangle &shape);
		void query(const plane &shape);
		void query(const sphere &shape);
		void query(const aabb &shape);
	};

	class CAGE_API collisionData : private immovable
	{
	public:
		void update(uint32 name, const collisionMesh *collider, const transform &t);
		void remove(uint32 name);
		void clear();
		void rebuild();
	};

	struct CAGE_API collisionDataCreateConfig
	{
		spatialDataCreateConfig *spatialConfig;
		uint32 maxCollisionPairs;
		collisionDataCreateConfig();
	};

	CAGE_API holder<collisionData> newCollisionData(const collisionDataCreateConfig &config);
	CAGE_API holder<collisionQuery> newCollisionQuery(const collisionData *data);
}

#endif // guard_collision_h_erthg456ter4h56r1th64rth
